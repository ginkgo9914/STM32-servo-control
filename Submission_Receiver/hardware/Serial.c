#include "Serial.h"
#include "Tick.h"

// 这是将整数转化为对应的字符的基础大小偏移
#define NUMBER_TO_STRING_BASE_SIZE  0x30 

// 数组接受包
static uint8_t rx_pack[2];

static uint8_t tx_pack[4];
static volatile bool rx_flag;
static char at_rx_buf[64];
static volatile bool at_rx_flag;


/**
 * @brief 串口初始化函数
 *         初始化USART1，配置为发送模式，波特率9600，8位数据位，1位停止位，无校验位
 */
void Serial_Init(void){
    // 使能USART1和GPIOA的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    // 复用推挽输出，
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    // 上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    // 波特率
    USART_InitStructure.USART_BaudRate = 115200;
    // 子长
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    // 停止位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    // 校验位
    USART_InitStructure.USART_Parity = USART_Parity_No;
    // 硬件流控制
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; 
    // 只接收
    USART_InitStructure.USART_Mode = USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);

    // 使能串口
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    // 中断优先级设置为组2，组2表示两位抢占➕两位子优先级
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    // 串口中断通道
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    // 子优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    // 使能
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief 发送一个字节
 * @param byte 要发送的字节
 */
void Serial_send_byte(uint8_t byte){
    // 等待发送完成
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    // 写入数据， TDR
    USART_SendData(USART1, byte);
}

/**
 * @brief 发送一个字节数组
 * @param *array 字节数组指针, uint8_t类型
 * @param len 数组长度
 */
void Serial_send_array(uint8_t *array, uint8_t len){

    for(uint8_t i = 0; i < len; i++){
        Serial_send_byte(array[i]);
    }

}

/**
 * @brief 发送字符串
 * @param *str 字符串指针, uint8_t类型
 */
void Serial_send_string(char *str){
    // 只要不为'/0'就继续发送
    while(*str != '\0'){
        Serial_send_byte(*str++);
    }
}
/**
 * @brief 指数计算函数
 * @param X 底数 uint32_t类型
 * @param Y 指数 uint32_t类型
 */
static uint32_t _pow(uint32_t X, uint32_t Y){
    uint32_t res = 1;
    while(Y--){
        res *= X;
    }
    return res;
}

/**
 * @brief 发送一个数字
 * @param number 要发送的数字
 * @param len 数字长度
 */
void Serial_send_number(uint32_t number, uint32_t len){
    if (number == 0){
        Serial_send_byte('0');
        return;
    }
    for (uint8_t i = 0; i < len; i++){
        Serial_send_byte(number / _pow(10, len - i - 1) % 10 + NUMBER_TO_STRING_BASE_SIZE);
    }
}
/**
 * @brief 快速发送一个数字,不需要指定长度
 * @param number 要发送的数字
 */
void Serial_send_number_quick(uint32_t number){
    if (number == 0){
        Serial_send_byte('0');
        return;
    }
    // 自动计算位数
    uint8_t len = 0;
    uint32_t temp = number;
    // 循环得出大小
    while (temp > 0){
        len++;
        temp /= 10;
    }
    for (uint8_t i = 0; i < len; i++){
        Serial_send_byte(number / _pow(10, len - i - 1) % 10 + NUMBER_TO_STRING_BASE_SIZE);
    }
}

/**
 * @brief 向tx_pack中写入数据
 * @param *data 数据指针, 只会写入前四位
 */
static void Serial_set_pack(uint8_t data[]){
    for (uint8_t i = 0; i < 4; i++){
        tx_pack[i] = data[i];
    }
}

/**
 * @brief 发送一个数据包, 约定格式为: 0xFF, 4个字节, 0xFE
 */
void Serial_send_pack(uint8_t data[], bool override){
    // 是否覆盖数据包
    if(override) Serial_set_pack(data);
    Serial_send_byte(0xFF);
    Serial_send_array(tx_pack, 4);
    Serial_send_byte(0xFE);
}

/**
 * @brief 返回发送数组头地址
 */
uint8_t* Serial_get_tx_pack(){
    return tx_pack;
}

/**
 * @brief 返回接收数组头地址
 */
uint8_t* Serial_get_rx_pack(){
    return rx_pack;
}

/**
 * 以向量的格式返回数据
 */
Vector Serial_get_vector(){
    Vector rx_vector = {.x = rx_pack[0], .y = rx_pack[1]};
    return rx_vector;
}

/**
 * @brief 获取接收标志位, 速度极快
 */
bool Serial_get_rx_flag(void){
    bool flag = rx_flag;
    rx_flag = false;
    return flag;
}

bool Serial_get_at_rx_flag(void){
    bool flag = at_rx_flag;
    at_rx_flag = false;
    return flag;
}

char* Serial_get_at_rx_buf(void){
    return at_rx_buf;
}


