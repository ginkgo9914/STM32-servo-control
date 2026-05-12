# C 语言面向对象设计方法总结

## 概述

本项目运行于 STM32F10x 平台（Cortex-M3，72MHz，RAM 20KB），无法使用 C++。但通过几组 C 语言手法，达成了封装、多态、命名空间等面向对象的核心收益，代码维护性和扩展性显著优于裸过程式写法。

---

## 手法一：文件级封装（private 模拟）

### 原理

`static` 关键字在 `.c` 文件作用域下，将变量/函数的可见性限制在当前编译单元。外部只能通过 `.h` 中声明的公开函数间接访问。

### 项目实例：Steering.c

```c
// 私有成员变量 —— 外部完全不可见
static Vector current_vector = {.x = 90, .y = 90};
static uint32_t receive_last_received_tick = 0;
static uint32_t last_display_update_tick = 0;

// 私有方法 —— 只在文件内可调用
static void steer_show_vector(Vector *cmp_vector, Vector *cur_vector);
static void steer_set_pwm_with_vector(const Vector vector);
static uint16_t _angle_to_compare(double _angle);
static Vector _vector_to_compare(const Vector _vector);
static void check_vector(Vector *vector);

// 公开 API —— .h 中声明，模块外部调用
void steer_init(void);
void steer_set_vector(Vector vector);
Vector steer_get_vector(void);
void steer_vector_add(Vector vector);
void steer_vector_sub(Vector vector);
```

| C 手法 | OOP 等价 |
|---|---|
| `static` 变量 | `private` 成员字段 |
| `static` 函数 | `private` 方法 |
| `.h` 中声明的非 static 函数 | `public` 方法 |

---

## 手法二：虚函数表（多态模拟）

### 原理

用**全局函数指针数组**作为虚函数表，数组索引对应硬件通道号。调用时通过数组间接跳转，不在运行时做任何 if-else / switch 分支判断。

### 项目实例：PWM.c

```
PWM.h:5        typedef void (*Pwm_SetCompare_Func)(uint16_t compare);   // 虚函数签名
PWM.c:13       static Pwm_SetCompare_Func pwm_compare_registry[5] = {0}; // vtable（索引0空置）
PWM.c:16-19    pwm_register_compare_func(channel, func)   // 注册 → override
PWM.c:23-29    pwm_compare_registry[channel](compare)     // 分发 → virtual call
PWM.c:32-45    _pwm_set_compare1/2/3/4                   // 具体实现 → private override
```

调用路径：

```
pwm_set_compare(2, 1500)
  → pwm_compare_registry[2](1500)      // 数组索引 = O(1) 跳转
    → _pwm_set_compare2(1500)          // TIM_SetCompare2(TIM2, 1500)
```

### 与 C++ 的对应关系

| C 手法 | C++ 等价 |
|---|---|
| `Pwm_SetCompare_Func` typedef | 虚函数签名 `virtual void setCompare(uint16_t) = 0` |
| `pwm_compare_registry[]` | 编译器生成的 vtable |
| `pwm_register_compare_func()` | 子类 `override` 注册 |
| 数组下标直接跳转 | 虚函数调用的间接分支 |

### 优势

- **消灭分支**：如果写成 `switch(channel) { case 1: ... case 2: ... }`，每个调用点都是一次分支预测风险。全局注册表只在初始化阶段 branch，热路径直接跳转
- **开闭原则**：新增通道只需写一个新的 `_pwm_set_compareN` 并注册，不修改分发逻辑

---

## 手法三：自指针对 `this` 模拟

### 原理

每个方法的第一参数是 `StructName *self`，调用时显式传入实例指针。这是 C 语言 OOP 中最核心的手法——**没有语法糖，但语义完整**。

### 设计示例：舵机对象

#### 每实例方法指针（完整版）

