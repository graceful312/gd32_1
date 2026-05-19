/*-----------------------------------------------------------------------
/  FatFs 底层磁盘 I/O 驱动 — SPI Flash (GD25Q40ESIGR) 适配层
/-----------------------------------------------------------------------
/
/  本文件将 FatFs 文件系统接口映射到 SPI Flash 硬件操作。
/
/  存储介质参数：
/    - Flash 容量：512KB（4Mbit）
/    - Flash 扇区：4KB（擦除最小单位）
/    - Flash 页：256 字节（写入最小单位）
/    - FatFs 扇区：512 字节（逻辑扇区大小）
/    - 总逻辑扇区数：1024 个（512KB / 512B）
/    - 每个 Flash 扇区包含 8 个 FatFs 扇区
/
/  写入策略：
/    Flash 必须先擦除（整扇区变 0xFF）再写入。
/    写入时采用"读出 → 修改 → 擦除 → 写回"的方式，
/    以 4KB Flash 扇区为单位操作，保留未修改区域的数据。
/
/  平台：GD32F470VET6 (CIMC IHD V0.4)
/-----------------------------------------------------------------------*/

#include "diskio.h"
#include "SPI_Flash.h"

/************************* 宏定义 *************************/

#define FLASH_SECTOR_SIZE       4096    /* Flash 物理扇区大小（擦除最小单位） */
#define SECTOR_SIZE             512     /* FatFs 逻辑扇区大小 */
#define FLASH_TOTAL_SIZE        (512 * 1024)   /* Flash 总容量 512KB */
#define SECTOR_COUNT            (FLASH_TOTAL_SIZE / SECTOR_SIZE)  /* 逻辑扇区总数：1024 */
#define SECTORS_PER_FLASH_SEC   (FLASH_SECTOR_SIZE / SECTOR_SIZE) /* 每 Flash 扇区含 8 个逻辑扇区 */

/* 静态扇区缓冲区，用于写入时的读-改-写操作（4KB） */
static uint8_t sector_buffer[FLASH_SECTOR_SIZE];

/************************ 函数实现 ************************/

/*-----------------------------------------------------------------------*/
/* 初始化磁盘驱动                                                       */
/*-----------------------------------------------------------------------*/
/* 参数：drv — 物理驱动器号（0）                                         */
/* 返回：0 = 成功，STA_NOINIT = 初始化失败                               */
/* 流程：初始化 SPI 接口 → 读取 Flash ID → 验证芯片型号                  */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE drv)
{
    uint32_t id;

    if (drv != 0) {
        return STA_NOINIT;
    }

    /* 初始化 SPI1 接口 */
    spi_flash_init();

    /* 读取 Flash JEDEC ID */
    id = spi_flash_read_id();

    /* 验证是否为 GD25Q40ESIGR（ID = 0xC84013） */
    if (id == SPI_FLASH_ID) {
        return 0;           /* 初始化成功 */
    }

    return STA_NOINIT;      /* ID 不匹配，初始化失败 */
}

/*-----------------------------------------------------------------------*/
/* 获取磁盘状态                                                          */
/*-----------------------------------------------------------------------*/
/* 参数：drv — 物理驱动器号                                              */
/* 返回：0 = 就绪，STA_NOINIT = 未初始化                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE drv)
{
    if (drv != 0) {
        return STA_NOINIT;
    }

    return 0;   /* SPI Flash 始终就绪 */
}

