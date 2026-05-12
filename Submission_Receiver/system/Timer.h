#ifndef __TIMER_H__
#define __TIMER_H__
void Timer_Init(void);
void Timer_setup_number(uint16_t *num); // 声明一个函数，用于设置全局变量number的值
uint16_t Timer_get_counter(void); // 声明一个函数，用于获取定时器的当前计数值
#endif
