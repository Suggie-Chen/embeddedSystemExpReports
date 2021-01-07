/*
 * By LJP.
 */
extern int trace_printf(const char *format, ...);

#define SVC_CALL() __asm volatile ("svc 0")

typedef unsigned int reg_t;

typedef struct svc_stack_frame {
//cpu context after trap
	//extra registers saved on thread stack
	reg_t control;
	reg_t trap_pc;	//=r15
	reg_t exc_return;	//=r14
	reg_t trap_sp;	//=r13
	reg_t trap_xpsr;

//cpu context before trap
	reg_t sp;	//SP=r13
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

static void cpu_state(svc_stack_frame_t *frame) {
	//# error "Not Implemented!"
	if (frame->control==0){
		trace_printf("Privilage\n");
	}
	else trace_printf("Not Privilage\n");
	
	if ((frame->exc_return&(0x0000000F))>>3==0){
		trace_printf("Handler\n");
	}
	else trace_printf("Thread\n");

	if ((frame->exc_return&(0x00000007))>>2==0){
		trace_printf("MSP\n");
	}
	else trace_printf("PSP\n");
}

//print cpu context on trapped
void svc_handler_c(svc_stack_frame_t *frame) {
	trace_printf("\r\nStack frame UP->Down:\n");
	trace_printf(" PSR =  %08X\n", frame->xPSR);
	trace_printf(" PC  =  %08X\n", frame->PC);
	trace_printf(" LR  =  %08X\n", frame->LR);
	trace_printf(" R12 =  %08X\n", frame->R12);
	trace_printf(" R3  =  %08X\n", frame->R3);
	trace_printf(" R2  =  %08X\n", frame->R2);
	trace_printf(" R1  =  %08X\n", frame->R1);
	trace_printf(" R0  =  %08X\n", frame->R0);
	trace_printf("\n");
	trace_printf(" r11 =  %08X\n", frame->r11);
	trace_printf(" r10 =  %08X\n", frame->r10);
	trace_printf(" r9  =  %08X\n", frame->r9);
	trace_printf(" r8  =  %08X\n", frame->r8);
	trace_printf(" r7  =  %08X\n", frame->r7);
	trace_printf(" r6  =  %08X\n", frame->r6);
	trace_printf(" r5  =  %08X\n", frame->r5);
	trace_printf(" r4  =  %08X\n", frame->r4);
	trace_printf(" sp  =  %08X\n", frame->sp);
	trace_printf("\n");
	trace_printf(" trap_xpsr  =  %08X\n", frame->trap_xpsr);
	trace_printf(" trap_sp  =  %08X\n", frame->trap_sp);
	trace_printf(" exc_return  =  %08X\n", frame->exc_return);
	trace_printf(" trap_pc  =  %08X\n", frame->trap_pc);
	trace_printf(" control  =  %08X\n", frame->control);
	cpu_state(frame);
}


//svc trap entry
#if   defined ( __CC_ARM )
__asm void _SVC_Handler() {
		PRESERVE8 
    tst LR, #4    		//which stack we were using?
    ite eq    
    mrseq r0, msp    
    mrsne r0, psp    

			//hareware saved xpsr pc lr r12 r3-r0
    mov r3, r0    //software save r11-r4 & sp
    stmdb r0!, {r3, r4-r11}    

    mrs r2, control    //software save trap changed registers
    mov r3, pc    
    mov r4, lr    
    mov r5, sp    
    mrs r6, psr    
    stmdb r0!, {r2 - r6}    

    tst lr, #4    //are we on the same stack? ajust sp if yes
    it eq    
    moveq sp, r0    

	mov r4, lr
    bl __cpp(svc_handler_c)    //stack is fine, let's call into c
		add sp, #56
		bx r4
}
#elif defined ( __GNUC__ )

void _SVC_Handler() {
	__asm volatile (
			" tst LR, #4 \n"		//which stack we were using?
			" ite eq \n"
			" mrseq r0, msp \n"
			" mrsne r0, psp \n"

			//hareware saved xpsr pc lr r12 r3-r0
			" mov r3, r0 \n"	//software save r11-r4 & sp
			" stmdb r0!, {r3, r4-r11} \n"

			" mrs r2, control \n"//software save trap changed registers
			" mov r3, pc \n"
			" mov r4, lr \n"
			" mov r5, sp \n"
			" mrs r6, psr \n"
			" stmdb r0!, {r2 - r6} \n"

			" tst lr, #4 \n"	//are we on the same stack? ajust sp if yes
			" it eq \n"
			" moveq sp, r0 \n"

			" mov r4, lr \n"	//reserve lr to r4
			" bl svc_handler_c \n"	//stack is fine, let's call into c
			" add sp, #56 \n"		//make sp point to where R0 is saved, ordered by exception return procedure
			" bx r4 \n"			//never return as lr ==exc_return
	);
}
#else
  #error Unknown compiler
#endif


//main entry
void exp1_a_main() {
	SVC_CALL();
}

