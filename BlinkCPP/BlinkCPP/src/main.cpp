/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32l1xx.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_rcc.h"
#include "stm32l100c_discovery.h"

void delay_cyc(unsigned int count) {
	while (--count) {
		asm volatile("nop");
	}
}

int main() {
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOC, &gpio);

	while (1) {
		GPIO_ToggleBits(GPIOC, GPIO_Pin_9);
		GPIO_SetBits(GPIOC, GPIO_Pin_8);
		delay_cyc(1000000);
		GPIO_ResetBits(GPIOC, GPIO_Pin_8);
		delay_cyc(1000000);
	}
}
