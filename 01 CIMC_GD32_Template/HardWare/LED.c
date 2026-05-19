/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：LED.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#include "LED.h"

/************************************************************
 * 函 数 名: LED_Init
 * 功能说明: 初始化 LED3（PA0）和 LED4（PA1）为推挽输出
 *          LED1（PA4）和 LED2（PA5）与按键共用引脚，此处不初始化
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void LED_Init(void)
{
    rcu_periph_clock_enable(LED_CLK);
    gpio_mode_set(LED_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED3_Pin | LED4_Pin);
    gpio_output_options_set(LED_Port, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED3_Pin | LED4_Pin);
}
