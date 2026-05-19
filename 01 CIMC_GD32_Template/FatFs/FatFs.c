/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：FatFs.c
 * 功能：FatFs 文件系统模块封装实现
 *
 *       将 FatFs 初始化逻辑封装在此文件中，Function.c 仅调用 FatFs_Init()。
 *
 *       初始化流程：
 *         1. 初始化磁盘驱动（SPI Flash）
 *         2. 尝试挂载文件系统
 *         3. 如果 Flash 未格式化，自动 f_mkfs 后重新挂载
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
************************************************************/

#include "FatFs.h"

/************************* 全局变量 *************************/

FATFS fatfs;    /* 文件系统对象 */

/************************ 函数实现 ************************/

/*!
    \brief      FatFs 文件系统初始化
    \param[in]  none
    \param[out] none
    \retval     none
    \note       首次使用时 Flash 无文件系统，会自动格式化（f_mkfs）。
                后续上电直接挂载，跳过格式化。
*/
void FatFs_Init(void)
{
    FRESULT res;
    uint8_t retry = 3;

    /* 初始化磁盘驱动（SPI Flash），最多重试 3 次 */
    while (retry--) {
        if (disk_initialize(0) == 0) {
            break;
        }
    }

    /* 尝试挂载文件系统 */
    res = f_mount(0, &fatfs);
    if (res == FR_NO_FILESYSTEM) {
        /* Flash 尚无文件系统，执行格式化 */
        printf("[FatFs] No filesystem, formatting...\r\n");
        res = f_mkfs(0, 0, 0);     /* 驱动器0, FDISK分区, 自动分配单元 */
        if (res == FR_OK) {
            printf("[FatFs] Format OK\r\n");
            /* 格式化后重新挂载 */
            res = f_mount(0, &fatfs);
        } else {
            printf("[FatFs] Format failed: %d\r\n", res);
        }
    }

    if (res == FR_OK) {
        printf("[FatFs] Mount OK\r\n");
    } else {
        printf("[FatFs] Mount failed: %d\r\n", res);
    }
}
