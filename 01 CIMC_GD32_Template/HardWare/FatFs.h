/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：FatFs.h
 * 功能：FatFs 文件系统模块封装头文件
 *
 *       基于 FatFs R0.09（ChaN），底层通过 diskio.c 对接 SPI Flash。
 *       支持 FAT12/16/32 文件系统，提供文件读写、目录操作、格式化等 API。
 *
 *       存储介质：外部 SPI Flash（GD25Q40ESIGR，512KB）
 *       逻辑扇区：512 字节，共 1024 个扇区
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
************************************************************/

#ifndef __FATFS_H
#define __FATFS_H

#include "HeaderFiles.h"
#include "ff.h"
#include "diskio.h"

/************************ 变量定义 ************************/

/* 文件系统对象（全局，供应用层使用） */
extern FATFS fatfs;

/************************ 函数定义 ************************/

void FatFs_Init(void);    /* FatFs 初始化（挂载文件系统，首次使用自动格式化） */

#endif /* __FATFS_H */
