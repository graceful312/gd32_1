/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：boot_cmd.h
 * 功能：应用层 Bootloader 通信接口
 *
 *       提供 RebootToBootloader() 函数，用于从应用程序
 *       主动重启进入 Bootloader 固件更新模式。
 *       原理：写入 RTC 备份寄存器 BKP1 标志 + 系统复位
************************************************************/

#ifndef __BOOT_CMD_H
#define __BOOT_CMD_H

#include "gd32f4xx.h"

#define BOOT_FLAG_MAGIC  0x424F4F54U    /* "BOOT" 标志值 */

/************************************************************
 * 函 数 名: RebootToBootloader
 * 功能说明: 设置 Bootloader 标志并复位系统
 *           Bootloader 启动时检测到此标志会自动进入更新模式
 * 参    数: 无
 * 返 回 值: 无（会触发系统复位，不会返回）
************************************************************/
static inline void RebootToBootloader(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    /* 写入 BKP1（RTC 基地址 + 0x50 + 1×4） */
    *(volatile uint32_t *)(RTC_BASE + 0x50U + 1 * 4) = BOOT_FLAG_MAGIC;
    NVIC_SystemReset();
}

#endif
