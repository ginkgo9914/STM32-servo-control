#include "Key.h"
#include "Steering.h"

const static KeyConfig KEY_CONFIGS[2] = {
	{GPIOB, GPIO_Pin_14, key_x_up},
	{GPIOB, GPIO_Pin_13, key_x_down}
};

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉模式，默认为1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
/**
 * @brief 获取按键返回值
 * @details 使用数组遍历按键
 * @return KeyReturn
*/
KeyReturn Key_GetReturn(void)
{
    for (int i = 0; i < sizeof(KEY_CONFIGS) / sizeof(KEY_CONFIGS[0]); i++)
    {
        if (GPIO_ReadInputDataBit(KEY_CONFIGS[i].port, KEY_CONFIGS[i].pin) == 0)
        {
            Delay_ms(20);  // 按下消抖
            while (GPIO_ReadInputDataBit(KEY_CONFIGS[i].port, KEY_CONFIGS[i].pin) == 0);  // 等待松手
            Delay_ms(20);  // 松开消抖
            return KEY_CONFIGS[i].key;
        }
    }
    return key_none;
}

/**
 * @brief 检测按键结果
 */
void Key_check_return(void){
	KeyReturn key_return = Key_GetReturn();
	if(key_return == key_x_up){
		steer_vector_add((Vector){10, 0});
	}
	if(key_return == key_x_down){
		steer_vector_sub((Vector){10, 0});
	}
}
