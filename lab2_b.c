/*
 *      Author: ljp
 */

#include "stm32f4xx_hal.h"

#include "board.h"

extern int trace_printf(const char *format, ...);

static int led_on = 1;
static int k5_push = 0;
static int led_flash = 0;

#ifdef CONFIG_USE_BOARD_QEMU
//B1-PA0
void key_init(){
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

unsigned int HAL_read_key(int pin) {
	int state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
	//trace_printf("HAL_read_key %d\r\n", state);
	return state;
}

void HAL_CPU_Sleep() {
	trace_printf("HAL_CPU_Sleep %d\r\n", HAL_GetTick());
	static int on = 0;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, (GPIO_PinState)on);
	on  = !on;
	__WFI();
}
void HAL_CPU_Reset() {
	trace_printf("HAL_CPU_Reset %d\r\n", HAL_GetTick());
	static int on = 0;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, (GPIO_PinState)on);
	on  = !on;
	NVIC_SystemReset();
}

void HAL_Led_write(unsigned int howmany, int on) {
//	trace_printf("HAL_Led_on howmany %d time %d\r\n", howmany, HAL_GetTick());
	on &= 1;
	if (howmany >= 1)
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, (GPIO_PinState)on);
}

void HAL_Led_flash(void) {
	if (led_flash) {
		unsigned int ms = HAL_GetTick() % 1000;
		GPIO_PinState on = GPIO_PIN_RESET;
		if (ms <= 500)
			on = GPIO_PIN_RESET;
		else
			on = GPIO_PIN_SET;
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, on);
	}
}

#define K3 0
#define K4 1
#define K5 2
#define K6 3

#else

//#define trace_printf(...) do{}while(0)
//K3-PI9
//K4-PF11
//K5-PC13
//K6-PA0

//×¢ÒâË³Ðò
struct GPIO_Config {
	GPIO_TypeDef *Port;
	unsigned int Pin;
	unsigned int Mode;
	unsigned int Pull;
} gpio_configs[] = {
//FOR FSM4 BOARD
		{ GPIOI, GPIO_PIN_9, GPIO_MODE_INPUT, GPIO_NOPULL },	//K3
		{ GPIOF, GPIO_PIN_11, GPIO_MODE_INPUT, GPIO_NOPULL },	//K4
		{ GPIOC, GPIO_PIN_13, GPIO_MODE_INPUT, GPIO_NOPULL },	//K5
		{ GPIOA, GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_NOPULL }, 	//K6

		{ GPIOF, GPIO_PIN_7, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL },  //LED6
		{ GPIOF, GPIO_PIN_8, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL },  //LED7
		{ GPIOF, GPIO_PIN_9, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL },  //LED8
		{ GPIOF, GPIO_PIN_10, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL }, //LED9

		{ GPIOF, GPIO_PIN_6, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL },	//BUZ1

		//FOR DISCOVERY BOARD
//		{GPIOA, GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_NOPULL},	 //B1
//		{GPIOD, GPIO_PIN_13, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP}, //D3
//		{GPIOD, GPIO_PIN_12, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP}, //D4
//		{GPIOD, GPIO_PIN_14, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP}, //D5
//		{GPIOD, GPIO_PIN_15, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP}  //D6

		};

#define K3 0
#define K4 1
#define K5 2
#define K6 3

#define LED6 4
#define LED7 5
#define LED8 6
#define LED9 7

#define BUZ1 8

#define __HAL_RCC_GPIOx_CLK_ENABLE(x)   do { \
                                        __IO uint32_t tmpreg = 0x00U; \
                                        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN << (x));\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN << (x));\
                                        UNUSED(tmpreg); \
                                          } while(0)

#define PORT_NUM(GPIOx)                 (((unsigned int)(GPIOx) - GPIOA_BASE) / (GPIOB_BASE-GPIOA_BASE))

