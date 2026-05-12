# 修改记录

## 2026-05-09

### Bug 修复

**自动模式索引越界** — `MenuStackManager.c:260`
`sizeof(AUTO_PHASES)` 返回字节大小 32 而非元素个数 8，导致 `auto_index` 在 0~7 合法范围后继续增长，越界读取内存。改为 `sizeof(AUTO_PHASES) / sizeof(AUTO_PHASES[0])`。

**自动模式切换无变化** — `MenuStackManager.c:52,257-258`
`auto_timer` 累加 72000 次主循环迭代才切换一次（约 12 分钟），且改用 Tick 模块的毫秒时间戳替代循环计数，间隔改为 200ms。

**主菜单行闪烁** — `MenuStackManager.c:99-104`
`_reverse_area_with_index()` 每帧对同一区域 XOR 翻转，导致高速反转/恢复闪烁。改为仅在 `menu_index` 变化时先恢复旧行再高亮新行。

**子页面返回后主菜单空白** — `MenuStackManager.c:161-165`
`MenuStack_Pop()` 只调了离开页面的 `on_exit`，未调返回页面的 `on_enter`，主菜单绘制逻辑不被触发。改为 pop 后调用新栈顶的 `on_enter`。

**NVIC 优先级分组重复调用** — `main.c:9` / `Timer.c` / `Serial.c`
`NVIC_PriorityGroupConfig` 在 Timer1_Init、Timer2_Init、Serial_Init 各调用一次。STM32 手册要求仅调用一次，多次调用会覆盖优先级配置。改为在 `main()` 中初始化前唯一调用一次。

**复合字面量常量报错** — `MenuStackManager.c:36-48`
Keil ARM 编译器不支持全局数组初始化器中的复合字面量 `(Vector){0, 0}`。宏改为纯大括号初始化器 `{0, 0}`，并补全缺失的分号。`RightTop` 值从 `{90, 180}` 修正为 `{180, 180}`。

### 优化

**Delay 统一 SysTick 访问** — `Delay.c`
提取 `_delay_chunk()` 内部函数，`Delay_us` 自动将长延迟拆为最大 233015us 的块，`Delay_ms`/`Delay_s` 统一走 `Delay_us`，减少 SysTick 重载次数。

**_auto_show_all 只绘制一次** — `MenuStackManager.c:77`
静态菜单文字从 `_mainmenu_update`（每帧调用）移至 `_mainmenu_enter`（进入时调用一次）。

### 新增

**Tick 模块** — `System/Tick.c` `System/Tick.h`
基于 TIM3 的毫秒级时间戳模块，替代 `auto_timer` 循环计数，提供 `Tick_Get()`、`Tick_Pause()`、`Tick_Resume()` 接口。
