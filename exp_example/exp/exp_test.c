#include "thread.h"
extern int trace_printf(const char *format, ...);
extern void morse_sentence(char * text);
extern void exp2_b_init(void);
extern void exp2_b_do(void);
extern void exp3_b_main(void);
extern void exp4_a_init(void);
extern void exp4_a_do(void);
extern struct ush_cmd* ush_cmd_add(void *func, const char *name, const char *desc);
extern void led_do(void);
struct uart_shell;

//morse "hello world"
void cmd_morse(struct uart_shell *shell, int argc, char *argv[]) {
	(void) shell;
	(void) argc;

	trace_printf("output morse code of %s \r\n", argv[1]);
	morse_sentence(argv[1]);

	return;
}

static void key_thread() {
	exp2_b_init();
	while (1) {
		exp2_b_do();
		thread_yield();
	}
}

static void shell_thread() {
	exp3_b_main();
}

static void led_thread() {
	while (1) {
		led_do();
		thread_yield();
	}
}

static void timeout_thread() {
	exp4_a_init();
	while (1) {
		exp4_a_do();
		thread_yield();
	}
}

//main entry
void exp_main() {
	trace_printf("START EXP TEST\n");
	ush_cmd_add(cmd_morse, "morse", "morse STRING: output morse code of STRING");

	threadlib_open();

	//create thread
	thread_create((th_func_t) shell_thread, 0, "shell");
	thread_create((th_func_t) led_thread, 0, "led_thread");
	thread_create((th_func_t) key_thread, 0, "key_thread");
	thread_create((th_func_t) timeout_thread, 0, "timeout_thread");

	thread_waitall();

	trace_printf("END EXP TEST\n");
}
