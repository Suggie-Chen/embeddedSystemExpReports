#include "thread.h"

/////////////////////////////////////////////////////////
extern int trace_printf(const char *format, ...);

//void template()
//{
//	while(1){
//		//sync condition
//		while(condition_not_ready()){ //data is not avaliable, key is not pressed/released, time is not out, no input on uart, etc
//			thread_yield();
//		}
//		//handle data
//		do_with_data();
//	}
//}

//user function for test
static void foo(Thread_ID *to) {
	int i = 0;
	while (i++ < 1) {
		//ping()
		trace_printf("%s -> %s\r\n", thread_name(thread_current()), thread_name(*to));
		thread_switch_to(*to);
		//pang()
		trace_printf("%s yield\r\n", thread_name(thread_current()));
		thread_yield();
	}
}

//main entry
void exp1_b_main() {
	trace_printf("start exp1_b\r\n");

	threadlib_open();

	Thread_ID th1, th2, th3;
	//create thread
	th1 = thread_create((th_func_t) foo, &th2, "foo 1");
	th2 = thread_create((th_func_t) foo, &th3, "foo 2");
	th3 = thread_create((th_func_t) foo, &th1, "foo 3");

	//start thread
//	thread_switch_to(th1);
//	thread_join(th1);
//	thread_join(th2);
//	thread_join(th3);

	thread_waitall();

	threadlib_close();
	trace_printf("end exp1_b\r\n");
}

