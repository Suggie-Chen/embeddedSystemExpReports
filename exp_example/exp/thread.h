/*
 * By LJP.
 */
#ifndef	_COROUTINE_H
#define	_COROUTINE_H

#ifdef	__cplusplus
extern "C" {
#endif


typedef struct thread *Thread_ID;
typedef void (*th_func_t)(void*);

Thread_ID thread_create(th_func_t func, void *arg, const char *name);
void thread_switch_to(Thread_ID to);
void thread_yield(void);
void thread_raise(Thread_ID th);
void thread_tick_update(void);
void thread_wait_timeout(Thread_ID th, int delay, int repeat);
void thread_waitall(void);

void threadlib_open(void);
void threadlib_close(void);

#ifdef	__cplusplus
}
#endif

#endif	/* !_COROUTINE_H */
