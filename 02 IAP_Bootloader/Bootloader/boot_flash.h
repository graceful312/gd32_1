/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：boot_flash.h
 * 功能：Bootloader 内部 Flash 擦写驱动
************************************************************/

#ifndef __BOOT_FLASH_H
#define __BOOT_FLASH_H

#include "gd32f4xx.h"

/************************ 应用区定义 ************************/

#define APP_BASE_ADDR      0x08010000U     /* 应用程序起始地址 */
#define APP_END_ADDR       0x08080000U     /* 应用程序结束地址（不含） */
#define APP_MAX_SIZE       (APP_END_ADDR - APP_BASE_ADDR)  /* 448KB */

/************************ 函数声明 ************************/

int Boot_Flash_EraseApp(void);
int Boot_Flash_WriteBlock(uint32_t addr, const uint8_t *data, uint32_t len);

#endif
