# IAP Bootloader Keil 配置说明

## 一、Bootloader 工程配置

### 1.1 新建工程

在 `02 IAP_Bootloader/project/` 目录下新建 Keil 工程：
- 目标芯片：**GD32F470VE**
- Pack：GigaDevice.GD32F4xx_DFP.3.0.3

### 1.2 添加源文件

**Bootloader 自有文件**（共 12 个）：

| 分组 | 文件路径 |
|------|---------|
| User | `User/main.c` |
| User | `User/gd32f4xx_it.c` |
| User | `User/retarget.c` |
| User | `User/systick.c` |
| Bootloader | `Bootloader/boot_uart.c` |
| Bootloader | `Bootloader/boot_flash.c` |
| Bootloader | `Bootloader/boot_xmodem.c` |

**SPL 库文件**（引用上级 `01 CIMC_GD32_Template/` 目录，不复制）：

| 文件路径 |
|---------|
| `../01 CIMC_GD32_Template/Library/GD32F4xx_standard_peripheral/Source/gd32f4xx_fmc.c` |
| `../01 CIMC_GD32_Template/Library/GD32F4xx_standard_peripheral/Source/gd32f4xx_rcu.c` |
| `../01 CIMC_GD32_Template/Library/GD32F4xx_standard_peripheral/Source/gd32f4xx_gpio.c` |
| `../01 CIMC_GD32_Template/Library/GD32F4xx_standard_peripheral/Source/gd32f4xx_usart.c` |
| `../01 CIMC_GD32_Template/Library/GD32F4xx_standard_peripheral/Source/gd32f4xx_misc.c` |
| `../01 CIMC_GD32_Template/Library/GD32F4xx_standard_peripheral/Source/gd32f4xx_pmu.c` |
| `../01 CIMC_GD32_Template/CMSIS/GD/GD32F4xx/Source/system_gd32f4xx.c` |
| `../01 CIMC_GD32_Template/CMSIS/GD/GD32F4xx/Source/ARM/startup_gd32f450_470.s` |

### 1.3 C/C++ 配置

**Define**：
```
USE_STDPERIPH_DRIVER,GD32F470
```

**Include Paths**：
```
..\01 CIMC_GD32_Template\CMSIS\
..\01 CIMC_GD32_Template\CMSIS\GD\GD32F4xx\Include\
..\01 CIMC_GD32_Template\Library\GD32F4xx_standard_peripheral\Include\
.\User\
.\Bootloader\
```

### 1.4 Linker 配置

- 勾选 **Use Memory Layout from Target Dialog** → **取消勾选**（使用自定义分散加载文件）
- Scatter File 路径：`.\project\IAP_Bootloader.sct`

或者在 Target 对话框中设置：
- IROM1: Start = `0x08000000`, Size = `0x00010000`

### 1.5 Output 配置

- 勾选 **Create HEX File**（用于调试器烧录）

---

## 二、应用工程修改

### 2.1 修改 IROM1 地址

在 Keil 中打开 `01 CIMC_GD32_Template` 工程：

**Target 对话框**（Options for Target → Target）：
- IROM1 Start: `0x08000000` → **`0x08010000`**
- IROM1 Size: `0x00080000` → **`0x00070000`**

或者直接使用已修改的分散加载文件（`project/Objects/CIMC_GD32_Template.sct` 已更新）。

### 2.2 VTOR 重定位

`Function/Function.c` 已修改，`System_Init()` 开头添加了：
```c
SCB->VTOR = 0x08010000U;
```

### 2.3 Bootloader 跳转接口

新建 `Function/boot_cmd.h`，应用中使用：
```c
#include "boot_cmd.h"

// 需要进入 Bootloader 时调用：
RebootToBootloader();
```

---

## 三、触发按键配置

`02 IAP_Bootloader/Bootloader/boot_uart.h` 中定义了 4 个宏：

```c
#define BOOT_KEY_GPIO       GPIOA       /* GPIO 端口 */
#define BOOT_KEY_PIN        GPIO_PIN_6  /* 引脚号 */
#define BOOT_KEY_RCU        RCU_GPIOA   /* 时钟 */
#define BOOT_KEY_ACTIVE_LOW 1           /* 1=低电平有效, 0=高电平有效 */
```

修改这 4 个宏即可更换触发引脚，例如改为 KEY4(PA7)：
```c
#define BOOT_KEY_GPIO       GPIOA
#define BOOT_KEY_PIN        GPIO_PIN_7
#define BOOT_KEY_RCU        RCU_GPIOA
#define BOOT_KEY_ACTIVE_LOW 1
```

---

## 四、Flash 存储器分区

| 区域 | 地址范围 | 大小 | 扇区 |
|------|---------|------|------|
| Bootloader | 0x08000000 ~ 0x0800FFFF | 64KB | 0-3（各 16KB）|
| Application | 0x08010000 ~ 0x0807FFFF | 448KB | 4（64KB）+ 5-7（各 128KB）|

---

## 五、固件更新操作流程

### 方式一：按键触发

1. 按住触发键（默认 KEY3/PA6）→ 上电或复位
2. 串口终端显示 Bootloader 菜单（115200 8N1）
3. 发送字符 `1` → 开始擦除 + 等待 XMODEM 传输
4. 使用串口工具发送 .bin 固件文件（**XMODEM-CRC 模式**）
5. 传输完成 → 自动跳转到新固件

### 方式二：应用内触发

应用代码中调用 `RebootToBootloader()`：
- 写入 BKP1 = 0x424F4F54
- 调用 NVIC_SystemReset() 复位
- Bootloader 检测到 BKP1 标志，自动进入更新模式

---

## 六、推荐上位机软件

| 软件 | 说明 |
|------|------|
| SecureCRT | 内置 XMODEM，菜单 Transfer → Send Xmodem，选 CRC 模式 |
| Tera Term | 免费，File → Transfer → XMODEM → Send，勾选 CRC |
| Mobaxterm | 免费版支持串口 + XMODEM |

**串口参数**：115200 / 8 / None / 1 / 无流控

**发送文件格式**：.bin（纯二进制，非 .hex）

---

## 七、系统时钟

- 系统主频：**240MHz**（25MHz HXTAL + PLL）
- AHB = 240MHz（/1）
- APB1 = 60MHz（/4）
- APB2 = 120MHz（/2）
- Bootloader 和应用必须使用同一个 `system_gd32f4xx.c`，确保波特率一致
