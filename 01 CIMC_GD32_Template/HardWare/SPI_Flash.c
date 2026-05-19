/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：SPI_Flash.c
 * 功能：外部 SPI Flash 驱动实现（GD25Q40ESIGR）
 *
 *       基于 SPI1 总线驱动板载 GD25Q40ESIGR NOR Flash 芯片。
 *       容量 4Mbit（512KB），页 256 字节，扇区 4KB。
 *
 *       SPI 模式：CPOL=0, CPHA=0（模式 0），MSB 在前，8 位帧。
 *       SPI 时钟 = AHB/8 = 168MHz/8 = 21MHz。
 *
 *       与 GD30AD3344 共用 SPI1 总线，通过不同 CS 片选区分。
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
************************************************************/

#include "SPI_Flash.h"

/************************* 指令定义 *************************/

#define CMD_WRITE       0x02    /* 页编程指令（Page Program） */
#define CMD_WRSR        0x01    /* 写状态寄存器指令 */
#define CMD_WREN        0x06    /* 写使能指令 */
#define CMD_READ        0x03    /* 读数据指令（Read Data） */
#define CMD_RDSR        0x05    /* 读状态寄存器指令 */
#define CMD_RDID        0x9F    /* 读 JEDEC ID 指令 */
#define CMD_SE          0x20    /* 扇区擦除指令（Sector Erase，4KB） */
#define CMD_BE          0xC7    /* 整片擦除指令（Bulk Erase） */

#define WIP_FLAG        0x01    /* 状态寄存器 bit0：Write In Progress 标志 */
#define DUMMY_BYTE      0xA5    /* SPI 空操作字节（用于产生时钟读取数据） */

/************************ 函数实现 ************************/

/*!
    \brief      SPI1 GPIO 和参数初始化
    \param[in]  none
    \param[out] none
    \retval     none
    \note       配置 SPI1 的 SCK/MISO/MOSI 为复用推挽输出，
                CS 为普通 GPIO 推挽输出，SPI 为主机模式 0。
*/
void spi_flash_init(void)
{
    spi_parameter_struct spi_init_struct;

    /* 使能 GPIOB 和 SPI1 时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SPI1);

    /* SPI1 引脚复用配置：SCK=PB13, MISO=PB14, MOSI=PB15，复用功能 AF5 */
    gpio_af_set(GPIOB, GPIO_AF_5, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    /* CS 片选引脚 PB12：普通 GPIO 推挽输出 */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);

    /* 默认释放片选（高电平） */
    SPI_FLASH_CS_HIGH();

    /* SPI1 参数配置 */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;  /* 全双工 */
    spi_init_struct.device_mode          = SPI_MASTER;                /* 主机模式 */
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;        /* 8 位帧 */
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;    /* CPOL=0, CPHA=0（模式0） */
    spi_init_struct.nss                  = SPI_NSS_SOFT;              /* 软件 NSS（CS 手动控制） */
    spi_init_struct.prescale             = SPI_PSC_8;                 /* 分频 8，SPI 时钟 = 168MHz/8 = 21MHz */
    spi_init_struct.endian               = SPI_ENDIAN_MSB;            /* MSB 在前 */
    spi_init(SPI1, &spi_init_struct);

    /* 使能 SPI1 */
    spi_enable(SPI1);
}

/*!
    \brief      擦除指定扇区（4KB）
    \param[in]  sector_addr: 扇区起始地址（必须为扇区对齐地址，即 4KB 边界）
    \param[out] none
    \retval     none
    \note       扇区擦除耗时约 45~200ms，擦除后该扇区所有字节变为 0xFF。
*/
void spi_flash_sector_erase(uint32_t sector_addr)
{
    /* 发送写使能命令（擦除前必须先写使能） */
    spi_flash_write_enable();

    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送扇区擦除指令 0x20 */
    spi_flash_send_byte(CMD_SE);

    /* 发送 24 位地址（高字节在前） */
    spi_flash_send_byte((sector_addr & 0xFF0000) >> 16);   /* A23~A16 */
    spi_flash_send_byte((sector_addr & 0xFF00) >> 8);       /* A15~A8  */
    spi_flash_send_byte(sector_addr & 0xFF);                 /* A7~A0   */

    /* 拉高 CS，释放 Flash，开始擦除 */
    SPI_FLASH_CS_HIGH();

    /* 等待擦除完成（轮询状态寄存器 WIP 标志） */
    spi_flash_wait_for_write_end();
}

