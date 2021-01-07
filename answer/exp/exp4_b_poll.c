extern void exp2_b_init(void);
extern void exp3_a_init(void);
extern void exp4_a_init(void);
extern void exp2_b_do(void);
extern void exp3_a_do(void);
extern void exp4_a_do(void);

void exp4_b_init(){
	//add task
//	exp2_a_init();//led
	exp2_b_init();//key+led
	exp3_a_init();//uart+led
	exp4_a_init();//timer+led
}

void exp4_b_do(){
	exp2_b_do();
	exp3_a_do();
	exp4_a_do();//handle timeout
}

/*
 * create send fifo but no recv fifo
 */
int stop_exp4_b;
void exp4_b_poll_main() {
	//init
	exp4_b_init();

	//background main loop
	while(!stop_exp4_b){
		exp4_b_do();
	}
}
