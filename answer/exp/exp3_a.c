/*
 * char fifo
 * */
extern int trace_printf(const char *format, ...);
extern char board_getc(void);
extern int board_putc(unsigned char c);
extern int board_getc_ready(void);
extern int board_putc_ready(void);
extern int led_cmd_exec(int argc, char *argv[]);

typedef char filo_data_t;
typedef struct fifo8 {
	filo_data_t *data;
	int capacity;
	int num;
	int head;
} fifo8_t;

void fifo_create(fifo8_t *fifo, filo_data_t *data_mem, int capacity) {
	fifo->capacity = capacity;
	fifo->data = data_mem;
	fifo->head = 0;
	fifo->num = 0;
}

#define MOD(x, y) (x) >= (y) ?  (x) - (y): (x)
#define INVALID_DATA 0

int fifo_empty(fifo8_t *fifo) {
	return (fifo->num <= 0);
}

int fifo_full(fifo8_t *fifo) {
	return (fifo->num >= fifo->capacity);
}

void fifo_push(fifo8_t *fifo, filo_data_t data) {
	if (fifo_full(fifo)) {
		trace_printf("fifo full!");
		return;
	}
	fifo->data[MOD((fifo->head + fifo->num), fifo->capacity)] = data;
	fifo->num++;
}

filo_data_t fifo_pop(fifo8_t *fifo) {
	if (fifo_empty(fifo)) {
		trace_printf("fifo empty!");
		return INVALID_DATA;
	}
	filo_data_t data;
	data = fifo->data[fifo->head];
	fifo->head = MOD(fifo->head + 1, fifo->capacity);
	fifo->num--;
	return data;
}

/*
 * poll structure
 */
typedef int (*pfn_poll_t)(void);
typedef void (*pfn_handler_t)(void);

typedef struct poller {
	pfn_poll_t poll;
	pfn_handler_t handler;
} poller_t;

#define MAX_POLL_ENTRY 10
static poller_t pollers[MAX_POLL_ENTRY];
static int free_head = 0;

void poll_loop() {
	poller_t *poller;
	for (poller = &pollers[0]; poller < &pollers[free_head]; poller++) {
		if (poller->poll && poller->handler) {
			if ((*poller->poll)()) {
				(*poller->handler)();
			}
		}
	}
}

void poll_add(pfn_poll_t poll, pfn_handler_t handler) {
	if (free_head >= MAX_POLL_ENTRY) {
		return;
	}
	pollers[free_head].poll = poll;
	pollers[free_head].handler = handler;
	free_head++;
}


/*
 * uart recv & send
 */
static char sendbuf[64];
static fifo8_t sendfifo;

#define COMMAND_LEN_MAX 32
static char command[COMMAND_LEN_MAX];
static int cmd_idx = 0;

static int split_cmdline(char *cmd, unsigned int length, char *argv[]) {
	char *ptr;
	unsigned int position;
	unsigned int argc;

	ptr = cmd;
	position = 0;
	argc = 0;

	while (1) {
		/* strip blank and tab */
		while ((*ptr == ' ' || *ptr == '\t') && position < length) {
			*ptr = '\0';
			ptr++;
			position++;
		}
		if (position >= length)
			break;

		argv[argc] = ptr;
		argc++;
		while ((*ptr != ' ' && *ptr != '\t') && position < length) {
			ptr++;
			position++;
		}
		if (position >= length)
			break;
	}
	return argc;
}

void uart_recv_process(void) {
	//# error "Not Implemented!"
	int argc=0;
	char *argv[16];
	char c=board_getc();
	fifo_push(&sendfifo,c);
	
	command[cmd_idx++]=c;
	if (c==13){
		trace_printf(command);
		argc=split_cmdline(command,cmd_idx,argv);
		led_cmd_exec(argc,argv);
		cmd_idx=0;
		memset(command,0,sizeof(command));
	}
}

void uart_send_process(void) {
	while (board_putc_ready() && !fifo_empty(&sendfifo)) {
		board_putc(fifo_pop(&sendfifo));
	}
}

int uart_readble() {
	return board_getc_ready();
}

int uart_writeable() {
	return board_putc_ready();
}

int led_ready() {
	return 1;
}
extern void led_do(void);

void exp3_a_init(){
	//init
	fifo_create(&sendfifo, sendbuf, sizeof(sendbuf));

	//add to poll
	poll_add(uart_readble, uart_recv_process);
	poll_add(uart_writeable, uart_send_process);
	poll_add(led_ready, led_do);
}

void exp3_a_do(){
	poll_loop();
}

/*
 * create send fifo but no recv fifo
 */
int stop_exp3_a;
void exp3_a_main() {
	exp3_a_init();
	//big loop
	while (!stop_exp3_a) {
		exp3_a_do();
	}
}

