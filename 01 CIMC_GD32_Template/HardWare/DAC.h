/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：DAC.h
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/05/19     V0.01    original
************************************************************/

#ifndef __DAC_H
#define __DAC_H

#include "HeaderFiles.h"

/************************* 通道定义 *************************/

#define DAC_CH0                 0       /* DAC 通道 0 — PA4 */
#define DAC_CH1                 1       /* DAC 通道 1 — PA5 */

/************************* 函数声明 *************************/

void DAC_Init(void);                                    // DAC 初始化（双通道，PA4/PA5）
void DAC_SetValue(uint8_t channel, uint16_t value);     // 设置 DAC 输出值（12位，0~4095）

#endif
