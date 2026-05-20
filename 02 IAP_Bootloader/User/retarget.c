/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：retarget.c
 * 功能：printf 重定向到 USART2
************************************************************/

#include "gd32f4xx.h"
#include "gd32f4xx_usart.h"
#include <stdio.h>

#pragma import(__use_no_semihosting)

struct __FILE { int handle; };
FILE __stdout;

void _sys_exit(int x) { x = x; }

int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART2, (uint8_t)ch);
    while (RESET == usart_flag_get(USART2, USART_FLAG_TBE));
    return ch;
}
