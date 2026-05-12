#ifndef __KEY_H__
#define __KEY_H__
#include "global.h"
#include "Delay.h"

typedef enum {
    key_none = 0,
    key_x_up,
    key_x_down,
    key_y_up,
    key_y_down,
    key_count
} KeyReturn;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    KeyReturn key;
} KeyConfig;

void Key_Init(void);
void Key_check_return(void);

#endif
