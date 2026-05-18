/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：RTC.h
 * 功能：RTC 实时时钟模块头文件
 *
 *       基于 GD32F470VET6 内置 RTC 外设，支持：
 *       - 日历读写（年/月/日/时/分/秒，BCD编码，24小时制）
 *       - 闹钟0/闹钟1 配置与中断
 *       - 唤醒定时器
 *       - 备份寄存器读写（掉电保持，VBAT供电）
 *       - 十进制 ↔ BCD 转换工具函数
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
 * 版本：V1.0.0, 2025
************************************************************/

#ifndef __RTC_H
#define __RTC_H

#include "HeaderFiles.h"

/************************* 宏定义 *************************/

/* RTC配置标记值：写入BKP0表示RTC已配置过 */
#define RTC_BKP_FLAG        ((uint32_t)0x32F0)

/* 时钟源选择（二选一，取消注释对应选项） */
#define RTC_CLOCK_SOURCE_LXTAL      /* 外部32.768kHz低速晶振（推荐，精度高） */
// #define RTC_CLOCK_SOURCE_IRC32K  /* 内部32kHz RC振荡器（无晶振时使用，精度±2%） */

/************************ 变量定义 ************************/

/* RTC初始化参数结构体（全局，供中断和主循环访问） */
extern rtc_parameter_struct rtc_time_para;

/************************ 函数定义 ************************/

/* --- 初始化 --- */
void RTC_Init(void);                    /* RTC初始化（自动检测是否首次配置） */
void RTC_SetTime(uint8_t year, uint8_t month, uint8_t date,
                 uint8_t hour, uint8_t minute, uint8_t second);  /* 程序化设置时间 */

/* --- 日历读取 --- */
void RTC_GetTime(rtc_parameter_struct *time);   /* 读取当前时间到结构体 */
void RTC_PrintTime(void);                        /* 串口打印当前时间 */

/* --- 十进制↔BCD转换 --- */
uint8_t RTC_DecToBCD(uint8_t dec);      /* 十进制 → BCD */
uint8_t RTC_BCDToDec(uint8_t bcd);      /* BCD → 十进制 */

/* --- 闹钟 --- */
void RTC_SetAlarm0(uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);  /* 配置闹钟0 */
void RTC_SetAlarm1(uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);  /* 配置闹钟1 */
void RTC_PrintAlarm0(void);              /* 串口打印闹钟0时间 */
void RTC_PrintAlarm1(void);              /* 串口打印闹钟1时间 */

/* --- 唤醒定时器 --- */
void RTC_SetWakeup(uint16_t count);      /* 设置唤醒定时器（ck_spre时钟源，单位秒） */

/* --- 备份寄存器 --- */
void RTC_WriteBackup(uint8_t reg, uint32_t value);  /* 写备份寄存器（reg: 0~19） */
uint32_t RTC_ReadBackup(uint8_t reg);               /* 读备份寄存器 */

#endif /* __RTC_H */
