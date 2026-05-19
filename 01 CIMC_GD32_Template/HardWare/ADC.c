/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：ADC.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#include "ADC.h"

/************************************************************
 * 函 数 名: ADC_Init
 * 功能说明: ADC外设初始化（复位、模式、通道、触发、使能、校准）
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void ADC_Init(void)
{
    adc_deinit();                                                          // 复位ADC外设

    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);        // 使能连续转换模式
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);                  // 数据右对齐
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1);               // 常规通道长度：1个通道

    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_56); // 通道10（PC0），采样56周期

    adc_external_trigger_source_config(ADC0, ADC_ROUTINE_CHANNEL, ADC_EXTTRIG_INSERTED_T0_CH3); // 外部触发源（定时器触发）
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, ENABLE);        // 使能外部触发

    adc_enable(ADC0);                                                      // 使能ADC接口

    delay_1ms(1);                                                          // 等待ADC稳定

    adc_calibration_enable(ADC0);                                          // ADC校准并复位校准
}

/************************************************************
 * 函 数 名: ADC_port_init
 * 功能说明: ADC GPIO引脚和时钟初始化，启动转换
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void ADC_port_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);                                    // 使能GPIOC时钟
    rcu_periph_clock_enable(RCU_ADC0);                                     // 使能ADC0时钟

    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);   // PC0设为模拟输入

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);                               // ADC时钟分频 PCLK2/8

    ADC_Init();                                                            // ADC外设初始化

    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);                // 软件触发首次转换
}

/************************************************************
 * 函 数 名: ADC_Read
 * 功能说明: 读取 ADC 转换结果
 *          连续转换模式下，每次调用自动等待下一次转换完成
 * 参    数: 无
 * 返 回 值: 12 位转换结果（0~4095），对应 0~3.3V
************************************************************/
uint16_t ADC_Read(void)
{
    adc_flag_clear(ADC0, ADC_FLAG_EOC);                    // 清除转换完成标志
    while (adc_flag_get(ADC0, ADC_FLAG_EOC) == RESET) {}   // 等待转换完成
    return (uint16_t)ADC_RDATA(ADC0);                      // 读取 12 位转换结果
}

