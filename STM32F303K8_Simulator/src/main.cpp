#include "stm32f30x.h"
#include "stm32f30x_rcc.h"
#include "stm32f30x_gpio.h"
#include <stdint.h>

const uint8_t rom[] = {
		0x07, 0xfd, 0x0b, 0x01, 0x51, 0x38, 0xfd, 0x13,
		0x00, 0x03, 0x00, 0xc1, 0x83, 0x80, 0x0b, 0xc9,
		0x83, 0x80, 0x09, 0x07, 0xff, 0x13, 0x01, 0x62,
		0x38, 0xff, 0xbf, 0x80, 0x07
};

volatile uint8_t ram[512];

typedef struct CPU_State_s {
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;
	uint16_t pc;
	uint8_t cc;
	volatile uint8_t* (*func_to_get_mem)(struct CPU_State_s*, uint16_t);
	uint8_t* rom;
	volatile uint8_t* ram;
} CPU_State;

uint16_t map_pins(uint8_t input) {
	return
		(input&0x01)?0x0001:0|
		(input&0x02)?0x0002:0|
		(input&0x04)?0x0008:0|
		(input&0x08)?0x0010:0|
		(input&0x10)?0x0020:0|
		(input&0x20)?0x0040:0|
		(input&0x40)?0x0080:0|
		(input&0x80)?0x0004:0;
}

uint8_t map_input(uint16_t pins) {
	return
		(pins&0x0001)?0x01:0|
		(pins&0x0002)?0x02:0|
		(pins&0x0008)?0x04:0|
		(pins&0x0010)?0x08:0|
		(pins&0x0020)?0x10:0|
		(pins&0x0040)?0x20:0|
		(pins&0x0080)?0x40:0|
		(pins&0x0004)?0x80:0;
}

void update_pins(CPU_State* state) {
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Pin = map_pins(*(state->func_to_get_mem)(state, 0xfd)); //pa_dr
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Pin = map_pins(~*(state->func_to_get_mem)(state, 0xfd)); //pa_dr
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);

	GPIO_Write(GPIOA, map_pins(*(state->func_to_get_mem)(state, 0xff))); //pa_or

	*(state->func_to_get_mem)(state, 0xfe) = map_input(GPIO_ReadInputData(GPIOA));
}

volatile uint8_t* get_mem(CPU_State* state, uint16_t index) {
	if (index > 0x7fff) {
		return state->rom + index - 0x8000;
	}
	else {
		return state->ram + index;
	}
}

#ifdef ENABLE_CPU_STATE_DEBUG
void print_state(CPU_State* state) {
	printf("A:  %02x\n", state->a);
	printf("X:  %02x\n", state->x);
	printf("Y:  %02x\n", state->y);
	printf("SP: %02x\n", state->sp);
	printf("PC: %04x\n", state->pc);
	printf("CC: %02x\n", state->cc);
}
#endif

void update_cc(CPU_State* state, uint8_t result) {
	if (result) {
		state->cc &= ~0x01;
	}
	else {
		state->cc |= 0x01;
	}
	state->cc |= 0x80;
}

