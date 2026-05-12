#ifndef __PWM_H__
#define __PWM_H__
#include "stm32f10x.h" // Device header
// 虚函数声明
typedef void (*Pwm_SetCompare_Func)(uint16_t compare);

void pwm_init(void);
void pwm_register_compare_func_by_range(uint8_t min, uint8_t max);
void pwm_set_compare(uint8_t channel, uint16_t compare);
void pwm_register_compare_func_by_channel(uint8_t channel);
#endif
