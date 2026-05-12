#ifndef __Key_H__
#define __Key_H__
#include "global.h"
#include "Delay.h"
typedef enum {
    key_none = 0,
    key_up = 1,
    key_down = 2,
    key_active = 3,
    // key 上界
    key_count
}KeyReturn;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    KeyReturn key;
} KeyConfig;

void Key_Init(void);
KeyReturn Key_GetReturn(void);

#endif
