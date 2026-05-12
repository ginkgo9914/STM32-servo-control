#include "stm32f10x.h"                  // Device header
uint16_t count_sensor_number;

void count_sensor_init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	// 中断引脚选择
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructre;
	GPIO_InitStructre.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructre.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructre.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOB, &GPIO_InitStructre);
	// 操作 AFIO
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);
	
	// 配置 EXTI
	EXTI_InitTypeDef EXTI_InitSructure;
	EXTI_InitSructure.EXTI_Line = EXTI_Line14;
	EXTI_InitSructure.EXTI_LineCmd = ENABLE;
	EXTI_InitSructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitSructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitSructure);
	
	// 配置 NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 分组一次
	NVIC_InitTypeDef NVIC_Initstructure;
	NVIC_Initstructure.NVIC_IRQChannel = EXTI15_10_IRQn; 
	NVIC_Initstructure.NVIC_IRQChannelCmd = ENABLE;
	// 抢占
	NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority = 1;
	// 响应
	NVIC_Initstructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_Initstructure);

}
/**
* @brief中断函数，固定名称
*/
void EXTI15_10_IRQHandler(void){
	// 判断进入的位
	if(EXTI_GetITStatus(EXTI_Line14) == SET){
		count_sensor_number++;
		// 清除标志位
		EXTI_ClearITPendingBit(EXTI_Line14);
	}
	
}

uint16_t count_sensor_get(void){
	return count_sensor_number;
}



