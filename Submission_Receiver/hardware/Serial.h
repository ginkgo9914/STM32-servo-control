#ifndef __SERIAL_H
#define __SERIAL_H


#include <stdbool.h>
#include "global.h"
#include "PrintfRemake.h"
#include "TypeExtend.h"
// #include "stddef.h"
void Serial_Init(void);
void Serial_send_byte(uint8_t byte);
void Serial_send_string(char *str);
void Serial_send_array(uint8_t *array, uint8_t len);
void Serial_send_number(uint32_t number, uint32_t len);
void Serial_send_number_quick(uint32_t number);
bool Serial_get_rx_flag(void);
void Serial_send_pack(uint8_t data[], bool override);
uint8_t* Serial_get_tx_pack(void);
uint8_t* Serial_get_rx_pack(void);
bool Serial_get_at_rx_flag(void);
char* Serial_get_at_rx_buf(void);
void Serial_send_at_cmd(char *cmd);
void Serial_check_timeout(void);
#endif
