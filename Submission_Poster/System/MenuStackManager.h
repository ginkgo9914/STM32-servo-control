#ifndef __MENU_H__
#define __MENU_H__
#include "global.h"
#include "TypeExtend.h"

// 菜单场景类型
typedef enum {
 	Menu_maimmenu = 0,
	Menu_none = 1,
	Menu_adc_control = 2,
	Menu_auto_control = 3,
	Menu_mpu6050_control = 4,
	Menu_count
} MenuIndex;

// 虚函数表：每个菜单页注册三个回调
typedef struct {
	void (*on_enter)(void);
	void (*on_exit)(void);
	void (*on_update)(void);
} MenuPageVTable;

void MenuStack_Init(void);
void Menu_Register(MenuIndex index, MenuPageVTable vtable);
void MenuStack_Push(MenuIndex page);
void MenuStack_Pop(void);
MenuIndex MenuStack_Top(void);
void MenuStack_Run(void);

#endif
