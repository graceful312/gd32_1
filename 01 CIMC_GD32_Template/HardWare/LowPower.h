/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：LowPower.h
 * 功能：低功耗模式驱动头文件
 *
 *       支持 Deep-sleep（深度睡眠）和 Standby（待机）两种模式，
 *       可选唤醒源：按键 EXTI、RTC 唤醒定时器、WKUP 引脚。
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
 * 版本：V0.01, 2025/05/19
************************************************************/

#ifndef __LOWPOWER_H
#define __LOWPOWER_H

#include "HeaderFiles.h"

/************************* 唤醒源定义 *************************/

#define LP_WAKEUP_KEY       0x01    /* EXTI 按键唤醒（PA6 / KEY3） */
#define LP_WAKEUP_RTC       0x02    /* RTC 唤醒定时器 */
#define LP_WAKEUP_PIN       0x04    /* WKUP 引脚（PA0） */

/************************* 唤醒原因 *************************/

#define LP_REASON_POWERON   0       /* 正常上电 */
#define LP_REASON_DEEPSLEEP 1       /* 从 Deep-sleep 唤醒 */
#define LP_REASON_STANDBY   2       /* 从 Standby 唤醒（唤醒 = 复位） */

/************************* 变量定义 *************************/

extern uint8_t LP_WakeupReason;     /* 唤醒原因（LP_REASON_xxx） */

/************************* 函数声明 *************************/

void LP_Init(void);                                          /* 低功耗模块初始化 */
void LP_EnterDeepSleep(uint8_t wakeup_src, uint16_t rtc_sec);   /* 进入深度睡眠 */
void LP_EnterStandby(uint8_t wakeup_src, uint16_t rtc_sec);     /* 进入待机模式 */
uint8_t LP_GetWakeupReason(void);                                /* 获取唤醒原因 */

#endif