/*!
    \brief      擦除指定地址起的若干字节（自动处理扇区边界）
    \param[in]  addr:            起始地址（不必扇区对齐）
    \param[in]  num_byte_to_erase: 要擦除的字节数
    \param[out] none
    \retval     none
    \note       该函数会自动保护未被擦除区域的数据：
                1. 读出待擦除扇区中需要保留的数据到扇区缓冲区
                2. 擦除整个扇区
                3. 写回保留的数据
                使用静态扇区缓冲区（4KB），避免栈溢出。
*/
static uint8_t sector_buf[SPI_FLASH_SECTOR_SIZE];  /* 扇区操作缓冲区（静态，避免栈溢出） */

void spi_flash_buffer_erase(uint32_t addr, uint32_t num_byte_to_erase)
{
    uint32_t sector_base;       /* 当前扇区起始地址 */
    uint32_t erase_start;       /* 扇区内擦除起始偏移 */
    uint32_t erase_end;         /* 扇区内擦除结束偏移（不含） */
    uint32_t end_addr;          /* 擦除范围结束地址（不含） */
    uint32_t cur_addr;          /* 当前遍历地址 */

    end_addr = addr + num_byte_to_erase;

    /* 按扇区遍历擦除范围 */
    for (cur_addr = addr; cur_addr < end_addr; ) {
        sector_base  = cur_addr - (cur_addr % SPI_FLASH_SECTOR_SIZE);
        erase_start  = (cur_addr == addr) ? (addr % SPI_FLASH_SECTOR_SIZE) : 0;
        erase_end    = (end_addr >= sector_base + SPI_FLASH_SECTOR_SIZE)
                       ? SPI_FLASH_SECTOR_SIZE
                       : (end_addr - sector_base);

        if (erase_start == 0 && erase_end == SPI_FLASH_SECTOR_SIZE) {
            /* 整个扇区都在擦除范围内，直接擦除 */
            spi_flash_sector_erase(sector_base);
        } else {
            /* 部分擦除：需要保留扇区内其他数据 */
            /* 1. 读出整个扇区 */
            spi_flash_buffer_read(sector_buf, sector_base, SPI_FLASH_SECTOR_SIZE);
            /* 2. 将擦除区域填充 0xFF（Flash 擦除后的默认值） */
            memset(&sector_buf[erase_start], 0xFF, erase_end - erase_start);
            /* 3. 擦除整个扇区 */
            spi_flash_sector_erase(sector_base);
            /* 4. 写回整个扇区 */
            spi_flash_buffer_write(sector_buf, sector_base, SPI_FLASH_SECTOR_SIZE);
        }

        /* 移动到下一个扇区 */
        cur_addr = sector_base + SPI_FLASH_SECTOR_SIZE;
    }
}

/*!
    \brief      整片擦除
    \param[in]  none
    \param[out] none
    \retval     none
    \note       擦除整个 Flash 芯片（512KB），耗时约 1~4 秒。
                谨慎使用，所有数据将丢失！
*/
void spi_flash_bulk_erase(void)
{
    /* 发送写使能命令 */
    spi_flash_write_enable();

    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送整片擦除指令 0xC7 */
    spi_flash_send_byte(CMD_BE);

    /* 拉高 CS，释放 Flash，开始擦除 */
    SPI_FLASH_CS_HIGH();

    /* 等待擦除完成 */
    spi_flash_wait_for_write_end();
}

