#ifndef __TICK_H
#define __TICK_H

#include <stdint.h>

void Tick_Init(void);
uint32_t Tick_Get(void);
void Tick_Pause(void);
void Tick_Resume(void);

#endif
