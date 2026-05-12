#include "main.h"
#include <stdbool.h>
#include "MenuStackManager.h"



int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	// 全局 NVIC 分组，仅调用一次

	// 注册所有页面
	MenuStack_Init();	// 内置主菜单

	// 启动时进入主菜单
	MenuStack_Push(Menu_maimmenu);

	while (true) {
		MenuStack_Run();
	}
}