void key_init() {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	for (unsigned int i = 0; i < sizeof(gpio_configs) / sizeof(gpio_configs[0]);
			i++) {
		__HAL_RCC_GPIOx_CLK_ENABLE(PORT_NUM(gpio_configs[i].Port));
		GPIO_InitStruct.Pin = gpio_configs[i].Pin;
		GPIO_InitStruct.Mode = gpio_configs[i].Mode;
		GPIO_InitStruct.Pull = gpio_configs[i].Pull;
		HAL_GPIO_Init(gpio_configs[i].Port, &GPIO_InitStruct);
	}
}

unsigned int HAL_read_key(int pin) {
	int state = HAL_GPIO_ReadPin(gpio_configs[pin].Port, gpio_configs[pin].Pin);
	//trace_printf("HAL_read_key %d\r\n", state);
	return state;
}

void HAL_CPU_Sleep() {
	if(HAL_GetTick() % 1000 == 0) {
		trace_printf("HAL_CPU_Sleep %d\r\n", HAL_GetTick());
	}
	static int on = 0;
	HAL_GPIO_WritePin(gpio_configs[LED6].Port, gpio_configs[LED6].Pin, (GPIO_PinState)on);
	on  = !on;
	__WFI();
}
void HAL_CPU_Reset() {
	trace_printf("HAL_CPU_Reset %d\r\n", HAL_GetTick());
	static int on = 0;
	HAL_GPIO_WritePin(gpio_configs[LED7].Port, gpio_configs[LED7].Pin, (GPIO_PinState)on);
	on  = !on;
	NVIC_SystemReset();
}

void HAL_Led_write(unsigned int howmany, int on) {
//	trace_printf("HAL_Led_on howmany %d time %d\r\n", howmany, HAL_GetTick());
	on &= 1;
	if (howmany >= 1)
		HAL_GPIO_WritePin(gpio_configs[LED6].Port, gpio_configs[LED6].Pin, (GPIO_PinState)on);
	if (howmany >= 2)
		HAL_GPIO_WritePin(gpio_configs[LED7].Port, gpio_configs[LED7].Pin, (GPIO_PinState)on);
	if (howmany >= 3)
		HAL_GPIO_WritePin(gpio_configs[LED8].Port, gpio_configs[LED8].Pin, (GPIO_PinState)on);
	if (howmany >= 4)
		HAL_GPIO_WritePin(gpio_configs[LED9].Port, gpio_configs[LED9].Pin, (GPIO_PinState)on);
}

void HAL_Led_flash(void) {
	if (led_flash) {
		unsigned int ms = HAL_GetTick() % 1000;
		GPIO_PinState on = GPIO_PIN_RESET;
		if (ms <= 500)
			on = GPIO_PIN_RESET;
		else
			on = GPIO_PIN_SET;
		HAL_GPIO_WritePin(gpio_configs[LED6].Port, gpio_configs[LED6].Pin, on);
		HAL_GPIO_WritePin(gpio_configs[LED7].Port, gpio_configs[LED7].Pin, on);
		HAL_GPIO_WritePin(gpio_configs[LED8].Port, gpio_configs[LED8].Pin, on);
		HAL_GPIO_WritePin(gpio_configs[LED9].Port, gpio_configs[LED9].Pin, on);
	}
}

#endif	//USE_QEMU

#define CHATTER_WINDOW 4	//chatter window

#define MAX_KEY 8	//max key supported

#define NO_KEY 0 //no key pressed
#define PRESS_TIME_MS_SHORT 50	//period for a short push
#define PRESS_TIME_MS_LONG 500	//period for a long push
#define PRESS_TIME_MS_HOLD 500	//period for a push and hold

static struct key_state {
	unsigned int output_level;
	unsigned int output_trig_down;
	unsigned int output_trig_up;

	unsigned int last_read_key;
	unsigned int stable_ctr;
	unsigned int trig_down_ms;
	unsigned int trig_up_ms;
} key_states[MAX_KEY] = { { NO_KEY, 0, 0, NO_KEY, 0, 0, 0 } }; //0 - 3 : K3 - K6

void key_state_init()
{
	for(unsigned int i= 0; i < sizeof(key_states)/sizeof(key_states[0]); i++){
		key_states[i].output_level = 1; //1 means key up
	}
}

