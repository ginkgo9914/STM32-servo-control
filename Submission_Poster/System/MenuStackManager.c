#include "MenuStackManager.h"
#include <stdbool.h>
#include "OLED.h"
#include "Key.h"
#include "AD.h"
#include "Timer.h"
#include "MPU6050.h"
#include "Serial.h"
#include "Delay.h"

#include "Tick.h"


// 所有其它菜单均可以按照此协议
/* ---- 子页面声明 ---- */
static void adc_enter(void);
static void adc_exit(void);
static void adc_update(void);

static void auto_enter(void);
static void auto_exit(void);
static void auto_update(void);

static void mpu6050_enter(void);
static void mpu6050_exit(void);
static void mpu6050_update(void);

// 工具函数声明

static void _show_string_with_vector(Vector vector, char *string);
static void _reverse_area_with_index(uint8_t flag);
static void _auto_show_all(void);

// 最大栈深度
#define STACK_SIZE 4

// ============== 自动模式相关 ===============
#define LeftBottom {0, 0}
#define MidBottom {90, 0}
#define RightBottom {180, 0}
#define MidRight {180, 90}
#define RightTop {180, 180}
#define MidTop {90, 180}
#define LeftTop {0, 180}
#define MidLeft {0, 90}

const Vector AUTO_PHASES[8] = {
	LeftBottom, MidBottom, RightBottom, MidRight,
	RightTop, MidTop, LeftTop, MidLeft
};

#define AUTO_INTERVAL (int)300  // 自动模式切换间隔

int auto_index = 0; // 自动模式索引
int last_switch_tick = 0;  // 上次切换时间戳



// 注册表
static MenuPageVTable page_vtables[Menu_count];

// 栈
static MenuIndex stack[STACK_SIZE];
// 顶栈
static int8_t top = -1;

#define INITIAL_MENU_INDEX 2  // 初始进入主菜单时选中第几行（2~4）

// ====== 主菜单页面（内部实现） ======
static uint8_t menu_index = INITIAL_MENU_INDEX;  // 表示当前选择第几行



static void _mainmenu_enter(void) {
	menu_index = INITIAL_MENU_INDEX;
	OLED_Clear();
	_auto_show_all();
	_reverse_area_with_index(menu_index);
	OLED_Update();
}

static void _mainmenu_exit(void) {
	// 主菜单不需要清理
	OLED_Clear();
}

static void _mainmenu_update(void) {
	KeyReturn key = Key_GetReturn();
	uint8_t prev_index = menu_index;
	if (key == key_up) {
		menu_index--;
		if (menu_index <= 1) menu_index = 4;
	}
	if (key == key_down) {
		menu_index++;
		if (menu_index >= 5) menu_index = 2;
	}
	if (key == key_active) {
		MenuStack_Push((MenuIndex)menu_index);
		return;
	}
	if (menu_index != prev_index) {
		_reverse_area_with_index(prev_index);	// 恢复旧行
		_reverse_area_with_index(menu_index);	// 高亮新行
	}
	Serial_SendPacket();	// 心跳包, 维持串口连接
	OLED_Update();
}


// ====== 栈操作 ======
/**
 * @brief 初始化菜单系统，注册所有页面
 */
void MenuStack_Init(void) {
	Menu_Register(Menu_maimmenu,
		(MenuPageVTable){_mainmenu_enter, _mainmenu_exit, _mainmenu_update});
	Menu_Register(Menu_adc_control,
		(MenuPageVTable){adc_enter, adc_exit, adc_update});
	Menu_Register(Menu_auto_control,
		(MenuPageVTable){auto_enter, auto_exit, auto_update});
	Menu_Register(Menu_mpu6050_control,
		(MenuPageVTable){mpu6050_enter, mpu6050_exit, mpu6050_update});
	// 所有初始化函数
	Tick_Init();
	Tick_Pause();  // 暂停计时，等进入页面后再 Resume
	Key_Init();
	Serial_Init();
	OLED_Init();
	Timer_Init(2);
}

/**
 * @brief 注册页面
 */
void Menu_Register(MenuIndex index, MenuPageVTable vtable) {
	page_vtables[index] = vtable;
}


/**
 * @brief 进入页面
 */
void MenuStack_Push(MenuIndex page) {
	if (top >= STACK_SIZE - 1) return;
	stack[++top] = page;
	if (page_vtables[page].on_enter)
		page_vtables[page].on_enter();
}


/**
 * @brief 退出页面, 返回上一页
 */
void MenuStack_Pop(void) {
	if (top < 0) return;
	MenuIndex leaving = stack[top];
	if (page_vtables[leaving].on_exit)
		page_vtables[leaving].on_exit();
	top--;
	if (top >= 0) {
		MenuIndex returning = stack[top];
		if (page_vtables[returning].on_enter)
			page_vtables[returning].on_enter();
	}
}

/**
 * @brief 获取当前页面索引
 */
MenuIndex MenuStack_Top(void) {
	return (top >= 0) ? stack[top] : Menu_none;
}

/**
 * @brief 运行当前页面的更新函数
 */
