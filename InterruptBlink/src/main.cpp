
#include "stm32l1xx.h"
#include "stm32l1xx_exti.h"
#include "stm32l1xx_syscfg.h"
#include "stm32l100c_discovery.h"

void EXTI0_IRQHandler() {
	if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
		GPIO_ToggleBits(GPIOC, GPIO_Pin_9);
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

int main() {
	SystemInit();
	GPIO_InitTypeDef gpio;
	EXTI_InitTypeDef exti;
	NVIC_InitTypeDef nvic;

	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Pin = GPIO_Pin_0;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gpio);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

	EXTI_StructInit(&exti);
	exti.EXTI_Line = EXTI_Line0;
	exti.EXTI_LineCmd = ENABLE;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&exti);

	nvic.NVIC_IRQChannel = EXTI0_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
	nvic.NVIC_IRQChannelSubPriority = 0x00;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
	while(1);
}
