/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Key.h
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#ifndef __Key_H
#define __Key_H

#include "HeaderFiles.h"

/************************* 按键定义 *************************/

#define KEY_COUNT               4

#define KEY_1                   0       /* KEY1 - PA4（与 LED1 共用引脚） */
#define KEY_2                   1       /* KEY2 - PA5（与 LED2 共用引脚） */
#define KEY_3                   2       /* KEY3 - PA6 */
#define KEY_4                   3       /* KEY4 - PA7 */

/************************* 事件标志 *************************/

#define KEY_HOLD                0x01    /* 按键持续按住 */
#define KEY_DOWN                0x02    /* 按键按下瞬间 */
#define KEY_UP                  0x04    /* 按键松开瞬间 */
#define KEY_SINGLE              0x08    /* 单击（松开后 200ms 内未再按下） */
#define KEY_DOUBLE              0x10    /* 双击（200ms 内再次按下） */
#define KEY_LONG                0x20    /* 长按（按住超过 2000ms） */
#define KEY_REPEAT              0x40    /* 连发（长按后每 100ms 重复触发） */

/************************* 函数声明 *************************/

void Key_Init(void);                         // 按键 GPIO 初始化
uint8_t Key_Check(uint8_t n, uint8_t Flag);  // 检测指定按键的指定事件
void Key_Tick(void);                         // 按键扫描状态机（由 Timer1 每 1ms 调用）

#endif
