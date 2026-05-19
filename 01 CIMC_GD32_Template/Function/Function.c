/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#include "Function.h"
#include "../FatFs/FatFs.h"

/************************* 全局变量 *************************/

uint8_t LP_WakeupReason = LP_REASON_POWERON;    /* 唤醒原因（由 LP_GetWakeupReason 设置） */

/************************************************************
 * 函 数 名: System_Init
 * 功能说明: 系统初始化，依次配置所有外设
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void System_Init(void)
{
    /* 检测唤醒来源（需先使能 PMU 时钟才能读取 PMU 标志） */
    rcu_periph_clock_enable(RCU_PMU);
    LP_WakeupReason = LP_GetWakeupReason();

    systick_config();           // 系统时钟配置（168MHz）
    Key_Init();                 // 按键 GPIO 初始化
    LED_Init();                 // LED GPIO 初始化
    OLED_Init();                // OLED 显示屏初始化
    Serial_Init();              // USART2 串口初始化

    /* 串口就绪后打印唤醒来源 */
    if (LP_WakeupReason == LP_REASON_STANDBY)
        printf("[LP] Wakeup from Standby (reset)\r\n");
    else if (LP_WakeupReason == LP_REASON_DEEPSLEEP)
        printf("[LP] Wakeup from Deep-sleep\r\n");
    else
        printf("[LP] Power-on reset\r\n");

    Timer1_Init();              // Timer1 初始化（1kHz 中断）
    ADC_port_init();            // 内部 ADC 初始化（PC0）
    DAC_Init();                 // DAC 初始化（PA4/PA5）
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
        DAC_SetValue(0,2000);
        DAC_SetValue(1,4095);
        OLED_ShowNum(0,0,ADC_Read(),4,16);
        OLED_Refresh();         // 刷新 OLED 显存
    }
}