void MenuStack_Run(void) {
	if (top < 0) return;
	MenuPageVTable *current = &page_vtables[stack[top]];
	if (current->on_update)
		current->on_update();
}

// ====== 绘制工具函数 ======

/**
 * @brief 自动根据索引翻转对应区域
 * @param index 索引
 */
static void _reverse_area_with_index(uint8_t index) {
	Vector vector_LT = { .x = 0, .y = (index - 1) * 16 };
	Vector vector_RB = { .x = 128, .y = 16 };
	OLED_ReverseArea(vector_LT.x, vector_LT.y, vector_RB.x, vector_RB.y);
}

/**
 * @brief 在指定位置显示字符串
 * @param vector 字符串显示位置
 * @param string 字符串
 */
static void _show_string_with_vector(Vector vector, char *string) {
	OLED_ShowString(vector.x, vector.y, string, OLED_8X16);
}

/**
 * @brief 自动显示所有菜单
 */
static void _auto_show_all(void) {
	_show_string_with_vector((Vector){0, 0},  "Mode Selection             ");
	_show_string_with_vector((Vector){0, 16}, "ADC-Joystick                  ");
	_show_string_with_vector((Vector){0, 32}, "One-touch start           ");
	_show_string_with_vector((Vector){0, 48}, "MPU6050                   ");
}

/* ================================================================
   ADC 摇杆页面
   ================================================================ */
static void adc_enter(void) {
	OLED_Clear();  // 清除上一页残留
	AD_Init();
}

static void adc_exit(void) {
	// ADC 无需额外关闭
}

static void adc_update(void) {
	if (Key_GetReturn() == key_active) {
		OLED_Clear();
		OLED_Update();
		MenuStack_Pop();
		return;
	}

	Vector angle_vector = AD_get_angle_with_vector();
	Serial_TxPacket[0] = angle_vector.x;
	Serial_TxPacket[1] = angle_vector.y;
	Serial_SendPacket();

	OLED_Printf(0, 0, OLED_8X16, "X:%3d", angle_vector.x);
	OLED_Printf(64, 0, OLED_8X16, "Y:%3d", angle_vector.y);
	OLED_Update();
}

/* ================================================================
   One-touch start 页面（占位）
   ================================================================ */

static void auto_enter(void) {
	OLED_Clear(); 
	Tick_Resume();  // 恢复计时，开始自动切换
}

static void auto_exit(void) {
	auto_index = 0;  // 重置索引
	last_switch_tick = Tick_Get();  // 重置计时
	Tick_Pause();  // 暂停计时，等进入页面后再 Resume
}

static void auto_update(void) {
	if (Key_GetReturn() == key_active) {
		OLED_Clear();
		OLED_Update();
		MenuStack_Pop();
		return;
		}
	
	if (Tick_Get() - last_switch_tick >= AUTO_INTERVAL) {
		last_switch_tick = Tick_Get();
		auto_index++;
		if (auto_index >= sizeof(AUTO_PHASES) / sizeof(AUTO_PHASES[0])) auto_index = 0;
	}
	_show_string_with_vector((Vector){0, 0}, "Vector(   ,   )");

	Serial_TxPacket[0] = AUTO_PHASES[auto_index].x;
	Serial_TxPacket[1] = AUTO_PHASES[auto_index].y;
	Serial_SendPacket();

	OLED_Printf(56, 0, OLED_8X16, "%3d", AUTO_PHASES[auto_index].x);
	OLED_Printf(88, 0, OLED_8X16, "%3d", AUTO_PHASES[auto_index].y);
	OLED_Update();
}

/* ================================================================
   MPU6050 页面
   ================================================================ */
static void mpu6050_enter(void) {
	MPU6050_Init();
	Timer_Init(1);
	OLED_Clear(); 
}

static void mpu6050_exit(void) {
	// 停掉定时器，防止退出后还在更新 mpu 数据
	Timer_Stop(1);
}

static void mpu6050_update(void) {
	if (Key_GetReturn() == key_active) {
		OLED_Clear();
		OLED_Update();
		MenuStack_Pop();
		return;
	}

	int8_t pitch_f = (int8_t)(mpu.Pitch + 90);
	int8_t roll_f  = (int8_t)(mpu.Roll  + 90);
	Serial_TxPacket[0] = pitch_f;
	Serial_TxPacket[1] = roll_f;
	Serial_SendPacket();
	OLED_Printf(0, 0, OLED_8X16, "Flag:%1d", mpu.TimerErrorFlag);
	OLED_Printf(64, 0, OLED_8X16, "C:%05d", mpu.TimerCount);
	OLED_Printf(0, 16, OLED_8X16, "%+06.1f", mpu.AngleAcc);
	OLED_Printf(64, 16, OLED_8X16, "%+06.1f", mpu.Pitch);
	OLED_Printf(0, 32, OLED_8X16, "%+06.1f", mpu.AngleAcc_1);
	OLED_Printf(64, 32, OLED_8X16, "%+06.1f", mpu.Roll);
	OLED_Printf(0, 48, OLED_8X16, "%+06d", pitch_f);
	OLED_Printf(64, 48, OLED_8X16, "%+06d", roll_f);
	OLED_Update();
}
