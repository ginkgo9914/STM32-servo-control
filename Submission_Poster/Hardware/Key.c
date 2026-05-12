#include "Key.h"
#include "Serial.h"

/**
  * @brief 按键配置表
  * 每个条目定义一个按键：GPIO端口、引脚号、按下后返回的键值
  * 新增按键只需在此表追加一行，无需修改检测逻辑
  */
static const KeyConfig KEY_CONFIGS[] = {
    {GPIOB, GPIO_Pin_1, key_up},     // B1 → 上键
    {GPIOA, GPIO_Pin_6, key_down},   // A6 → 下键
    {GPIOA, GPIO_Pin_4, key_active}, // A4 → 确认键
};

/**
  * @brief  按键GPIO初始化
  * @param  无
  * @retval 无
  * @note   将所有按键引脚配置为上拉输入模式
  *         B1 为上键，A6/A4 为下键和确认键
  */
void Key_Init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_4;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
  * @brief  检测并返回按键值（阻塞式）
  * @param  无
  * @retval KeyReturn 按下的按键键值，无按键时返回 key_none
  * @note   遍历 KEY_CONFIGS 表，逐个检测引脚电平。
  *         检测到低电平（按下）后，执行前后各 20ms 的消抖，
  *         并阻塞等待松手，然后返回对应的键值。
  *         多个按键同时按下时，表内靠前的按键优先。
  */
KeyReturn Key_GetReturn(void){
    // 目前等效为 3个Key
    for (int i = 0; i < sizeof(KEY_CONFIGS) / sizeof(KEY_CONFIGS[0]); i++)
    {
        if (GPIO_ReadInputDataBit(KEY_CONFIGS[i].port, KEY_CONFIGS[i].pin) == 0)
        {
            // 按下消抖：用多次短延时替代单次长延时，期间发送心跳包维持蓝牙连接
            for (uint8_t j = 0; j < 10; j++)
            {
                Delay_ms(2);
                Serial_SendPacket();
            }
            // 等待松手：期间持续发送心跳包
            while (GPIO_ReadInputDataBit(KEY_CONFIGS[i].port, KEY_CONFIGS[i].pin) == 0)
            {
                Serial_SendPacket();
                Delay_ms(5);
            }
            // 松开消抖
            for (uint8_t j = 0; j < 10; j++)
            {
                Delay_ms(2);
                Serial_SendPacket();
            }
            return KEY_CONFIGS[i].key;
        }
    }
    return key_none;
}
