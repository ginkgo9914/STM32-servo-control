#include "Serial.h"


/**
 * @brief  printf重定向到串口
 * @param  ch
 * @param  f
 * @return int
 */
int fputc(int ch, FILE *f){
    Serial_send_byte(ch);
    return ch;
}
/**
 * @brief  printf重定向到串口，带参数
 * @param  fmt
 * @param  args
 * @return 无
 */
void Serial_printf(const char *fmt, ...){
    char string[100]; // 定义一个字符数组，用于存储格式化后的字符串
    va_list args;  // 定义一个va_list类型的变量，用于存储可变参数
    va_start(args, fmt);  // 初始化args，使其指向可变参数列表的第一个参数
    vsprintf(string, fmt, args);  // 使用vsprintf函数将可变参数格式化为字符串，并存储到string数组中
    va_end(args);  // 清理args
    Serial_send_string(string);  // 将格式化后的字符串发送到串口
}