void simulate_step(CPU_State* state) {
	uint8_t instruction = *(state->func_to_get_mem)(state, state->pc);
	uint8_t prefix = (instruction&0xc0)>>6;
	uint8_t pc_increment = 1;
	switch (prefix) {
	case 0: //mov
	{
		uint16_t dest_loc = (instruction&0x38)>>3;
		uint16_t source_loc = (instruction&0x07);
		uint8_t source = 0;
		switch (source_loc&0x03) {
		case 0: source = state->a; //source is a
		break;
		case 1: source = state->x; //source is x
		break;
		case 2: source = state->y; //source is y
		break;
		case 3: source = *(state->func_to_get_mem)(state, state->pc + 1); //source is immediate
			++pc_increment;
		break;
		default: break;
		}
		if (source_loc&0x04) source = *(state->func_to_get_mem)(state, source); //source is indirect
		if (dest_loc&0x04) {
			switch (dest_loc&0x03) {
			case 0: *(state->func_to_get_mem)(state, state->a) = source; break; //dest is indirect a
			case 1: *(state->func_to_get_mem)(state, state->x) = source; break; //dest is indirect x
			case 2: *(state->func_to_get_mem)(state, state->y) = source; break; //dest is indirect y
			case 3: *(state->func_to_get_mem)(state, *(state->func_to_get_mem)(state, state->pc + 1)) = source; ++pc_increment; break; //dest is indirect imm
			default: break;
			}
		}
		else {
			switch (dest_loc&0x03) {
			case 0: state->a = source; break; //dest is a
			case 1: state->x = source; break; //dest is x
			case 2: state->y = source; break; //dest is y
			default: break;
			}
		}
		update_cc(state, source);
	}
	break;
	case 1: //bitop
	{
		uint8_t source = 0;
		uint8_t dest = 0;
		switch (instruction&0x03) {
		case 0: source = state->a; break;
		case 1: source = state->x; break;
		case 2: source = state->y; break;
		default: break;
		}
		switch ((instruction&0x0c)>>2) {
		case 0: dest = state->a; break;
		case 1: dest = state->x; break;
		case 2: dest = state->y; break;
		default: break;
		}
		switch ((instruction&0x30)>>4) {
		case 0: dest &= source; break; //and
		case 1: dest |= source; break; //or
		case 2: dest ^= source; break; //xor
		case 3: dest = ~source; break; //not
		default: break;
		}
		switch ((instruction&0x0c)>>2) {
		case 0: state->a = dest; break;
		case 1: state->x = dest; break;
		case 2: state->y = dest; break;
		default: break;
		}
		update_cc(state, dest);
	}
	break;
	case 2: //jmp
	{
		uint8_t bit = (instruction&0x1c)>>2;
		uint8_t timeToJump = ((state->cc)&(1<<bit))>>bit;
		if (!(instruction&0x20)) timeToJump = !timeToJump;

		if (timeToJump) {
			state->pc = (*(state->func_to_get_mem)(state, state->pc+1)<<8)|(*(state->func_to_get_mem)(state, state->pc+2));
			pc_increment = 0;
		}
		else {
			pc_increment += 2;
		}
	}
	break;
	case 3: //arith, pushpop (not implemented yet)
	{
		switch (instruction&0x30) {
		case 0: break;
		case 1:
		{
			uint8_t source = 0;
			uint8_t dest = 0;
			switch (instruction&0x03) {
			case 0: source = state->a; break;
			case 1: source = state->x; break;
			case 2: source = state->y; break;
			default: break;
			}
			switch ((instruction&0x0c)>>2) {
			case 0: dest = state->a; break;
			case 1: dest = state->x; break;
			case 2: dest = state->y; break;
			default: break;
			}
			switch ((instruction&0x30)>>4) {
			case 0: dest += source; break;
			case 1: dest -= source; break;
			default: break;
			}
			switch ((instruction&0x0c)>>2) {
			case 0: state->a = dest; break;
			case 1: state->x = dest; break;
			case 2: state->y = dest; break;
			default: break;
			}
			update_cc(state, dest);
		}
		break;
		case 2:
		break;
		default:
		break;
		}
	}
	break;
	default: break;
	}
	state->pc += pc_increment;
}

void delay_cyc(unsigned int count) {
	while (--count) {
		asm volatile("nop");
	}
}

int main() {
	CPU_State state;
	int i = 0;
	state.a = 0;
	state.x = 0;
	state.y = 0;
	state.sp = 0;
	state.cc = 0;
	state.pc = 0x8000;
	state.func_to_get_mem = get_mem;
	state.rom = const_cast<uint8_t*>(rom);
	state.ram = ram;
	for (i = 0; i < 512; ++i) {
		ram[i] = 0;
	}
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_DeInit(GPIOA);
	while (1) {
		simulate_step(&state);
		update_pins(&state);
		delay_cyc(10000);
	}
}