//scan gpio pin x (only one pin)
void key_scan(int pin) {
	struct key_state *ks = &key_states[pin % MAX_KEY];
	//reset edge detect
	ks->output_trig_down = ks->output_trig_up = 0;
	//detecting key-pressed
	unsigned int curr_read_key = HAL_read_key(pin);
	if (curr_read_key != ks->last_read_key) {
		ks->stable_ctr = 0;
		ks->last_read_key = curr_read_key;
		return;
	}

	//chattering elimination
	if (++ks->stable_ctr < CHATTER_WINDOW) {
		return;
	}

	ks->stable_ctr = 0;
	ks->last_read_key = curr_read_key;

	//output signal
	ks->output_trig_down = ks->output_level
			& (ks->output_level ^ curr_read_key);
	ks->output_trig_up = curr_read_key & (ks->output_level ^ curr_read_key);
	ks->output_level = curr_read_key;
	if (ks->output_trig_down) {
		ks->trig_down_ms = HAL_GetTick();
	}
	if (ks->output_trig_up) {
		ks->trig_up_ms = HAL_GetTick();
	}
}


void key_process() {
	//detect signal and handle by state
	if (key_states[K3].output_trig_up) {
		struct key_state *ks = &key_states[K3];
		unsigned int presstime = ks->trig_up_ms - ks->trig_down_ms;
		//short push
		if ((PRESS_TIME_MS_SHORT < presstime)
				& (presstime < PRESS_TIME_MS_LONG)) {
			HAL_CPU_Sleep();
		}
	}
	if (key_states[K4].output_trig_up) {
		struct key_state *ks = &key_states[K4];
		unsigned int presstime = ks->trig_up_ms - ks->trig_down_ms;
		//long push
		if (presstime > PRESS_TIME_MS_LONG) {
			HAL_CPU_Reset();
		}
	}
	if (key_states[K5].output_trig_up) {
		struct key_state *ks = &key_states[K5];
		unsigned int presstime = ks->trig_up_ms - ks->trig_down_ms;
		// flash
		if ((PRESS_TIME_MS_SHORT < presstime)
				& (presstime < PRESS_TIME_MS_LONG)) {
			if(!k5_push)
				k5_push = 1;
			else if(k5_push==1){
					while(1){
						led_flash = 1;
						HAL_Led_flash();
						key_scan(K5);
						if(key_states[K5].output_trig_down){
							break;
						}
					}
				}
			}
		else{
			k5_push=0;	
		}
	}
	if (key_states[K6].output_trig_down) {
		struct key_state *ks = &key_states[K6];
		while(1){
			if((HAL_GetTick() - key_states[K6].trig_down_ms) > PRESS_TIME_MS_HOLD*5){
				HAL_Led_write(4, 1);
				break;
			}	
			else
				if((HAL_GetTick() - key_states[K6].trig_down_ms) > PRESS_TIME_MS_HOLD*4)//大于4倍长按时间
					HAL_Led_write(4, 0);
				else//大于3倍长按时间: 
					if((HAL_GetTick() - key_states[K6].trig_down_ms) > PRESS_TIME_MS_HOLD*3)
						HAL_Led_write(3, 0);
					else //大于2倍长按时间
						if((HAL_GetTick() - key_states[K6].trig_down_ms) > PRESS_TIME_MS_HOLD*2)
							HAL_Led_write(2, 0);
						else//大于1倍时间
							if((HAL_GetTick() - key_states[K6].trig_down_ms) > PRESS_TIME_MS_HOLD)
								HAL_Led_write(1, 0);
							else//全灭
								HAL_Led_write(4, 1);
		}
}
}

void lab2_b_init(){
	key_init();
	key_state_init();
}

void lab2_b_do(void) {
	if (HAL_GetTick() % 10 == 0) {
		key_scan(K3);
		key_scan(K4);
		key_scan(K5);
		key_scan(K6);
		key_process();
	}
}

int stop_lab2_b;
void lab2_b_main() {
	lab2_b_init();
	while (!stop_lab2_b) {
		lab2_b_do();
	}
}
