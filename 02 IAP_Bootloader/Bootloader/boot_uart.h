/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：boot_uart.h
 * 功能：Bootloader USART2 轮询驱动 + 触发按键配置
************************************************************/

#ifndef __BOOT_UART_H
#define __BOOT_UART_H

#include "gd32f4xx.h"

/************************ 按键配置宏 ************************/
/* 修改以下宏即可更换 Bootloader 触发引脚 */
#define BOOT_KEY_GPIO       GPIOA       /* GPIO 端口 */
#define BOOT_KEY_PIN        GPIO_PIN_6  /* 引脚号 */
#define BOOT_KEY_RCU        RCU_GPIOA   /* 时钟 */
#define BOOT_KEY_ACTIVE_LOW 1           /* 1=低电平有效(按下=低), 0=高电平有效 */

/************************ 函数声明 ************************/

void    Boot_UART_Init(void);
void    Boot_UART_SendByte(uint8_t byte);
void    Boot_UART_SendString(const char *str);
uint8_t Boot_UART_ByteReady(void);
uint8_t Boot_UART_ReceiveByte(void);

#endif
