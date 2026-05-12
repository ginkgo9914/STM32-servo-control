#include "stm32f10x.h"

#define DELAY_MAX_US 233015  // SysTick 24-bit: 2^24/72 ≈ 233016

static void _delay_chunk(uint32_t us) {
	SysTick->LOAD = 72 * us;
	SysTick->VAL  = 0x00;
	SysTick->CTRL = 0x00000005;
	while (!(SysTick->CTRL & 0x00010000));
	SysTick->CTRL = 0x00000004;
}

void Delay_us(uint32_t xus) {
	while (xus > DELAY_MAX_US) {
		_delay_chunk(DELAY_MAX_US);
		xus -= DELAY_MAX_US;
	}
	if (xus) _delay_chunk(xus);
}

void Delay_ms(uint32_t xms) {
	while (xms--) Delay_us(1000);
}

void Delay_s(uint32_t xs) {
	while (xs--) Delay_ms(1000);
}
