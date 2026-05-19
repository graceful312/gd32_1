/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：DAC.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/05/19     V0.01    original
************************************************************/

#include "DAC.h"

/************************************************************
 * 函 数 名: DAC_Init
 * 功能说明: DAC 双通道初始化
 *          PA4 = DAC_OUT0，PA5 = DAC_OUT1
 *          配置为模拟输出，使能输出缓冲，初始输出 0V
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void DAC_Init(void)
{
    rcu_periph_clock_enable(RCU_DAC);                                           // 使能 DAC 时钟

    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);        // PA4 设为模拟模式
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_5);        // PA5 设为模拟模式

    /* 通道 0 初始化 */
    dac_disable(DAC0, DAC_OUT0);                                                // 先关闭
    dac_output_buffer_enable(DAC0, DAC_OUT0);                                   // 使能输出缓冲
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, 0);                          // 初始值 0
    dac_enable(DAC0, DAC_OUT0);                                                 // 使能 DAC 通道 0

    /* 通道 1 初始化 */
    dac_disable(DAC0, DAC_OUT1);
    dac_output_buffer_enable(DAC0, DAC_OUT1);
    dac_data_set(DAC0, DAC_OUT1, DAC_ALIGN_12B_R, 0);
    dac_enable(DAC0, DAC_OUT1);
}

/************************************************************
 * 函 数 名: DAC_SetValue
 * 功能说明: 设置指定 DAC 通道的输出值
 * 参    数: channel - DAC_CH0 或 DAC_CH1
 *           value   - 输出值（0~4095，对应 0~3.3V）
 * 返 回 值: 无
************************************************************/
void DAC_SetValue(uint8_t channel, uint16_t value)
{
    uint32_t dac_out = (channel == DAC_CH0) ? DAC_OUT0 : DAC_OUT1;

    dac_data_set(DAC0, dac_out, DAC_ALIGN_12B_R, value);
}
