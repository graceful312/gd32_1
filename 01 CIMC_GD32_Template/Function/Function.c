/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#include "Function.h"
#include "../FatFs/FatFs.h"

/************************************************************
 * 函 数 名: System_Init
 * 功能说明: 系统初始化，依次配置所有外设
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void System_Init(void)
{
    systick_config();           // 系统时钟配置（168MHz）
    Key_Init();                 // 按键 GPIO 初始化
    LED_Init();                 // LED GPIO 初始化
    OLED_Init();                // OLED 显示屏初始化
    Serial_Init();              // USART2 串口初始化
    Timer1_Init();              // Timer1 初始化（1kHz 中断）
    ADC_port_init();            // 内部 ADC 初始化（PC0）
    GD30AD3344_Init();          // GD30AD3344 外部 ADC 初始化
    RTC_Init();                 // RTC 实时时钟初始化
    spi_flash_init();           // 外部 SPI Flash 初始化
    FatFs_Init();               // FatFs 文件系统初始化
}

/************************************************************
 * 函 数 名: UsrFunction
 * 功能说明: 用户主循环
 * 参    数: 无
 * 返 回 值: 无（永不返回）
************************************************************/
void UsrFunction(void)
{
    while (1)
    {
        OLED_Refresh();         // 刷新 OLED 显存
    }
}
