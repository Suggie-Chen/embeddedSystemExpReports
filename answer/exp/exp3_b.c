#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "thread.h"
#include "stdarg.h"
#include "limits.h"

extern int trace_printf(const char *format, ...);
extern int led_cmd_exec(int argc, char *argv[]);
extern void led_off(int n);
extern void led_on(int n);
extern void led_do(void);
extern void led_ready(void);
extern void set_state(int n,int state);

#define CMD_ARG_MAX    5
#define MAX_HISTORY_LINES 10
#define CMD_SIZE		80

#define SHELL_PROMPT     "#"
#define SHELL_ESCAPSE_CHAR	0x1d	/*Ctrl + ]*/

#define CR "\r\n"

enum input_stat {
	WAIT_NORMAL, WAIT_SPEC_KEY, WAIT_FUNC_KEY,
};

typedef struct uart_shell {
	unsigned int echo_mode;

	enum input_stat stat;

	char line[CMD_SIZE];
	int line_position;
	int line_curpos;

	unsigned int current_history;
	unsigned int history_count;
	char cmd_history[MAX_HISTORY_LINES][CMD_SIZE];

	/*input and output function for frontend*/
	char (*_getchar)(void);
	void (*_putchar)(char ch);
} uart_shell_t;

typedef void (*cmd_function_t)(struct uart_shell *shell, int argc, char *argv[]);

/* system cmd table */
struct ush_cmd {
	const char *name; /* the name of system call */
	const char *desc; /* description of system call */
	cmd_function_t func;/* the function address */
};

extern void cmd_help(struct uart_shell *shell, int argc, char *argv[]);
extern void cmd_led(struct uart_shell *shell, int argc, char *argv[]);
extern void cmd_d(struct uart_shell *shell, int argc, char *argv[]);

#define MAX_CMD 16
static struct ush_cmd ush_cmd_table[MAX_CMD] = { { "?", "print this help",
		cmd_help }, //help
		{ "help", "print this help", cmd_help }, 		//help
		{ "led", "led n on/off/flash [!]", cmd_led }, //led
		{ "d", "d [address size], memory display", cmd_d }, //dispaly memory
		};

//struct ush_cmd *ush_cmd_table_begin = &ush_cmd_table[0];
//struct ush_cmd *ush_cmd_table_end = &ush_cmd_table[sizeof(ush_cmd_table)
//		/ sizeof(ush_cmd_table[0])];

struct ush_cmd* ush_cmd_add(void *func, const char *name, const char *desc) {
	struct ush_cmd *s_cmd;
	for (s_cmd = &ush_cmd_table[0]; s_cmd < &ush_cmd_table[MAX_CMD]; s_cmd++) {
		if (!s_cmd->func) {
			s_cmd->name = name;
			s_cmd->desc = desc;
			s_cmd->func = (cmd_function_t)func;
			return s_cmd;
		}
	}
	return NULL;
}

#define PRINTF_BUFFER_SIZE 128
void ush_printf(struct uart_shell *shell, char *fmt, ...) {
	va_list ap;
	char buffer[PRINTF_BUFFER_SIZE];
	char *chstring = buffer;

	if (!shell->_putchar)
		return;

	va_start(ap, fmt);
	vsnprintf(buffer, PRINTF_BUFFER_SIZE, fmt, ap);
	while (*chstring)
		shell->_putchar(*chstring++);
	va_end(ap);
}



static int shell_show_history(struct uart_shell *shell) {
	/*clear command  display*/
	ush_printf(shell, "\r																			   \r"); //clean line, for windows
	//ush_printf(shell, "\033[2K\r");/*this works fine */
	ush_printf(shell, "%s%s", SHELL_PROMPT, shell->line);
	return 0;
}

static void shell_push_history(struct uart_shell *shell) {
	if (shell->line_position != 0) {
		/* push history */
		if (shell->history_count >= MAX_HISTORY_LINES) {
			/* move history */
			int index;
			for (index = 0; index < MAX_HISTORY_LINES - 1; index++) {
				memcpy(&shell->cmd_history[index][0],
						&shell->cmd_history[index + 1][0], CMD_SIZE);
			}
			memset(&shell->cmd_history[index][0], 0, CMD_SIZE);
			memcpy(&shell->cmd_history[index][0], shell->line,
					shell->line_position);

			/* it's the maximum history */
			shell->history_count = MAX_HISTORY_LINES;
		} else {
			memset(&shell->cmd_history[shell->history_count][0], 0, CMD_SIZE);
			memcpy(&shell->cmd_history[shell->history_count][0], shell->line,
					shell->line_position);

			/* increase count and set current history position */
			shell->history_count++;
		}
	}
	shell->current_history = shell->history_count;
}

