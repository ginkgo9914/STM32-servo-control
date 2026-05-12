#include "stm32f10x.h"                  // Device header
#include "Key.h"
#include "Steering.h"
#include "Serial.h"
#include "Tick.h"
#include "stddef.h"

int main(void)
/**
* @主函数
*/
{
	Tick_Init();
	Key_Init();
	steer_init();
	Serial_Init();
	while(true)
	{
		Key_check_return();
		steer_check_bluetooth_signal();
		Serial_check_timeout();
	}
}
