# 串口超时检测修复分析

## 原始问题

`Serial_check_timeout()` 使用主循环迭代次数来模拟时间，注释标注"72MHz主循环频率下约100ms超时"，但存在两层错误：

### 1. 注释自身数学错误

720000 / 72000000 = **10ms**，不是 100ms。注释本身就差了 10 倍。

### 2. 主循环根本达不到 72MHz（致命问题）

主循环每轮执行：

| 调用 | 操作 | 耗时 |
|------|------|------|
| `Key_check_return()` | 无按键：读 GPIO 两次 | ~1µs |
| | 有按键：`Delay_ms(20)` × 2 + 等松手 | **阻塞 40ms+** |
| `steer_check_bluetooth_signal()` | 收到数据：`steer_set_vector()` → OLED 清屏 + 多行显示 | 数百 µs ~ 1ms |
| | 无数据：`OLED_ShowChar(1, 16, 'F')` 写一个字符 | 50~100µs |
| `Serial_check_timeout()` | 几条指令 | <1µs |

常态下（无按键、无蓝牙数据），单次循环约 **50~100µs**（受限于 I2C OLED），相当于 **10kHz~20kHz**。

以 10kHz 计算：**720000 / 10000 = 72 秒**。注释期望的 100ms 超时，实际生效时间超过一分钟——状态机卡在半路上基本得不到及时恢复。

## 修复方案

### 新模块：`system/Tick.h` + `system/Tick.c`

基于 **TIM3** 硬件定时器提供 1ms 精度的时间基准。

```
HCLK = 72MHz → APB1 Timer Clock = 72MHz
PSC = 72-1   →  72MHz / 72 = 1MHz
ARR = 1000-1 →  1MHz / 1000 = 1kHz = 1ms 溢出周期
```

选择 TIM3 的原因：
- TIM2 已被 PWM（舵机）占用
- SysTick 被 `Delay_us`/`Delay_ms` 以阻塞方式占用（每次调用都重写 LOAD/VAL/CTRL 寄存器并停止定时器），无法同时用作周期性中断
- TIM3 完全空闲，不与任何现有外设冲突

### 超时逻辑改动（`hardware/Serial.c`）

**ISR 中：**
```c
// 旧：每次收到字节时清零循环计数器
rx_timeout = 0;

// 新：每次收到字节时记录当前毫秒时间戳
rx_last_byte_tick = Tick_Get();
```

**超时检测函数：**
```c
// 旧：依赖主循环迭代次数的假超时
void Serial_check_timeout(void){
    if (rx_state != STATE_WAIT_HEADER) {
        rx_timeout++;
        if (rx_timeout > 720000) {  // 不可靠
            rx_state = STATE_WAIT_HEADER;
            rx_timeout = 0;
        }
    }
}

// 新：基于硬件定时器的真超时
void Serial_check_timeout(void){
    if (rx_state != STATE_WAIT_HEADER) {
        if ((uint32_t)(Tick_Get() - rx_last_byte_tick) > 100) {
            rx_state = STATE_WAIT_HEADER;
        }
    }
}
```

`(uint32_t)(current - start) > 100` 的写法天然处理了 32 位计数器回绕（约 49 天一次），无需额外判断。

### 初始化顺序（`user/main.c`）

```c
Tick_Init();    // 最先初始化，确保 ISR 最早拿到时间基准
Key_Init();
steer_init();
Serial_Init();
```

## 改动清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `system/Tick.h` | 新增 | 声明 `Tick_Init()` / `Tick_Get()` |
| `system/Tick.c` | 新增 | TIM3 1ms 中断 + 毫秒计数器 |
| `hardware/Serial.c` | 修改 | 超时检测改用 `Tick_Get()` |
| `user/main.c` | 修改 | 新增 `Tick_Init()` 调用 |

## 关键设计决策

**为什么不用 SysTick？** `Delay_us()` 每次调用都会重写 SysTick 的 LOAD、VAL、CTRL 寄存器，并在结束时停止定时器（`CTRL = 0x00000004`）。若在 `Delay_ms(20)` 执行期间 SysTick 被重新配置为 1ms 周期中断，Delay 函数和 Tick 计数器会互相破坏。保留 SysTick 给 Delay、复用 TIM3 给 Tick，两者完全隔离。

**为什么不用 TIM2？** TIM2 已被 `pwm_init()` 配置为 PWM 输出（舵机控制，50Hz，通道 1/2），无法同时用作 1ms 中断源。
