/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Headerfiles.h
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2023/2/16     V0.01    original
************************************************************/

#ifndef __HEADERFILES_H
#define __HEADERFILES_H

/************************* 头文件 *************************/

#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"
#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include "Function.h"     // 执行函数
#include "LED.h"
#include "KEY.h"
#include "oled.h"
#include "Serial.h"
#include "ADC.h"
#include "Timer.h"
#include "GD30AD3344.h"   /* GD30AD3344外部ADC芯片驱动 */
#include "RTC.h"          /* RTC实时时钟模块 */
#include "SPI_Flash.h"    /* 外部 SPI Flash 驱动 */
#include "FatFs.h"        /* FatFs 文件系统模块 */
extern uint32_t i;

#endif

/****************************End*****************************/

