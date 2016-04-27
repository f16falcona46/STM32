
#include "stm32l1xx.h"
#include "stm32l1xx_exti.h"
#include "stm32l1xx_syscfg.h"
#include "stm32l100c_discovery.h"

#include <vector>
#include <functional>

volatile static int count = 0;

void delay_cyc(unsigned int count) {
	while (--count) {
		asm volatile("nop");
	}
}

extern "C" {
void EXTI0_IRQHandler() {
	if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
		++count;
		for (int i = 0; i < count; ++i) {
			GPIO_SetBits(GPIOC, GPIO_Pin_9);
			delay_cyc(500000);
			GPIO_ResetBits(GPIOC, GPIO_Pin_9);
			delay_cyc(500000);
		}
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}
}

int main() {
	SystemInit();
	GPIO_InitTypeDef gpio;
	EXTI_InitTypeDef exti;
	NVIC_InitTypeDef nvic;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOC, &gpio);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Pin = GPIO_Pin_0;
	gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
	gpio.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gpio);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

	EXTI_StructInit(&exti);
	exti.EXTI_Line = EXTI_Line0;
	exti.EXTI_LineCmd = ENABLE;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_Init(&exti);

	nvic.NVIC_IRQChannel = EXTI0_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
	nvic.NVIC_IRQChannelSubPriority = 0x00;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	while(1);
}
