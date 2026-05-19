/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：ADC.h
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#ifndef __ADC_H
#define __ADC_H

#include "HeaderFiles.h"

void ADC_port_init(void);    // ADC引脚和时钟初始化，启动转换
void ADC_Init(void);         // ADC外设初始化（复位、模式、通道、触发、使能、校准）

#endif