```c
// servo.h
typedef struct Servo Servo;

struct Servo {
    // 属性
    uint8_t  channel;
    uint16_t min_ccr;       // 0°  对应 CCR
    uint16_t max_ccr;       // 180° 对应 CCR
    int16_t  angle;         // 当前角度
    bool     inverted;      // 是否反装

    // 方法（函数指针 —— 每个实例持有自己的方法表）
    void   (*set_angle)(Servo *self, int16_t angle);
    int16_t(*get_angle)(Servo *self);
    void   (*set_range)(Servo *self, uint16_t min, uint16_t max);
    void   (*center)(Servo *self);
};

void Servo_Init(Servo *self, uint8_t channel);
void Servo_InitEx(Servo *self, uint8_t channel, uint16_t min, uint16_t max, bool inverted);
```

```c
// servo.c
static void _set_angle(Servo *self, int16_t angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    self->angle = angle;
    uint16_t ccr = self->min_ccr
        + (uint16_t)((uint32_t)angle * (self->max_ccr - self->min_ccr) / 180);
    if (self->inverted) ccr = self->max_ccr - (ccr - self->min_ccr);
    pwm_set_compare(self->channel, ccr);   // 复用现有 vtable 分发
}

static int16_t _get_angle(Servo *self) { return self->angle; }
static void _set_range(Servo *self, uint16_t min, uint16_t max) {
    self->min_ccr = min; self->max_ccr = max;
}
static void _center(Servo *self) { self->set_angle(self, 90); }

void Servo_Init(Servo *self, uint8_t channel) {
    self->channel   = channel;
    self->min_ccr   = 500;
    self->max_ccr   = 2500;
    self->angle     = 90;
    self->inverted  = false;

    self->set_angle = _set_angle;    // 构造函数中绑定方法
    self->get_angle = _get_angle;
    self->set_range = _set_range;
    self->center    = _center;

    pwm_register_compare_func_by_channel(channel);
}
```

#### 调用方式

```c
Servo pan, tilt;

Servo_Init(&pan, 1);          // 构造函数
Servo_InitEx(&tilt, 2, 600, 2400, false);

pan.set_angle(&pan, 45);      // pan 转到 45°
tilt.center(&tilt);           // tilt 归中

// pan.set_angle 等同于 C++ 的 pan->setAngle(45)
// &pan 就是 this
```

### 内存优化版：共享方法表

当舵机数量多（≥6 个）时，每个实例 4 个函数指针 = 16 字节的冗余（所有实例的函数实现相同）。改用共享 vtable：

```c
typedef struct {
    void   (*set_angle)(Servo *self, int16_t angle);
    int16_t(*get_angle)(Servo *self);
    void   (*set_range)(Servo *self, uint16_t min, uint16_t max);
    void   (*center)(Servo *self);
} ServoVTable;

static const ServoVTable servo_vtable = {
    _set_angle, _get_angle, _set_range, _center
};

struct Servo {
    const ServoVTable *vptr;   // 4 字节，指向共享表
    uint8_t  channel;
    uint16_t min_ccr, max_ccr;
    int16_t  angle;
    bool     inverted;
};
```

| 方案 | 每实例方法指针开销 | 12 个舵机总开销 | 调用语法 |
|---|---|---|---|
| 每实例指针 | 16 字节（4 个 funptr） | 192 字节 | `s->set_angle(s, a)` |
| 共享 vtable | 4 字节（1 个 vptr） | 48 字节 | `s->vptr->set_angle(s, a)` |

对于 STM32F1 仅 20KB 的 RAM，12 个舵机省 144 字节是值得的。调用时的额外一次解引用在现代 Cortex-M3 上仅多 1 个 load 指令，可忽略。

---

## 手法四：管理器组合模式

### 原理

创建一个管理器结构体，持有多个子对象的指针，提供批量操作接口。