static int str_common(const char *str1, const char *str2) {
	const char *str = str1;

	while ((*str != 0) && (*str2 != 0) && (*str == *str2)) {
		str++;
		str2++;
	}

	return (str - str1);
}

void ush_auto_complete_internal(struct uart_shell *shell, char *prefix) {
	int length, min_length;
	const char *name_ptr, *cmd_name;
	struct ush_cmd *s_cmd;

	min_length = 0;
	name_ptr = NULL;

	if (*prefix == '\0') {
		cmd_help(shell, 0, NULL);
		return;
	}

	/* checks in internal command */
	{
		for (s_cmd = &ush_cmd_table[0]; s_cmd->func != NULL; s_cmd++) {
			cmd_name = (const char*) s_cmd->name;
			if (strncmp(prefix, cmd_name, strlen(prefix)) == 0) {
				if (min_length == 0) {
					/* set name_ptr */
					name_ptr = cmd_name;
					/* set initial length */
					min_length = strlen(name_ptr);
				}

				length = str_common(name_ptr, cmd_name);
				if (length < min_length)
					min_length = length;

				ush_printf(shell, "%s"CR, cmd_name);
			}
		}
	}

	/* auto complete string */
	if (name_ptr != NULL) {
		strncpy(prefix, name_ptr, min_length);
	}

	return;
}

static void ush_auto_complete(struct uart_shell *shell, char *prefix) {
	ush_printf(shell, CR);
	ush_auto_complete_internal(shell, prefix);
	ush_printf(shell, "%s%s", SHELL_PROMPT, prefix);
}

static int ush_split_cmdline(char *cmd, unsigned int length,
		char *argv[CMD_ARG_MAX]) {
	char *ptr;
	unsigned int position;
	unsigned int argc;

	ptr = cmd;
	position = 0;
	argc = 0;

	while (position < length) {
		/* strip blank and tab */
		while ((*ptr == ' ' || *ptr == '\t') && position < length) {
			*ptr = '\0';
			ptr++;
			position++;
		}
		if (position >= length)
			break;

		/* handle string */
		if (*ptr == '"') {
			ptr++;
			position++;
			argv[argc] = ptr;
			argc++;

			/* skip this string */
			while (*ptr != '"' && position < length) {
				if (*ptr == '\\') {
					if (*(ptr + 1) == '"') {
						ptr++;
						position++;
					}
				}
				ptr++;
				position++;
			}
			if (position >= length)
				break;

			/* skip '"' */
			*ptr = '\0';
			ptr++;
			position++;
		} else {
			argv[argc] = ptr;
			argc++;
			while ((*ptr != ' ' && *ptr != '\t') && position < length) {
				ptr++;
				position++;
			}
			if (position >= length)
				break;
		}
	}

	return argc;
}

static cmd_function_t ush_find_cmd(char *cmd, int size) {
	struct ush_cmd *s_cmd;
	cmd_function_t cmd_func = NULL;

	for (s_cmd = &ush_cmd_table[0]; s_cmd->func != NULL; s_cmd++) {
		if (strncmp(s_cmd->name, cmd, size) == 0 && s_cmd->name[size] == '\0') {
			cmd_func = (cmd_function_t) s_cmd->func;
			break;
		}
	}

	return cmd_func;
}

static int ush_parse_and_exec_cmd(struct uart_shell *shell, char *cmd,
		unsigned int length, int *retp) {
	int argc;
	unsigned int cmd0_size = 0;
	cmd_function_t cmd_func;
	char *argv[CMD_ARG_MAX];

	if ((!cmd) || (!retp))
		return -1;

	/* find the size of first command */
	while ((cmd[cmd0_size] != ' ' && cmd[cmd0_size] != '\t')
			&& cmd0_size < length)
		cmd0_size++;
	if (cmd0_size == 0)
		return -1;

	cmd_func = ush_find_cmd(cmd, cmd0_size);
	if (cmd_func == NULL)
		return -1;

	/* split arguments */
	memset(argv, 0x00, sizeof(argv));
	argc = ush_split_cmdline(cmd, length, argv);
	if (argc == 0)
		return -1;

	/* exec this command */
	/**retp = */cmd_func(shell, argc, argv);
	return 0;
}

