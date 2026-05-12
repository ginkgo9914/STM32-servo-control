# 修改记录

> 日期: 2026-05-08
> 主题: MPU6050 数据传递重构 — 全局变量收敛为结构体

---

## 变更概要

将 MPU6050.c 中 10 个松散的全局变量收敛为一个 `MPU6050_Data` 结构体，ISR 写入、主循环只读，同时将仅 ISR 内部使用的变量改为 `static` 隐藏。

---

## 文件变更明细

### Hardware/MPU6050.h

| 变更 | 内容 |
|------|------|
| +新增 | `MPU6050_Data` 结构体 typedef，含 6 个字段 |
| +新增 | `extern MPU6050_Data mpu;` |
| +新增 | `#include "global.h"` |

**结构体字段：**

| 字段 | 类型 | 说明 |
|------|------|------|
| TimerErrorFlag | uint8_t | 定时器溢出标志 |
| TimerCount | uint16_t | 中断执行时间计数值 |
| AngleAcc | float | X轴加速度计角度 |
| AngleAcc_1 | float | Y轴加速度计角度 |
| Pitch | float | X轴互补滤波角度 |
| Roll | float | Y轴互补滤波角度 |

---

### Hardware/MPU6050.c

| 变更 | 内容 |
|------|------|
| +新增 | `#include "MPU6050.h"` |
| +新增 | `MPU6050_Data mpu;` — 全局输出实例 |
| ~修改 | `AX, AY, AZ, GX, GY, GZ` → `static`，ISR 内部使用 |
| ~修改 | `AngleGyro, AngleGyro_1` → `static`，ISR 内部使用 |
| ~修改 | `AngleAcc` → `mpu.AngleAcc`（ISR 中全部引用） |
| ~修改 | `AngleAcc_1` → `mpu.AngleAcc_1` |
| ~修改 | `Pitch` → `mpu.Pitch` |
| ~修改 | `Roll` → `mpu.Roll` |
| ~修改 | `TimerErrorFlag` → `mpu.TimerErrorFlag` |
| ~修改 | `TimerCount` → `mpu.TimerCount` |
| -删除 | `Pitch_Filtered, Roll_Filtered`（改由 main.c 就地计算） |

---

### User/main.c

| 变更 | 内容 |
|------|------|
| ~修改 | `mpu6050_control_step()` 形参 `()` → `(void)` |
| ~修改 | `Pitch_Filtered` → 局部变量 `pitch_f = (int8_t)(mpu.Pitch + 90)` |
| ~修改 | `Roll_Filtered` → 局部变量 `roll_f = (int8_t)(mpu.Roll + 90)` |
| ~修改 | 所有 OLED 打印参数从裸变量名改为 `mpu.xxx` |

---

## 设计说明

- **原子性**: Cortex-M3 上 float(32-bit) 单次读写为原子操作，无字撕裂风险。不同字段可能来自不同次 ISR 调用，但对 OLED 显示无影响
- **内聚性**: 原始数据(AX~GZ)和中间计算量(AngleGyro)对外不可见，仅 ISR 内部访问
- **可扩展**: 后续添加新输出字段只需扩展结构体，无需新增全局变量