```c
// ServoManager.h
typedef struct {
    Servo *servos[16];
    uint8_t count;
} ServoManager;

void ServoManager_Init(ServoManager *self);
void ServoManager_Add(ServoManager *self, Servo *servo);
void ServoManager_SetAll(ServoManager *self, int16_t angle);
void ServoManager_CenterAll(ServoManager *self);
Servo* ServoManager_Get(ServoManager *self, uint8_t index);
```

```c
// 使用
ServoManager mgr;
ServoManager_Init(&mgr);
ServoManager_Add(&mgr, &pan);
ServoManager_Add(&mgr, &tilt);
ServoManager_Add(&mgr, &gripper);

ServoManager_CenterAll(&mgr);  // 一键全部归中
```

管理器不拥有 servo 内存（只存指针），servo 可以静态分配在栈上或全局区，安排灵活，不引入动态内存分配。

---

## 手法五：值对象 + 命名空间前缀

### 原理

用 `typedef struct` 定义纯数据载体，用 `ModuleName_action()` 前缀模拟命名空间方法。

### 项目实例：TypeExtend

```c
// TypeExtend.h
typedef struct vector { int16_t x; int16_t y; } Vector;   // 值对象
Vector TypeExtend_vector_add_vector(Vector v1, Vector v2); // 命名空间方法
Vector TypeExtend_vector_sub_vector(Vector v1, Vector v2);
```

```c
// 调用
Vector result = TypeExtend_vector_add_vector(v1, v2);
//               ^^^^^^^^^         ^^^  ^^^  ^^
//               命名空间          方法名 参数  参数
```

Vector 在模块间自由传递（值语义），不需要指针也不需要考虑所有权，适合用于角度、坐标、速度等小数据。

---

## 四种手法对照

| 手法 | 解决的问题 | 在项目中的位置 | 适用场景 |
|---|---|---|---|
| 文件级封装 (`static`) | 数据隐藏、接口隔离 | [Steering.c](../hardware/Steering.c) | 所有模块 |
| 虚函数表 (全局注册表) | 多态、消除分支 | [PWM.c](../hardware/PWM.c) | 底层硬件抽象 |
| 自指针 `self` | this 模拟、实例方法 | Servo 示例 | 多实例对象 |
| 值对象 + 前缀 | 数据载体、命名空间 | [TypeExtend](../utils/TypeExtend.h) | 基础类型扩展 |

---

## 设计决策指南

### 何时用全局注册表（PWM 模式）

- 只有**一个维度**在变化（如通道号）
- 实例数量**固定**且有限
- 需要**极致性能**（数组索引比 struct 成员指针少一次解引用）

### 何时用自指针 `self`（Servo 模式）

- 每个实例有**多个独立属性**（通道、行程、反装标志）
- 实例数量**运行时决定**
- 需要**每实例配置不同行为**（不同类型舵机的方法表可以不同）

### 何时用共享 vtable 优化

- 实例数量 ≥ 6
- 所有实例方法实现相同
- RAM 紧张（<32KB）

### 何时用值对象

- 数据小于 8 字节
- 没有关联行为
- 需要在模块间频繁传递

---

## 完整数据流

以一次蓝牙指令更新舵机为例：

```
蓝牙数据到达
  → USART1_IRQHandler (Serial.c:264)
    → 状态机解析 rx_pack
      → rx_flag = true

主循环 Serial_check_timeout → 检测 rx_flag
  → steer_check_bluetooth_signal (Steering.c:26)
    → 读取 rx_pack → 构造 Vector
    → steer_set_vector(vector)
      → check_vector (边界裁剪)
      → _vector_to_compare (角度→CCR转换)
      → steer_set_pwm_with_vector
        → pwm_set_compare(channel, ccr)          ← 虚函数分发
          → pwm_compare_registry[channel](ccr)   ← O(1) 跳转
            → TIM_SetCompareN(TIM2, ccr)         ← 硬件寄存器

整个链路无需任何运行时类型判断。
```