int ush_execute_internal(struct uart_shell *shell, char *cmd,
		unsigned int length) {
	int cmd_ret;

	/* strim the beginning of command */
	while (*cmd == ' ' || *cmd == '\t') {
		cmd++;
		length--;
	}

	if (length == 0)
		return 0;

	/* Exec command*/
	if (ush_parse_and_exec_cmd(shell, cmd, length, &cmd_ret) == 0) {
		return cmd_ret;
	}

	/* truncate the cmd at the first space. */
	{
		char *tcmd;
		tcmd = cmd;
		while (*tcmd != ' ' && *tcmd != '\0') {
			tcmd++;
		}
		*tcmd = '\0';
	}
	ush_printf(shell, "%s: command not found."CR, cmd);
	return -1;
}

void print_line(struct uart_shell *shell) {
    ush_printf(shell, "\033[2K\r");
	for (int i = 0;  i < shell->line_position; i++)
		ush_printf(shell, "%c", shell->line[i]);
}

void delete_ch(struct uart_shell *shell) {
	for(int i = shell->line_curpos; i < shell->line_position; i++){
		shell->line[i - 1] = shell->line[i];
	}
	shell->line_position--;
	shell->line_curpos--;
	shell->line[shell->line_position] = '\0';
	print_line(shell);
}

char* shell_readline(struct uart_shell *shell) {
	
	char ch = 0;
	static int state = 0;
	shell->line_curpos = 0;
	while (1) {
		
		poll_loop();
		/* read one character from device */
		ch = shell->_getchar();

		if (ch == SHELL_ESCAPSE_CHAR) {
			return NULL;
		}

		/*
		 * handle control key
		 * up key  : 0x1b 0x5b 0x41
		 * down key: 0x1b 0x5b 0x42
		 * right key:0x1b 0x5b 0x43
		 * left key: 0x1b 0x5b 0x44
		 */
		if (ch == 0x1b && state == 0){
			state = 1;
			continue;
		}
		if (ch == 0x5b && state == 1){
			state = 2;
			continue;
		}
		if (ch == 0x41 && state == 2) {
			state = 0;
			if (shell->current_history > 0) {
				while(shell->line_curpos > 0) {
					delete_ch(shell);
				}
				shell->current_history--;
				memcpy(shell->line,&shell->cmd_history[shell->current_history][0],CMD_SIZE);
				ush_printf(shell, "using history <<%s>>" "\r\n", shell->line);
				shell->line_curpos = shell->line_position = strlen(shell->line);
			}
			continue;
		}
		if(ch == 0x42 && state == 2) {
			state = 0;
			if (shell->current_history + 1 < shell->history_count) {
				shell->current_history++;
				memcpy(shell->line,&shell->cmd_history[shell->current_history][0],CMD_SIZE);
				ush_printf(shell, "using history <<%s>>" "\r\n", shell->line);
				shell->line_curpos = shell->line_position = strlen(shell->line);
			} else if (shell->current_history == shell->history_count - 1) {
				shell->current_history++;
				ush_printf(shell, "reset" "\r\n");
				memset(shell->line, 0, CMD_SIZE);
				shell->line_curpos = shell->line_position = strlen(shell->line);
			}
			continue;
		}
		if(ch == 0x43 && state == 2) {
			state = 0;
			print_line(shell);
			if (shell->line_curpos < shell->line_position) {
				shell->line_curpos = shell->line_curpos + 1;
				ush_printf(shell, "\033[%dC\r");
			}
			continue;
		}
		if(ch == 0x44 && state == 2) {
			state = 0;
			if(shell->line_curpos > 0) {
				
				shell->line_curpos--;
				ush_printf(shell, "\033[%dD");
				
			}
			continue;
		}
		/* handle tab key */
		if (ch == '\t') {
			shell->line[shell->line_position] = '\0';
			ush_auto_complete(shell,shell->line);
			shell->line_curpos = shell->line_position = strlen(shell->line);
			print_line(shell);
			continue;
		}
		/* handle backspace key */
		if (ch == 0x7f || ch == 0x08) {
			state=0;
			if(shell->line_curpos > 0) {
				delete_ch(shell);
			}
			continue;
		}
		/* handle end of line, break */
		if (ch == '\n' || ch == '\r') {
			state=0;
			print_line(shell);
			shell_push_history(shell);
			//shell_execute(shell);
			/*exec command*/
			return shell->line;
		}
		/* handle normal character */
		for (int i = shell->line_position; i > shell->line_curpos; i--){
			shell->line[i] = shell->line[i-1];
		}
		state=0;
		
		shell->line_position++;
		shell->line[shell->line_curpos] = ch;
		shell->line_curpos++;
		print_line(shell);
	}//end of while(1)
}

//execute a command in shell->line
void shell_execute(struct uart_shell *shell) {

	ush_printf(shell, CR);

	ush_execute_internal(shell, shell->line, shell->line_position);

	ush_printf(shell, SHELL_PROMPT);
	memset(shell->line, 0, sizeof(shell->line));
	shell->line_curpos = shell->line_position = 0;
}