// ==================== 双协议状态机接收 ====================
// 协议1: 0xFF + 4字节数据 + 0xFE
#define PACK_HEADER  0xFF
#define PACK_FOOTER  0xFE
#define PACK_LEN     2

// 协议2: '@' + 变长数据(以'\0'结尾) + '\r' + '\n'
#define AT_HEADER    '@'
#define AT_TERM      '\r'
#define AT_END       '\n'

typedef enum {
    STATE_WAIT_HEADER,
    STATE_FF_RECEIVING,
    STATE_FF_WAIT_FOOTER,
    STATE_AT_RECEIVING,
    STATE_AT_WAIT_LF
} RxState;

// 初始化状态为准备接收
static volatile RxState rx_state = STATE_WAIT_HEADER;
// 接收数据缓冲区索引
static volatile uint8_t rx_index;
// 接收数据时间戳
static volatile uint32_t rx_last_byte_tick;

/*
 * USART1 接收中断处理
 *
 * 错误恢复机制（两层防御）:
 *
 * 1. ORE (OverRun Error / 过载错误)
 *    - 触发条件: 接收数据寄存器未读，移位寄存器又完成一次字节接收
 *    - 致命特性: ORE 一旦置位，RXNE 会"粘住"不再触发新中断，接收链路永久卡死
 *    - 恢复方式: 先读 SR 确认 ORE，再读 DR 清除标志，状态机复位丢弃残帧
 *
 * 2. FE/NE (Framing Error / Noise Error / 帧错误与噪声错误)
 *    - 触发条件: 线路上噪声导致采样不一致(NE)，或未检测到有效停止位(FE)
 *    - 危害: 若将错误字节推进状态机（误当帧头/长度/CRC），会导致解析越界或
 *           状态机卡死在中间状态
 *    - 恢复方式: 丢弃错误字节，状态机归零等待下一帧起始
 *
 * 两个处理路径都会刷新 rx_last_byte_tick，保证被丢弃的残帧不会立即触发超时
 * 复位，留出足够时间等待正确帧到来。三者配合使接收在噪声或高吞吐压力下能自
 * 我恢复。
 */
void USART1_IRQHandler(void){
    // if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET) {
    //     uint8_t dummy = USART_ReceiveData(USART1);
    //     (void)dummy;
    //     rx_state = STATE_WAIT_HEADER;
    //     rx_timeout = 0;
    //     return;
    // }

    // if (USART_GetFlagStatus(USART1, USART_FLAG_FE) == SET ||
    //     USART_GetFlagStatus(USART1, USART_FLAG_NE) == SET) {
    //     uint8_t dummy = USART_ReceiveData(USART1);
    //     (void)dummy;
    //     rx_state = STATE_WAIT_HEADER;
    //     rx_timeout = 0;
    //     return;
    // }

    if (USART_GetITStatus(USART1, USART_IT_RXNE) == RESET) return;

    uint8_t byte = USART_ReceiveData(USART1);
    // 更新时间戳
    rx_last_byte_tick = Tick_Get();

    switch (rx_state) {

    case STATE_WAIT_HEADER:
        if (byte == PACK_HEADER) {
            rx_state = STATE_FF_RECEIVING;
            rx_index = 0;
        } else if (byte == AT_HEADER) {
            rx_state = STATE_AT_RECEIVING;
            rx_index = 0;
        }
        break;

    case STATE_FF_RECEIVING:
        rx_pack[rx_index++] = byte;
        if (rx_index >= PACK_LEN) {
            rx_state = STATE_FF_WAIT_FOOTER;
        }
        break;

    case STATE_FF_WAIT_FOOTER:
        if (byte == PACK_FOOTER) {
            rx_flag = true;
        }
        rx_state = STATE_WAIT_HEADER;
        break;

    case STATE_AT_RECEIVING:
        if (byte == AT_TERM) {
            at_rx_buf[rx_index] = '\0';
            rx_state = STATE_AT_WAIT_LF;
        } else if (rx_index < sizeof(at_rx_buf) - 1) {
            at_rx_buf[rx_index++] = byte;
        }
        break;

    case STATE_AT_WAIT_LF:
        if (byte == AT_END) {
            at_rx_flag = true;
        }
        rx_state = STATE_WAIT_HEADER;
        break;
    }
}
/**
 * @brief 状态机超时检测，当状态卡在非等待态超过约100ms时自动复位
 *        需在主循环中定期调用
 */
void Serial_check_timeout(void){
    if (rx_state != STATE_WAIT_HEADER) {
        if ((uint32_t)(Tick_Get() - rx_last_byte_tick) > 100) {
            rx_state = STATE_WAIT_HEADER;
        }
    }
}

/**
 * @brief 发送AT指令，自动添加头尾
 */
void Serial_send_at_cmd(char *cmd){
    Serial_send_byte(AT_HEADER);
    Serial_send_string(cmd);
    Serial_send_byte(AT_TERM);
    Serial_send_byte(AT_END);
}
