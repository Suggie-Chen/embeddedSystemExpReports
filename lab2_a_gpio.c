/*
 *      Author: ljp
 */

#include "stm32f4xx_hal.h"
#include "stdio.h"

//PF6
#define BUZ1_PORT 5
#define BUZ1_PIN 6

//PF7-10
#define LED6_PORT 5
#define LED6_PIN 7

#define LED7_PORT 5
#define LED7_PIN 8

#define LED8_PORT 5
#define LED8_PIN 9

#define LED9_PORT 5
#define LED9_PIN 10

//PI9
#define KEY3_PORT 8
#define KEY3_PIN 9

//PF11
#define KEY4_PORT 5
#define KEY4_PIN 11

//PC13
#define KEY5_PORT 2
#define KEY5_PIN 13

//PA0
#define KEY6_PORT 0
#define KEY6_PIN 0

#define GPIOx_BASE(port, r) *(volatile unsigned int*)(0x40020000 + (port) * 0x400 + (r) * 4)

#define GPIOx_MODER(x) 		GPIOx_BASE(x, 0)	/*!< GPIO port mode register,               Address offset: 0x00      */
#define GPIOx_OTYPER(x) 	GPIOx_BASE(x, 1)	/*!< GPIO port output type register,        Address offset: 0x04      */
#define GPIOx_OSPEEDR(x)	GPIOx_BASE(x, 2)	/*!< GPIO port output speed register,       Address offset: 0x08      */
#define GPIOx_PUPDR(x)		GPIOx_BASE(x, 3)	/*!< GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
#define GPIOx_IDR(x)		GPIOx_BASE(x, 4) 	/*!< GPIO port input data register,         Address offset: 0x10      */
#define GPIOx_ODR(x)		GPIOx_BASE(x, 5)	/*!< GPIO port output data register,        Address offset: 0x14      */
#define GPIOx_BSRR(x)		GPIOx_BASE(x, 6)	/*!< GPIO port bit set/reset register,      Address offset: 0x18      */

#define RCC_AHB1ENR 		*(volatile unsigned int*)0x40023830	/*!< RCC AHB1 peripheral clock register,                          Address offset: 0x30 */

void buz1_init(void)
{
// #error "UNIMPLEMENTED!"
	GPIOx_MODER(BUZ1_PORT) |= 1 << BUZ1_PIN * 2;
	GPIOx_MODER(BUZ1_PORT) &= ~(1 << (BUZ1_PIN * 2 + 1));
}

void buz1_on(void){
	GPIOx_ODR(BUZ1_PORT) |= 1 << BUZ1_PIN;
}

void buz1_off(void){
	GPIOx_ODR(BUZ1_PORT) &= ~(1 << BUZ1_PIN);
}

void buz1_play(void)
{
	buz1_on();
	HAL_Delay(1000*1);
	buz1_off();
	HAL_Delay(1000*1);
}

void led6_init(void)
{
// #error "UNIMPLEMENTED!"
	GPIOx_MODER(LED6_PORT) |= 1 << LED6_PIN * 2; 
	GPIOx_MODER(LED6_PORT) &= ~(1 << (LED6_PIN * 2 + 1));
}
	
void led6_on(void)
{
	GPIOx_ODR(LED6_PORT) &= ~(1 << LED6_PIN);
}

void led6_off(void)
{
	GPIOx_ODR(LED6_PORT) |= 1 << LED6_PIN;
}

void led6_flash(void)
{
	led6_on();
	HAL_Delay(1000*1);
	led6_off();
	HAL_Delay(1000*1);
}

