/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：LED.h
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#ifndef __LED_H
#define __LED_H

#include "HeaderFiles.h"

/************************* 引脚定义 *************************/

#define LED_CLK    RCU_GPIOA
#define LED_Port   GPIOA
#define LED1_Pin   GPIO_PIN_4       /* LED1 - PA4（与 KEY1 共用引脚） */
#define LED2_Pin   GPIO_PIN_5       /* LED2 - PA5（与 KEY2 共用引脚） */
#define LED3_Pin   GPIO_PIN_0       /* LED3 - PA0 */
#define LED4_Pin   GPIO_PIN_1       /* LED4 - PA1 */

/************************* 控制宏 *************************/

#define LED1_ON()   gpio_bit_set(LED_Port, LED1_Pin)
#define LED1_OFF()  gpio_bit_reset(LED_Port, LED1_Pin)

#define LED2_ON()   gpio_bit_set(LED_Port, LED2_Pin)
#define LED2_OFF()  gpio_bit_reset(LED_Port, LED2_Pin)

#define LED3_ON()   gpio_bit_set(LED_Port, LED3_Pin)
#define LED3_OFF()  gpio_bit_reset(LED_Port, LED3_Pin)

#define LED4_ON()   gpio_bit_set(LED_Port, LED4_Pin)
#define LED4_OFF()  gpio_bit_reset(LED_Port, LED4_Pin)

/************************* 函数声明 *************************/

void LED_Init(void);       // LED GPIO 初始化

#endif