//Main loop, get input and execute it.
void shell_loop(struct uart_shell *shell) {

	ush_printf(shell, SHELL_PROMPT);

	//loop until ctrl+] received
	while (shell_readline(shell)) {
		shell_execute(shell);
	}

	ush_printf(shell, CR"Bye Bye"CR);
}

static struct uart_shell ushell;
/*return struct uart_shell*, keep it for ush_teardown*/
void* ush_init(char (*_getchar)(), void (*_putchar)(char ch)) {
	struct uart_shell *shell = &ushell;

	memset(shell, 0, sizeof(struct uart_shell));

	/* normal is echo mode */
	shell->echo_mode = 1;
	shell->stat = WAIT_NORMAL;
	shell->_getchar = _getchar;
	shell->_putchar = _putchar;

	return (shell);
}

/***************************************************************
 * ush commands
 ***************************************************************/

void cmd_help(struct uart_shell *shell, int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	ush_printf(shell, "shell commands:"CR);
	{
		struct ush_cmd *s_cmd;

		for (s_cmd = &ush_cmd_table[0]; s_cmd->func != NULL; s_cmd++) {
			ush_printf(shell, "%-16s - %s"CR, s_cmd->name, s_cmd->desc);
		}
	}
	ush_printf(shell, CR);

	return;
}

void cmd_led(struct uart_shell *shell, int argc, char *argv[]) {
	if (argc == 0) {
		return;
	}
	int ledn = 0;
	if (!strcmp(argv[0], "led") ) {
		if (argc != 3) {
			trace_printf("bad command, not length of 3""\r\n");
			return;
		}
		if (strcmp(argv[1], "0")) {
			ledn = atoi(argv[1]);
			if (ledn <= 0 || ledn >= 4) {
				trace_printf("bad command, led is number n is not in good form""\r\n");
				return;
			}
		}

		if (strcmp(argv[2], "on") >= 0) {
			set_state(ledn,1);
			return;
		} else if (strcmp(argv[2], "of") >= 0) {
			set_state(ledn,0);
			return;
		} else if (strncmp(argv[2], "f", 1) == 0) {
			set_state(ledn,3);
			return;
		} 
	} else {
		trace_printf("unknown command, type help?""\r\n");
	}
	return;
}

static void display_mem(struct uart_shell *shell, unsigned char *pBfr,
		int bfrLen) {
	int i, j;

	for (i = 0; i < bfrLen; i += 16) {
		ush_printf(shell, "%08x:", (unsigned int) (pBfr + i));
		for (j = i; j < bfrLen && j < i + 16; j++) {
			ush_printf(shell, "%2.2x ", pBfr[j]);
			if (j % 16 == 7) {
				ush_printf(shell, " ");
			}
		}
		ush_printf(shell, " ");
		for (j = i; j < bfrLen && j < i + 16; j++) {
			ush_printf(shell, "%c",
					(pBfr[j] >= 32 && pBfr[j] <= 127) ? pBfr[j] : '.');
		}
		ush_printf(shell, "\r\n");
	}
}

static unsigned int shell_md_address, shell_md_size = 128;
void cmd_d(struct uart_shell *shell, int argc, char *argv[]) {
	if (argc > 1) {
		if (argv[1] != NULL) {
			char *endptr;
			shell_md_address = strtoul(argv[1], &endptr, 0);
			if (shell_md_address == ULONG_MAX || *endptr != '\0') {
				ush_printf(shell, "wrong address:%s!\r\n", argv[1]);
				return;
			}
		}
		if (argv[2] != NULL) {
			char *endptr;
			shell_md_size = strtoul(argv[2], &endptr, 0);
			if (shell_md_size == ULONG_MAX || *endptr != '\0') {
				ush_printf(shell, "wrong size:%s!\r\n", argv[2]);
				return;
			}
		}
	}
	display_mem(shell, (unsigned char*) shell_md_address, (int) shell_md_size);
	shell_md_address += shell_md_size;
}

char board_getc(void);
int board_putc(unsigned char ch);

static void th1() {
	poll_add(led_ready, led_do);
	shell_loop(ush_init(board_getc, board_putc));
}

static void th2() {
	while (1) {
		led_do();
		thread_yield();
	}
}

//main entry
void exp3_b_main() {
	//poll_add(led_ready, led_do);
	//shell_loop(ush_init(board_getc, board_putc));
	threadlib_open();
	//create thread
	thread_create((th_func_t)th1, 0, "shell");
	thread_create((th_func_t)th2, 0, "led");

	thread_waitall();
	threadlib_close();
}