/*!
    \brief      页写入（单次最多 256 字节）
    \param[in]  pbuffer:          数据缓冲区指针
    \param[in]  write_addr:       Flash 写入地址
    \param[in]  num_byte_to_write: 写入字节数（不超过页边界，即 256 - 页内偏移）
    \param[out] none
    \retval     none
    \note       页写入不能跨越页边界（256 字节对齐），否则地址会回卷到页起始。
                跨页写入请使用 spi_flash_buffer_write()。
*/
void spi_flash_page_write(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    /* 发送写使能命令 */
    spi_flash_write_enable();

    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送页编程指令 0x02 */
    spi_flash_send_byte(CMD_WRITE);

    /* 发送 24 位写入地址 */
    spi_flash_send_byte((write_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((write_addr & 0xFF00) >> 8);
    spi_flash_send_byte(write_addr & 0xFF);

    /* 逐字节写入数据 */
    while (num_byte_to_write--) {
        spi_flash_send_byte(*pbuffer);
        pbuffer++;
    }

    /* 拉高 CS，释放 Flash，开始编程 */
    SPI_FLASH_CS_HIGH();

    /* 等待写入完成 */
    spi_flash_wait_for_write_end();
}

/*!
    \brief      任意长度数据写入（自动处理页边界）
    \param[in]  pbuffer:          数据缓冲区指针
    \param[in]  write_addr:       Flash 写入起始地址
    \param[in]  num_byte_to_write: 要写入的总字节数
    \param[out] none
    \retval     none
    \note       自动将数据拆分为多段页写入，处理地址不对齐和跨页的情况。
*/
void spi_flash_buffer_write(uint8_t *pbuffer, uint32_t write_addr, uint32_t num_byte_to_write)
{
    uint32_t page_offset;       /* 写入地址在页内的偏移 */
    uint32_t space_in_page;     /* 当前页剩余可写字节数 */
    uint32_t num_full_pages;    /* 完整页数 */
    uint32_t tail_bytes;        /* 最后不满一页的字节数 */

    page_offset   = write_addr % SPI_FLASH_PAGE_SIZE;           /* 页内偏移 */
    space_in_page = SPI_FLASH_PAGE_SIZE - page_offset;          /* 当前页剩余空间 */
    num_full_pages = num_byte_to_write / SPI_FLASH_PAGE_SIZE;   /* 完整页数 */
    tail_bytes     = num_byte_to_write % SPI_FLASH_PAGE_SIZE;   /* 不足一页的字节数 */

    if (page_offset == 0) {
        /* 写入地址页对齐：直接按完整页 + 尾部写入 */
        while (num_full_pages--) {
            spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
            write_addr += SPI_FLASH_PAGE_SIZE;
            pbuffer    += SPI_FLASH_PAGE_SIZE;
        }
        if (tail_bytes > 0) {
            spi_flash_page_write(pbuffer, write_addr, tail_bytes);
        }
    } else {
        /* 写入地址非页对齐 */
        if (num_byte_to_write < space_in_page) {
            /* 数据量小于当前页剩余空间，直接写入 */
            spi_flash_page_write(pbuffer, write_addr, num_byte_to_write);
        } else {
            /* 先写满当前页剩余空间 */
            spi_flash_page_write(pbuffer, write_addr, space_in_page);

            /* 计算剩余数据 */
            num_full_pages = (num_byte_to_write - space_in_page) / SPI_FLASH_PAGE_SIZE;
            tail_bytes     = (num_byte_to_write - space_in_page) % SPI_FLASH_PAGE_SIZE;
            write_addr += space_in_page;
            pbuffer    += space_in_page;

            /* 写入完整页 */
            while (num_full_pages--) {
                spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
                write_addr += SPI_FLASH_PAGE_SIZE;
                pbuffer    += SPI_FLASH_PAGE_SIZE;
            }

            /* 写入剩余字节 */
            if (tail_bytes > 0) {
                spi_flash_page_write(pbuffer, write_addr, tail_bytes);
            }
        }
    }
}

/*!
    \brief      读取数据块
    \param[in]  pbuffer:          数据接收缓冲区指针
    \param[in]  read_addr:        Flash 读取起始地址
    \param[in]  num_byte_to_read: 要读取的字节数
    \param[out] none
    \retval     none
    \note       读取没有页边界限制，可连续读取任意长度。
*/
void spi_flash_buffer_read(uint8_t *pbuffer, uint32_t read_addr, uint16_t num_byte_to_read)
{
    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送读数据指令 0x03 */
    spi_flash_send_byte(CMD_READ);

    /* 发送 24 位读取地址 */
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);

    /* 逐字节读取数据（发送空字节产生时钟） */
    while (num_byte_to_read--) {
        *pbuffer = spi_flash_send_byte(DUMMY_BYTE);
        pbuffer++;
    }

    /* 拉高 CS，释放 Flash */
    SPI_FLASH_CS_HIGH();
}

/*!
    \brief      读取 Flash JEDEC ID
    \param[in]  none
    \param[out] none
    \retval     24 位 ID 值（高字节=厂商ID，中字节=存储器类型，低字节=容量）
    \note       GD25Q40ESIGR 的 ID 为 0xC84013。
                厂商 0xC8 = GigaDevice，容量 0x13 = 4Mbit。
*/
uint32_t spi_flash_read_id(void)
{
    uint32_t temp = 0;
    uint32_t byte0, byte1, byte2;

    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送读 ID 指令 0x9F */
    spi_flash_send_byte(CMD_RDID);

    /* 连续读取 3 个字节的 ID */
    byte0 = spi_flash_send_byte(DUMMY_BYTE);   /* 厂商 ID */
    byte1 = spi_flash_send_byte(DUMMY_BYTE);   /* 存储器类型 */
    byte2 = spi_flash_send_byte(DUMMY_BYTE);   /* 容量 */

    /* 拉高 CS，释放 Flash */
    SPI_FLASH_CS_HIGH();

    /* 合成 24 位 ID */
    temp = (byte0 << 16) | (byte1 << 8) | byte2;

    return temp;
}

/*!
    \brief      启动连续读序列
    \param[in]  read_addr: Flash 读取起始地址
    \param[out] none
    \retval     none
    \note       发送读命令和地址后不拉高 CS，可用于后续连续调用
                spi_flash_read_byte() 逐字节读取。
*/
void spi_flash_start_read_sequence(uint32_t read_addr)
{
    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送读数据指令 */
    spi_flash_send_byte(CMD_READ);

    /* 发送 24 位地址 */
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);
    /* 注意：CS 保持低电平，由调用方负责在读取结束后拉高 */
}

