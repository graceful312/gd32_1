/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：boot_xmodem.h
 * 功能：XMODEM-CRC 文件传输协议
************************************************************/

#ifndef __BOOT_XMODEM_H
#define __BOOT_XMODEM_H

#include "gd32f4xx.h"

/************************ 函数声明 ************************/

int Boot_Xmodem_Receive(uint32_t app_base);

#endif
