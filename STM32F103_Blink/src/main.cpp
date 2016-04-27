
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

void delay_cyc(unsigned int count) {
	while (--count) {
		asm volatile("nop");
	}
}

int main() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_13;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio);

	while (1) {
		//GPIO_ToggleBits(GPIOC, GPIO_Pin_9);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		delay_cyc(1000000);
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		delay_cyc(1000000);
	}
}
