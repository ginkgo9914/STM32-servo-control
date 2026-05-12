#include "global.h"
/**
 * @static 全局使用时，限定为文件私有，防止外部访问。函数内使用时，只初始化一次
 * @brief 通过函数指针数组实现虚函数机制，彻底消灭if-else分支，提升代码的可维护性和扩展性
 */
// 1. 定义函数指针类型 (虚函数签名)
#include "PWM.h" // 包含函数指针类型定义

// 2. 建立注册表 (函数指针数组)
// 索引 0 不用，索引 1 对应通道1，索引 2 对应通道2，以此类推
// 这样数组下标就完美对应了硬件的通道号
// 0 表示未注册，非0表示已注册的函数地址
static Pwm_SetCompare_Func pwm_compare_registry[5] = {0};

// 3. 实现注册功能
void pwm_register_compare_func(uint8_t channel, Pwm_SetCompare_Func func) {
    if (channel >= 1 && channel <= 4) {
        pwm_compare_registry[channel] = func;
    }
}

// 4. 统一的虚函数调用入口
void pwm_set_compare(uint8_t channel, uint16_t compare) {
    // 防御性编程：检查通道号是否越界，检查是否注册过函数
    if (channel >= 1 && channel <= 4 && pwm_compare_registry[channel] != 0) {
        // 【核心】通过数组索引直接调用，相当于虚函数分发
        pwm_compare_registry[channel](compare);
    }
}

// 5. 底层具体的实现
static void _pwm_set_compare1(uint16_t compare) {
    TIM_SetCompare1(TIM2, compare);
}

static void _pwm_set_compare2(uint16_t compare) {
    TIM_SetCompare2(TIM2, compare);
}

static void _pwm_set_compare3(uint16_t compare) {
     TIM_SetCompare3(TIM2, compare); 
    }
static void _pwm_set_compare4(uint16_t compare) {
     TIM_SetCompare4(TIM2, compare); 
    }

// 6. 选择注册范围
void pwm_register_compare_func_by_range(uint8_t min, uint8_t max){
    uint8_t i;
    for (i = min; i <= max; i++) {
        switch (i) {
            case 1: if (pwm_compare_registry[i] == 0) {
                        pwm_register_compare_func(i, _pwm_set_compare1);
                    }
                    break;
            case 2: if (pwm_compare_registry[i] == 0) {
                        pwm_register_compare_func(i, _pwm_set_compare2);
                    }
                    break;
            case 3: if (pwm_compare_registry[i] == 0) {
                        pwm_register_compare_func(i, _pwm_set_compare3);
                    }
                    break;
            case 4: if (pwm_compare_registry[i] == 0) {
                        pwm_register_compare_func(i, _pwm_set_compare4);
                    }
                    break;
            default: break; // 超出范围的通道不注册
        }
    }
}

// 7. 注册指定通道的函数
void pwm_register_compare_func_by_channel(uint8_t channel) {
    switch (channel) {
        case 1: if (pwm_compare_registry[channel] == 0) {
                    pwm_register_compare_func(channel, _pwm_set_compare1);
                }
                break;
        case 2: if (pwm_compare_registry[channel] == 0) {
                    pwm_register_compare_func(channel, _pwm_set_compare2);
                }
                break;
        case 3: if (pwm_compare_registry[channel] == 0) {
                    pwm_register_compare_func(channel, _pwm_set_compare3);
                }
                break;
        case 4: if (pwm_compare_registry[channel] == 0) {
                    pwm_register_compare_func(channel, _pwm_set_compare4);
                }
                break;
        default: break; // 超出范围的通道不注册
    }
}

void pwm_init(void)
{

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 使能TIM2时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
	GPIO_InitTypeDef GPIO_InitStructure;
    // 配置PA1为复用推挽输出，不再依靠输出数据寄存器控制输出电平，而是由定时器控制输出电平
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_InternalClockConfig(TIM2); // 配置TIM2为内部时钟
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    // 72MHZ / 72 = 1MHz
    // 1MKHz / 20000 = 50Hz
    // 计时器溢出频率公式： F = TIM_CLK / ((PSC + 1) * (ARR + 1))
    TIM_TimeBaseStructure.TIM_Period = 20000 -1 ; // ARR自动重装载值 不需要很快
    TIM_TimeBaseStructure.TIM_Prescaler = 72 -1 ; // PSC预分频值  同理
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 时钟分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式
    // TIM_TimeBaseStructure.TIM_RepetitionCounter = 0; // 重复计数器值 TIM! TIM8专属
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_OCInitTypeDef TIM_OCInitStructure;

    TIM_OCStructInit(&TIM_OCInitStructure); // 初始化TIM_OCInitStructure为默认值
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // PWM模式1  
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 使能输出,高电平有效
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 输出比较状态,使能输出
    /**
     * @formula: Duty = CCR / (ARR + 1) * 100%  CCR寄存器的值就是占空比的值
     * @formula: Freq = TIM_CLK / ((PSC + 1) * (ARR + 1)) 计时器溢出频率公式
     * @formula: Reso = 1 / (ARR + 1) 分辨率公式
    */ 
    TIM_OCInitStructure.TIM_Pulse = 0; // CCR寄存器的值就是占空比的值
    TIM_OC1Init(TIM2, &TIM_OCInitStructure); // 初始化TIM2通道1
    TIM_OC2Init(TIM2, &TIM_OCInitStructure); // 初始化TIM2通道2

    TIM_Cmd(TIM2, ENABLE); // 使能TIM2
}




