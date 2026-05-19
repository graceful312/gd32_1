# CLAUDE.md

本文件用于指导 Claude Code 在本项目中的工作。

## 项目概况

CIMC 2026 工业嵌入式系统开发竞赛项目，目标芯片 **GD32F470VET6**（Cortex-M4, 168 MHz），开发板 **CIMC IHD V0.4**。主项目在 `01 CIMC_GD32_Template/` 目录下，顶层其他目录是 GD30AD3344 参考代码，不参与主项目编译。

## 编译方式

- **IDE**: Keil uVision MDK-ARM，编译器 ARMCC V5.06
- **工程文件**: `01 CIMC_GD32_Template/project/CIMC_GD32_Template.uvprojx`
- 没有 Makefile/CMakeLists，只能通过 Keil 编译
- **依赖包**: GigaDevice.GD32F4xx_DFP.3.0.3（通过 Keil Pack Installer 安装）

## 代码结构

分层架构，从底层到上层：

```
CMSIS/          → ARM Cortex-M4 核心头文件 + GigaDevice 设备头文件 (gd32f4xx.h)
Library/        → GD32F4xx 标准外设库 V2.6.4（28 个外设驱动）
HardWare/       → 板级驱动：LED、Key、OLED、Serial、ADC、Timer、GD30AD3344、RTC、SPI_Flash、FatFs
Function/       → 应用逻辑：System_Init()、UsrFunction()
User/           → main.c 入口、中断处理 (gd32f4xx_it.c)、systick
HeaderFiles/    → HeaderFiles.h，统一包含所有头文件
Protocol/       → 空目录（竞赛任务区域）
System/         → 空目录
```

所有 `.c` 文件只 include `HeaderFiles.h`，它会链式引入 gd32f4xx.h → libopt.h → 所有外设头文件。

### 程序入口

`main()` → `System_Init()`（初始化 systick、按键、LED、OLED、USART2、Timer1、GD30AD3344、RTC、SPI_Flash、FatFs）→ `UsrFunction()`（主循环，调用 `OLED_Refresh()`）。

### 关键驱动

- **按键** (`HardWare/Key.c`)：状态机消抖，支持单击/双击/长按/连发检测。`Key_Tick()` 由 Timer1 中断每 1ms 调用，实际 GPIO 采样间隔 20ms。用 `Key_Check(n, KEY_SINGLE | KEY_DOUBLE | KEY_LONG | KEY_REPEAT | KEY_HOLD)` 读取事件。
- **串口** (`HardWare/Serial.c`)：USART2，115200 波特率（PB10 TX，PC5 RX）。数据包协议：`$` 开头，`#` 结尾。支持 8 路数字（`D` 前缀）和模拟（`A` 前缀）传感器数据解析。
- **OLED** (`HardWare/OLED.c`)：SPI 驱动的显示屏。主循环中调用 `OLED_Refresh()` 刷新显存。
- **定时器** (`HardWare/Timer.c`)：Timer1，1kHz，中断中驱动 `Key_Tick()`。
- **LED** (`HardWare/LED.h`)：4 个 LED 在 GPIOA——LED1=PA4，LED2=PA5，LED3=PA0，LED4=PA1。高电平点亮，用 `LEDx_ON()`/`LEDx_OFF()` 宏控制。
- **ADC** (`HardWare/ADC.c`)：PC0，仅做了基本时钟配置。
- **GD30AD3344** (`HardWare/GD30AD3344.c`)：外部 ADC 芯片，通过 SPI1（PB12~PB15）通信。CS=PB12，SPI 初始化和引脚复用配置在 `GD30AD3344_spi.h` 中以宏定义封装，修改宏即可切换 SPI 端口。
- **RTC** (`HardWare/RTC.c`)：实时时钟模块，使用外部 32.768kHz LXTAL 晶振，预分频 1Hz。支持日历读写（BCD 编码）、闹钟 0/1（带中断）、唤醒定时器、备份寄存器。中断处理函数（`RTC_Alarm_IRQHandler`、`RTC_WKUP_IRQHandler`）直接写在 RTC.c 中，不修改 `gd32f4xx_it.c`。
- **SPI Flash** (`HardWare/SPI_Flash.c`)：外部 NOR Flash（GD25Q40ESIGR，4Mbit/512KB），SPI1 通信。支持扇区擦除（4KB）、整片擦除、页写入（256B）、任意长度读写、ID 读取。与 GD30AD3344 共用 SPI1 总线，通过 CS 引脚区分。
- **FatFs** (`HardWare/FatFs.c`)：FAT 文件系统 R0.09 封装，底层 `diskio.c` 对接 SPI Flash。512B 逻辑扇区，1024 个扇区（512KB）。首次使用自动格式化并挂载。API 参考 `ff.h`。配置 `ffconf.h`：代码页 936（GBK 中文），已开启 `_USE_MKFS`。

### 引脚分配

| 外设 | 引脚 | 备注 |
|------|------|------|
| LED1-LED4 | PA4, PA5, PA0, PA1 | 高电平点亮 |
| KEY1-KEY4 | PA4, PA5, PA6, PA7 | 上拉输入，低电平有效（注意 PA4/PA5 与 LED 共用） |
| USART2 TX/RX | PB10 / PC5 | AF7，115200 波特率 |
| ADC | PC0 | 模拟模式 |
| GD30AD3344 / SPI Flash | PB13(SCK), PB14(MISO), PB15(MOSI), PB12(CS) | SPI1, AF5, **共用 CS 引脚 PB12，不能同时使用** |
| RTC LXTAL | PC14/PC15 | 外部 32.768kHz 晶振 |
| Timer1 | 内部 | 1kHz 中断 |

### 存储器

- Flash：512 KB，起始 0x08000000
- SRAM：192 KB（128 KB @ 0x20000000 + 64 KB @ 0x10000000）
- 分散加载文件：`project/Objects/CIMC_GD32_Template.sct`

## 开发约定

- 代码注释用中文，新增代码保持一致风格
- 驱动放在 `HardWare/` 下，每个外设一对 `.c`/`.h`
- 全局变量（标志位、缓冲区）在驱动 `.c` 中定义，通过 `extern` 或直接 include 访问
- `Protocol/` 和 `System/` 目录留给竞赛任务实现
- NVIC 优先级分组在 `Timer1_Init()` 中设为 2 位抢占 + 2 位子优先级
- 使用标准外设库（SPL），不用 HAL——外设调用统一用 `Library/GD32F4xx_standard_peripheral/` 下的 `gd32f4xx_*` 函数
- 不要修改 `Library/` 下的 `gd32f4xx_*` 库文件和 `User/gd32f4xx_it.c`，中断处理函数写在对应模块的 `.c` 文件中
- 所有模块 `.c` 文件只 `#include "HeaderFiles.h"`，由它统一引入所有头文件（中心辐射模式）

### ARMCC 编译器注意事项

- ARMCC V5.06 **不支持** `printf` 中使用 UTF-8 中文字符串，会导致 `#870-D: invalid multibyte character sequence` 警告。串口输出一律使用英文。
- GD32F4xx 库中星期三的宏名是 `RTC_WEDSDAY`（少了一个 E），不是 `RTC_WEDNESDAY`。
