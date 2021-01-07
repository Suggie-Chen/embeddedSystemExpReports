/*
 * By LJP.
 * CoRoutine (Lightweight Thread) implementation on Cortex-M4.
 * Compiler arm-none-eabi-gcc v8.3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread.h"

extern int trace_printf(const char *format, ...);

#define SVC_CALL() __asm volatile ("svc 0")

#define STACK_SIZE 1024	//1024*4bytes
#define MAX_THREAD 10	//how many threads we can create

typedef unsigned int reg_t;
typedef unsigned int stack_t;

typedef struct thread {
	stack_t *context; 			//pointer to current top of stack

	th_func_t func;			//user function entry
	void *arg;					//user function argument

	const char *name;			//name for debug
	unsigned int id;						//which slot
	int run_ctr;					//statstics

	int alive;					//state: alive or dead

	int prio;		//priority. normal is zero. for interrupt backend thread

	int waiting;	//set when wait for timeout, cleared when timeout
	int delay;		//event delay, in milliseconds
	int repeat;	//two type timers: oneshot and periodic. event occurs every 'repeat' milliseconds

	stack_t stack[STACK_SIZE];	//we use static stack
} thread_t;

#define THREAD_RUNNABLE(th) (th && th->alive && !th->waiting)


static thread_t thread_heap[MAX_THREAD];	//save thread data
static Thread_ID main_thread;		//thread 0
static Thread_ID current_thread;	//who is running on cpu
static Thread_ID next_thread;		//who will run next time
static int alive_thread_count;		//how many alive
static unsigned int free_slot_head = 0;	//[0 .. free_slot_head) maybe free, [free_slot_head, max) must be free.
static int svc_nest_count;

typedef struct svc_stack_frame {
	//extra registers saved on thread stack
	reg_t control;
	reg_t exc_return;	//=r14

	reg_t r4;	//Remaining registers saved on thread stack
	reg_t r5;
	reg_t r6;
	reg_t r7;
	reg_t r8;
	reg_t r9;
	reg_t r10;
	reg_t r11;

	reg_t R0;	//Registers stacked as if auto-saved on exception
	reg_t R1;
	reg_t R2;
	reg_t R3;
	reg_t R12;
	reg_t LR;	//LR=r14
	reg_t PC;	//PC=r15
	reg_t xPSR;
} svc_stack_frame_t;


static Thread_ID pick_next(void);

//Save the swapped-out thread's stack ptr, return the to-be swapped-in thread's stack ptr
svc_stack_frame_t* thread_switch_intenal(svc_stack_frame_t *stack_save) {
	svc_nest_count++;
	if (!current_thread || !next_thread) {
		trace_printf("Stack frame on %08X:\n", stack_save);
		trace_printf(" xPSR 	=  %08X\n", stack_save->xPSR);
		trace_printf(" PC 	=  %08X\n", stack_save->PC);
		trace_printf(" LR 	=  %08X\n", stack_save->LR);
		trace_printf(" R0 	=  %08X\n", stack_save->R0);
		trace_printf(" EXC_RETURN =  %08X\n", stack_save->exc_return);
		trace_printf(" CONTROL 	  =  %08X\n", stack_save->control);
		trace_printf(" SVC NO 	  =  %08X\n", ((char*) stack_save->PC)[-2]);
		svc_nest_count--;
		return stack_save;
	}

//	trace_printf("SWITCH %s/%d -> %s/%d\n", current_thread->name,
//			current_thread->alive, next_thread->name, next_thread->alive);
	
	svc_stack_frame_t *stack_restore;
	current_thread->context = (stack_t*) stack_save;
	stack_restore = (svc_stack_frame_t*) next_thread->context;
	next_thread->run_ctr++;
	next_thread->prio = 0;

	current_thread = next_thread;
	svc_nest_count--;
	return stack_restore;
}

#if   defined ( __CC_ARM )
//SVC trap entry. Thread context switching
__asm void SVC_Handler() {
		PRESERVE8
//SAVE CPU CONTEXT
    CPSID   I    //disable interrupt
    tst lr, #4    //which stack we were using?
    ite eq    
    mrseq r0, msp    
    mrsne r0, psp    

			//hareware saved xpsr pc lr r12 r3-r0
    stmdb r0!, {r4-r11}    //software save r11-r4

    mrs r2, control    
    mov r3, lr    
    stmdb r0!, {r2 - r3}    //software save lr & control

    tst lr, #4    //are we on the same stack? ajust sp if yes
    it eq    
    moveq sp, r0    

//call C to help
    bl __cpp(thread_switch_intenal)    //stack is fine, let's call into c

//RESTORE CPU CONTEXT
    ldmia r0!, {r2, r3, r4-r11}    //new stack is in R0, r2=new control, r3=new exc_return
    mov lr, r3    
    msr control, r2    
    ISB    

    tst lr, #4    
    ite eq    
    msreq msp, r0    //return stack = MSP or
    msrne psp, r0    //return stack = PSP

    CPSIE   I    //enable interrupt

    bx LR    //return to new thread from trap. PC & PSR popped out by bx lr
}
#elif defined ( __GNUC__ )
//SVC trap entry. Thread context switching
void thread_svc_handler_asm() {
	__asm volatile (
//SAVE CPU CONTEXT
			" CPSID   I \n"//disable interrupt
			" tst lr, #4 \n"//which stack we were using?
			" ite eq \n"
			" mrseq r0, msp \n"
			" mrsne r0, psp \n"

			//hareware saved xpsr pc lr r12 r3-r0
			" stmdb r0!, {r4-r11} \n"//software save r11-r4

			" mrs r2, control \n"
			" mov r3, lr \n"
			" stmdb r0!, {r2 - r3} \n"//software save lr & control

			" tst lr, #4 \n"//are we on the same stack? ajust sp if yes
			" it eq \n"
			" moveq sp, r0 \n"

//call C to help
			" bl thread_switch_intenal \n"//stack is fine, let's call into c

//RESTORE CPU CONTEXT
			" ldmia r0!, {r2, r3, r4-r11} \n"//new stack is in R0, r2=new control, r3=new exc_return
			" mov lr, r3 \n"
			" msr control, r2 \n"
			" ISB \n"

			" tst lr, #4 \n"
			" ite eq \n"
			" msreq msp, r0 \n"//return stack = MSP or
			" msrne psp, r0 \n"//return stack = PSP


			" CPSIE   I \n"//enable interrupt

			" bx LR \n"//return to new thread from trap. PC & PSR popped out by bx lr
	);
}
#else
  #error Unknown compiler
#endif

static void thread_guard(Thread_ID th);

//fill thread's init stack
static stack_t* prepair_thread_stack(stack_t *stack_top, th_func_t entry,
		void *arg) {
	svc_stack_frame_t *frame = (svc_stack_frame_t*) ((unsigned int) stack_top
			& ~(8 - 1));					//align to double word
	frame--;

	//context saved & restore by hardware exception
	frame->xPSR = 0x01000000u;	//initial xPSR: EPSR.T = 1, Thumb mode
	frame->PC = ((reg_t) entry & ~(2 - 1));	//Entry Point. UNPREDICTABLE if the new PC not halfword aligned
	frame->LR = (unsigned int)-1;		//invalid caller
	frame->R12 = 0x12121212u;
	frame->R3 = 0x03030303u;
	frame->R2 = 0x02020202u;
	frame->R1 = 0x01010101u;
	frame->R0 = (reg_t) arg;

	//context saved & restore by software (svc handler)
	frame->r11 = 0x11111111u;
	frame->r10 = 0x10101010u;
	frame->r9 = 0x09090909u;
	frame->r8 = 0x08080808u;
	frame->r7 = 0x07070707u;
	frame->r6 = 0x06060606u;
	frame->r5 = 0x05050505u;
	frame->r4 = 0x04040404u;

	frame->exc_return = 0xFFFFFFFDu; // initial EXC_RETURN begin state: Thread mode +  non-floating-point + PSP
	frame->control = 0x3;		// initial CONTROL : unprivileged, PSP, no FP

	return (stack_t*) frame;
}

//create a thread
Thread_ID thread_create(th_func_t func, void *arg, const char *name) {
	//find a empty slot
	Thread_ID th = NULL;
	unsigned int i;
	for (i = 0; i < sizeof(thread_heap) / sizeof(thread_heap[0]); i++) {
		if (!thread_heap[i].alive) {
			th = &thread_heap[i];
			if (i >= free_slot_head) {
				free_slot_head = i + 1;
			}
			alive_thread_count++;
			break;
		}
	}
	if (!th) {
		trace_printf("thread_create failed. No free slot\n");
		return NULL;
	}

	//init stack data
	th->func = func;
	th->arg = arg;
	th->name = name;
	th->alive++;
	th->id = i;

	th->run_ctr = 0;
	th->prio = 0;
	th->waiting = 0;
	th->delay = 0;
	th->repeat = 0;

	memset(th->stack, 'E', sizeof(th->stack));

	//create an init stack frame for function to become thread
	stack_t *stack_top = (stack_t*) ((char*) th->stack + sizeof(th->stack));
	th->context = prepair_thread_stack(stack_top, (th_func_t) thread_guard,
			th);

	return th;
}

//Mark thread free
void thread_free(Thread_ID th) {
	th->alive = 0;
	if ((th->id + 1) == free_slot_head) { //update free_slot_head
		free_slot_head = th->id;
	}
	alive_thread_count--;
}

//Switch from current thread to next thread
void thread_switch_to(Thread_ID to) {
	if(svc_nest_count > 0)
		return;
	if (!to || to == current_thread) {
		return;
	}
	if (!THREAD_RUNNABLE(to)) { //if thread is freed, or wait for timeout, just yield cpu to main_thread
		//thread_yield();//bug! thread_yield->thread_switch_to->thread_yield. may stuck in dead loop!
		to = main_thread;
	}

	if (!current_thread) {
		current_thread = next_thread;
	}
	next_thread = to;
	SVC_CALL();
}

//Give up CPU to another thread (main_thread)
void thread_yield() {
	thread_switch_to(pick_next());
}

//Wait thread to exit
void thread_join(Thread_ID th) {
	while (th && th->alive) {
		thread_switch_to(th);
	}
}

//Make this thread prio to schedule. usually used in interrupt
void thread_raise(Thread_ID th) {
	th->prio++;
}

//Update when clock tick hit
void thread_tick_update() {
	unsigned int i;
	for (i = 1; i < free_slot_head; i++) {
		Thread_ID th = &thread_heap[i];
		if (th->alive && th->waiting) {
			if (th->delay == 0) {	//timeout
				th->waiting = 0;	//thread need run
				if (th->repeat) {
					th->delay = th->repeat; //reload if periodic
				}
			} else {
				th->delay--; //count down if not timeout
			}
		}
	}
}

//Repeat: 0-oneshot, 1-periodic time
void thread_wait_timeout(Thread_ID th, int delay, int repeat) {
	th->delay = delay;
	th->repeat = repeat;
	th->waiting++;
	thread_yield();
}

#define MOD(x, y) (x) >= (y) ?  (x) - (y): (x)

static Thread_ID picked_rr = NULL;

//Schedule policy
static Thread_ID pick_next() {
//#ifndef SKIP_ERROR
	//# error "Not Implemented!"
//#endif	
//	unsigned int i;
//	for (i = 0; i < sizeof(thread_heap) / sizeof(thread_heap[0]); i++)
//		if (thread_heap[i].alive && current_thread != &thread_heap[i])
//			return &thread_heap[i];
//	return current_thread;
	static int thread_ptr=0;
	int i=0;
	Thread_ID th=NULL;
	for(i=0;i<MAX_THREAD;i++){
		if (thread_heap[thread_ptr].alive&&thread_heap[thread_ptr].waiting==0) {
			th=thread_heap+thread_ptr;
			i=MAX_THREAD-1;
		}
		thread_ptr=(thread_ptr+1)%MAX_THREAD;
	}

	return th;
}

//Wait all thread to exit
void thread_waitall() {
	Thread_ID th;
	while ((th = pick_next()) && alive_thread_count > 1) {
		thread_switch_to(th);
	}
}

//parent function for monitor
static void thread_guard(Thread_ID th) {
	if (!th) {
		return;
	}
	if (th->func) {
		th->func(th->arg);
	}
	thread_free(th);
	thread_yield();
}

const char *thread_name(Thread_ID th)
{
	return th ? th->name : "null";
}
Thread_ID thread_current(void)
{
	return current_thread;
}
void threadlib_open() {
	picked_rr = 0;
	free_slot_head = 0;
	if (!main_thread) {
		alive_thread_count = 0;
		main_thread = thread_create(0, 0, "main thread");
	}
	current_thread = main_thread;
}

void threadlib_close() {
	thread_free(main_thread);
	current_thread = main_thread = NULL;
}