/*-----------------------------------------------------------------------*/
/* 读取扇区                                                              */
/*-----------------------------------------------------------------------*/
/* 参数：drv   — 物理驱动器号                                            */
/*       buff  — 数据接收缓冲区                                          */
/*       sector — 逻辑扇区号（LBA，0~1023）                              */
/*       count  — 要读取的扇区数（1~255）                                */
/* 返回：RES_OK = 成功，RES_ERROR = 失败，RES_PARERR = 参数错误          */
/* 流程：逻辑扇区号 × 512 = Flash 字节地址，直接调用 spi_flash_buffer_read */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, BYTE count)
{
    uint32_t byte_addr;

    /* 参数检查 */
    if (drv != 0 || buff == NULL || count == 0) {
        return RES_PARERR;
    }

    /* 逻辑扇区号转字节地址 */
    byte_addr = sector * SECTOR_SIZE;

    /* 读取数据（可连续读取，无页边界限制） */
    spi_flash_buffer_read(buff, byte_addr, count * SECTOR_SIZE);

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* 写入扇区                                                              */
/*-----------------------------------------------------------------------*/
/* 参数：drv    — 物理驱动器号                                           */
/*       buff   — 要写入的数据                                           */
/*       sector — 逻辑扇区号（LBA）                                      */
/*       count  — 要写入的扇区数（1~255）                                */
/* 返回：RES_OK = 成功，RES_ERROR = 失败，RES_PARERR = 参数错误          */
/* 流程：                                                                */
/*   1. 计算目标逻辑扇区所在的 Flash 扇区（4KB 对齐）                    */
/*   2. 读出整个 Flash 扇区到缓冲区                                      */
/*   3. 将写入数据覆盖到缓冲区对应位置                                    */
/*   4. 擦除 Flash 扇区                                                  */
/*   5. 将缓冲区写回 Flash                                               */
/*-----------------------------------------------------------------------*/
#if _READONLY == 0
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, BYTE count)
{
    uint32_t flash_sec_base;    /* Flash 扇区起始地址 */
    uint32_t offset_in_flash;   /* 逻辑扇区在 Flash 扇区内的字节偏移 */
    uint32_t byte_addr;         /* 目标字节地址 */

    /* 参数检查 */
    if (drv != 0 || buff == NULL || count == 0) {
        return RES_PARERR;
    }

    byte_addr = sector * SECTOR_SIZE;

    /* 逐个 Flash 扇区处理（4KB 为单位） */
    while (count > 0) {
        /* 计算当前逻辑扇区所在的 Flash 扇区基地址 */
        flash_sec_base = byte_addr - (byte_addr % FLASH_SECTOR_SIZE);
        /* 计算在 Flash 扇区内的偏移 */
        offset_in_flash = byte_addr % FLASH_SECTOR_SIZE;

        /* 1. 读出整个 Flash 扇区到缓冲区 */
        spi_flash_buffer_read(sector_buffer, flash_sec_base, FLASH_SECTOR_SIZE);

        /* 2. 计算本次可写入的扇区数（不超过当前 Flash 扇区边界） */
        uint32_t space_left = FLASH_SECTOR_SIZE - offset_in_flash;
        uint32_t sectors_to_write = space_left / SECTOR_SIZE;
        if (sectors_to_write > count) {
            sectors_to_write = count;
        }

        /* 3. 将新数据覆盖到缓冲区对应位置 */
        memcpy(&sector_buffer[offset_in_flash], buff, sectors_to_write * SECTOR_SIZE);

        /* 4. 擦除 Flash 扇区 */
        spi_flash_sector_erase(flash_sec_base);

        /* 5. 将修改后的缓冲区写回 Flash */
        spi_flash_buffer_write(sector_buffer, flash_sec_base, FLASH_SECTOR_SIZE);

        /* 更新指针和计数 */
        buff   += sectors_to_write * SECTOR_SIZE;
        byte_addr += sectors_to_write * SECTOR_SIZE;
        count  -= sectors_to_write;
    }

    return RES_OK;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* 磁盘控制命令                                                          */
/*-----------------------------------------------------------------------*/
/* 参数：drv  — 物理驱动器号                                             */
/*       ctrl — 控制命令码                                                */
/*       buff — 数据缓冲区（输入/输出）                                   */
/* 返回：RES_OK = 成功                                                   */
/* 支持的命令：                                                          */
/*   CTRL_SYNC        — 同步（无需操作，写入已即时生效）                  */
/*   GET_SECTOR_COUNT — 返回逻辑扇区总数（供 f_mkfs 使用）               */
/*   GET_SECTOR_SIZE  — 返回逻辑扇区大小（512）                           */
/*   GET_BLOCK_SIZE   — 返回擦除块大小（供 f_mkfs 使用）                  */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
    DWORD *dp;

    if (drv != 0) {
        return RES_PARERR;
    }

    switch (ctrl) {
        case CTRL_SYNC:
            /* SPI Flash 写入操作已同步完成，无需额外操作 */
            return RES_OK;

        case GET_SECTOR_COUNT:
            /* 返回逻辑扇区总数，供 f_mkfs 计算卷大小 */
            dp = (DWORD *)buff;
            *dp = SECTOR_COUNT;
            return RES_OK;

        case GET_SECTOR_SIZE:
            /* 返回逻辑扇区大小（字节） */
            *(WORD *)buff = SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            /* 返回擦除块大小（以扇区为单位），
               即 1 个 Flash 扇区 = 8 个逻辑扇区 */
            *(DWORD *)buff = SECTORS_PER_FLASH_SEC;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

/*-----------------------------------------------------------------------*/
/* 获取当前时间戳（供 FatFs 文件时间戳使用）                             */
/*-----------------------------------------------------------------------*/
/* 返回：32 位时间编码（格式：bit31:25=年-1980, bit24:21=月,             */
/*        bit20:16=日, bit15:11=时, bit10:5=分, bit4:0=秒/2）            */
/* 当前返回 0（固定时间），如有 RTC 可改为读取 RTC 时间。                */
/*-----------------------------------------------------------------------*/
DWORD get_fattime(void)
{
    /* TODO: 可对接 RTC 模块，返回实际时间 */
    /* 例如：return ((2025-1980)<<25) | (1<<21) | (1<<16) | (0<<11) | (0<<5) | 0; */
    return 0;
}