/*!
    \brief      从 SPI Flash 读取一个字节
    \param[in]  none
    \param[out] none
    \retval     读取到的字节
    \note       需在 spi_flash_start_read_sequence() 之后调用，
                或单独使用读取单个字节。
*/
uint8_t spi_flash_read_byte(void)
{
    return spi_flash_send_byte(DUMMY_BYTE);
}

/*!
    \brief      SPI 收发一个字节
    \param[in]  byte: 要发送的字节
    \param[out] none
    \retval     从 MISO 上读取到的字节
    \note       全双工操作：发送的同时接收。
*/
uint8_t spi_flash_send_byte(uint8_t byte)
{
    /* 等待发送缓冲区空（TBE 标志置位） */
    while (RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));

    /* 通过 SPI1 发送字节 */
    spi_i2s_data_transmit(SPI1, byte);

    /* 等待接收缓冲区非空（RBNE 标志置位） */
    while (RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE));

    /* 返回接收到的字节 */
    return spi_i2s_data_receive(SPI1);
}

/*!
    \brief      SPI 收发一个半字（16 位）
    \param[in]  half_word: 要发送的 16 位数据
    \param[out] none
    \retval     从 MISO 上读取到的 16 位数据
    \note       用于某些需要 16 位帧的 SPI 通信场景。
*/
uint16_t spi_flash_send_halfword(uint16_t half_word)
{
    /* 等待发送缓冲区空 */
    while (RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));

    /* 通过 SPI1 发送半字 */
    spi_i2s_data_transmit(SPI1, half_word);

    /* 等待接收缓冲区非空 */
    while (RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE));

    /* 返回接收到的半字 */
    return spi_i2s_data_receive(SPI1);
}

/*!
    \brief      写使能
    \param[in]  none
    \param[out] none
    \retval     none
    \note       在写入或擦除操作前必须先发送写使能命令，
                否则 Flash 会忽略写入/擦除指令。
*/
void spi_flash_write_enable(void)
{
    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送写使能指令 0x06 */
    spi_flash_send_byte(CMD_WREN);

    /* 拉高 CS，释放 Flash */
    SPI_FLASH_CS_HIGH();
}

/*!
    \brief      等待写操作完成
    \param[in]  none
    \param[out] none
    \retval     none
    \note       轮询状态寄存器的 WIP（Write In Progress）标志位，
                直到 Flash 内部写操作完成。在擦除或编程期间，
                WIP=1；操作完成后 WIP=0。
*/
void spi_flash_wait_for_write_end(void)
{
    uint8_t flash_status = 0;

    /* 拉低 CS，选中 Flash */
    SPI_FLASH_CS_LOW();

    /* 发送读状态寄存器指令 0x05 */
    spi_flash_send_byte(CMD_RDSR);

    /* 循环读取状态寄存器，直到 WIP 标志清零（写操作完成） */
    do {
        flash_status = spi_flash_send_byte(DUMMY_BYTE);
    } while ((flash_status & WIP_FLAG) == SET);

    /* 拉高 CS，释放 Flash */
    SPI_FLASH_CS_HIGH();
}
