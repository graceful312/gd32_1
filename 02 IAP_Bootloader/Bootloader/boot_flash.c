/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：boot_flash.c
 * 功能：Bootloader 内部 Flash 擦写驱动
 *
 *       GD32F470VET6 512KB Flash，扇区布局：
 *       扇区 0-3: 16KB × 4 = 64KB（Bootloader 区）
 *       扇区 4:   64KB（应用区 0x08010000）
 *       扇区 5-7: 128KB × 3 = 384KB（应用区）
 *       合计应用区：448KB
************************************************************/

#include "boot_flash.h"
#include "gd32f4xx_fmc.h"

/* 应用区对应的 Flash 扇区号 */
static const uint32_t app_sectors[] = {
    CTL_SECTOR_NUMBER_4,    /* 64KB  @ 0x08010000 */
    CTL_SECTOR_NUMBER_5,    /* 128KB @ 0x08020000 */
    CTL_SECTOR_NUMBER_6,    /* 128KB @ 0x08040000 */
    CTL_SECTOR_NUMBER_7,    /* 128KB @ 0x08060000 */
};
#define APP_SECTOR_COUNT  (sizeof(app_sectors) / sizeof(app_sectors[0]))

/************************************************************
 * 函 数 名: Boot_Flash_EraseApp
 * 功能说明: 擦除应用区全部扇区（4-7）
 * 参    数: 无
 * 返 回 值: 0=成功, -1=擦除失败
************************************************************/
int Boot_Flash_EraseApp(void)
{
    fmc_state_enum state;

    fmc_unlock();

    /* 清除上次操作的错误标志 */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR |
                   FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    for (uint32_t i = 0; i < APP_SECTOR_COUNT; i++)
    {
        state = fmc_sector_erase(app_sectors[i]);
        if (state != FMC_READY)
        {
            fmc_lock();
            return -1;
        }
    }

    fmc_lock();
    return 0;
}

/************************************************************
 * 函 数 名: Boot_Flash_WriteBlock
 * 功能说明: 将数据块写入 Flash（按 32 位字编程）
 *           地址必须 4 字节对齐，数据长度必须是 4 的倍数
 *           写入前确保目标区域已擦除（全 0xFF）
 * 参    数: addr - 目标地址（应用区范围内）
 *           data - 数据指针
 *           len  - 数据长度（字节）
 * 返 回 值: 0=成功, -1=地址越界或写入失败
************************************************************/
int Boot_Flash_WriteBlock(uint32_t addr, const uint8_t *data, uint32_t len)
{
    fmc_state_enum state;

    /* 地址范围检查 */
    if (addr < APP_BASE_ADDR || (addr + len) > APP_END_ADDR)
        return -1;

    fmc_unlock();

    /* 按 32 位字逐个写入 */
    for (uint32_t i = 0; i < len; i += 4)
    {
        uint32_t word = (uint32_t)data[i]
                      | ((uint32_t)data[i + 1] << 8)
                      | ((uint32_t)data[i + 2] << 16)
                      | ((uint32_t)data[i + 3] << 24);

        state = fmc_word_program(addr + i, word);
        if (state != FMC_READY)
        {
            fmc_lock();
            return -1;
        }
    }

    fmc_lock();
    return 0;
}
