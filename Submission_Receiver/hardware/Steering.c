#include "PWM.h"
#include "OLED.h"
#include "Steering.h"
#include "Serial.h"
#include "Tick.h"

static Vector current_vector = {.x = 90, .y = 90};
static uint32_t receive_last_received_tick = 0; // 上次接收数据的时间戳
static uint32_t last_display_update_tick = 0; // 上次更新显示的时间戳


static void steer_show_vector(Vector *cmp_vector, Vector *cur_vector);

static void steer_set_pwm_with_vector(const Vector vector);
// 这里可以添加舵机初始化的代码，例如设置PWM频率等
void steer_init(void){
    OLED_Init();
    pwm_init();
    pwm_register_compare_func_by_range(1, 2); // 注册通道1, 2
    steer_set_vector(current_vector);
}

/**
 * @brief 检查蓝牙信号（保持不变）
 */
void steer_check_bluetooth_signal(void){
    if (Serial_get_rx_flag()){
        OLED_ShowChar(1, 16, ' ');
        uint8_t *pack = Serial_get_rx_pack();
        Vector vector = (Vector){.x = pack[0], .y = pack[1]};
        receive_last_received_tick = Tick_Get();
        steer_set_vector(vector);
    }
    else {
        if ((uint32_t)(Tick_Get() - receive_last_received_tick) > 500) { // 超过500ms未接收数据
            // steer_set_vector((Vector){.x = 90, .y = 90}); // 恢复默认位置
            OLED_ShowChar(1, 16, 'F'); // 显示信号丢失
        }
       
    }
    
}

// 静态转换函数（保持不变，公式正确）
static uint16_t _angle_to_compare(double _angle) {
    return (uint16_t)(500 + (_angle * 2000 / 180));
}

/**
 * @brief 设置PWM占空比（保持参数不变）
 */
static void steer_set_pwm_with_vector(const Vector vector) {
    pwm_set_compare(1, vector.x);     
    pwm_set_compare(2, vector.y);          
}

/**
 * @brief 转换向量（参数向量本身保持不变）
 */
static Vector _vector_to_compare(const Vector _vector) {
    return (Vector){.x = _angle_to_compare(_vector.x), .y = _angle_to_compare(_vector.y)};
}

static void check_vector(Vector *vector) {
    if (vector->x < 0) {
        vector->x = 0;
    } else if (vector->x > 180) {
        vector->x = 180;
    }
    if (vector->y < 0) {
        vector->y = 0;
    } else if (vector->y > 180) {
        vector->y = 180;
    }
}

// 设置角度（核心修复）
void steer_set_vector(Vector vector) {
    OLED_Clear();
    check_vector(&vector);
    current_vector = vector;  // 再次更新向量
    Vector cmp_vector = _vector_to_compare(vector); // 计算 CCR

    steer_set_pwm_with_vector(cmp_vector); // 设置 PWM  
    if ((uint32_t)(Tick_Get() - last_display_update_tick) > 50) { // 每50ms更新显示
        last_display_update_tick = Tick_Get();
        steer_show_vector(&cmp_vector, &current_vector); // 显示 CCR 和角度
    }

}

static void steer_show_vector(Vector *cmp_vector, Vector *cur_vector) {
    OLED_ShowString(3, 1, "AngleVector:");            // 显示角度标签
    OLED_ShowString(1, 1, "CCRVector:");              // 显示 CCR 标签
    OLED_ShowString(2, 1, "(    ,    )");            // CCR占位向量外框
    OLED_ShowString(4, 1, "(   ,   )");            // Angle占位向量外框
    OLED_ShowNum(2, 2, cmp_vector->x, 4);
    OLED_ShowNum(2, 7, cmp_vector->y, 4);        
    OLED_ShowNum(4, 2, cur_vector->x, 3); 
    OLED_ShowNum(4, 6, cur_vector->y, 3);     
}

// 获取当前真实角度
Vector steer_get_vector(void) {
    return current_vector;   // 返回角度向量
}

// 加减角度（现在逻辑清晰）
void steer_vector_add(Vector vector) {
    current_vector = TypeExtend_vector_add_vector(current_vector, vector);
    steer_set_vector(current_vector);
}

void steer_vector_sub(Vector vector) {
    current_vector = TypeExtend_vector_sub_vector(current_vector, vector);
    steer_set_vector(current_vector);
}

