#include "stm32f10x.h"                  // Device header
#include "global.h"

uint16_t *Timer_number;  // 定义一个全局变量，用于在定时器中断服务程序中使用
void Timer_Init(void){
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 使能TIM2时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 使能GPIOA时钟

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; // 选择PA0引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 设置为上拉模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 设置GPIO速度
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 初始化GPIOA
    // 配置TIM2为外部时钟模式2，使用外部引脚作为时钟源，固定为PA0
    
    TIM_ETRClockMode2Config(
        TIM2, 
        TIM_ExtTRGPSC_OFF, // TIM_ExtTRGPSC_OFF: 不分频
        TIM_ExtTRGPolarity_NonInverted, //  TIM_ExtTRGPolarity_NonInverted: 非反相输入
        0x0f  // 过滤器值 0~15，0表示不使用滤波器，其他值表示滤波器的采样周期数，具体可以参考STM32F10x的参考手册
    ); // 配置TIM2为外部时钟
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    // 72MHZ / 7200 = 10KHz
    // 10KHz / 10000 = 1Hz
    // 计时器溢出频率公式： F = TIM_CLK / ((PSC + 1) * (ARR + 1))
    TIM_TimeBaseStructure.TIM_Period = 10 -1 ; // 自动重装载值 不需要很快
    TIM_TimeBaseStructure.TIM_Prescaler = 1 -1 ; // 预分频值  同理
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 时钟分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式
    // TIM_TimeBaseStructure.TIM_RepetitionCounter = 0; // 重复计数器值 TIM! TIM8专属

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); // 使能TIM2更新中断

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置NVIC优先级分组2
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn; // TIM2中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; // 子优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; // 使能TIM2中断
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE); // 使能TIM2
}

uint16_t Timer_get_counter(void) {
    return TIM_GetCounter(TIM2); // 返回TIM2的当前计数值
}

void Timer_setup_number(uint16_t *num) {
    Timer_number = num; // 将传入的数值赋值给全局变量number
}

void TIM2_IRQHandler(void){
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) { // 检查TIM2更新中断是否发生
        (*Timer_number)++; // 在定时器中断服务程序中对全局变量number地址进行操作，例如自增
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清除TIM2更新中断标志
        // 在这里执行定时器溢出时需要处理的代码
    }
}
