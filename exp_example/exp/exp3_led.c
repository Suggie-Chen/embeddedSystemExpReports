/*
 * led operation
 */
#include "stm32f4xx_hal.h"
#include "string.h"

#define MAX_LED 4
#define LED_OFF 0
#define LED_ON 1
#define LED_FLASH_ON 3
static int led_state[MAX_LED];

int led_cmd_exec(int argc, char *argv[]) {
	if ((argc == 3) || (argc == 4)) {
		int ledn = argv[1][0] - '0';
		if (ledn >= MAX_LED)
			ledn = 0;
		if (strcmp(argv[2], "on") >= 0) {
			led_state[ledn] = LED_ON;
			return 0;
		}
		if (strcmp(argv[2], "of") >= 0) {
			led_state[ledn] = LED_OFF;
			return 0;
		}
		if (strcmp(argv[2], "f") >= 0) {
			if (argv[3] && (strcmp(argv[3], "!") == 0)) {
				led_state[ledn] = LED_OFF;
				return 0;
			}
			led_state[ledn] = LED_FLASH_ON;
			return 0;
		}
	}
	return -1;
}

void led_toggle(int n, int level) {
	if(n > 3)
		n = 3;
	HAL_GPIO_WritePin(GPIOF, 1 << (7 + n), level);
}

void led_on(int n)
{
	led_toggle(n, 0);
}
void led_off(int n)
{
	led_toggle(n, 1);
}

void led_do(void) {
	for (int i = 0; i < MAX_LED; i++) {
		if (led_state[i] == LED_ON) {
			led_on(i);
		}
		if (led_state[i] == LED_OFF) {
			led_off(i);
		}
		if (led_state[i] == LED_FLASH_ON) {
			unsigned int ms = HAL_GetTick() % 1000;
			if (ms < 500) {
				led_on(i);
			} else {
				led_off(i);
			}
		}
	}
}
