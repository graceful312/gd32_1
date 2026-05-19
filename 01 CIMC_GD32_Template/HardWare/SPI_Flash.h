/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：SPI_Flash.h
 * 功能：外部 SPI Flash 驱动头文件（GD25Q40ESIGR）
 *
 *       通过 SPI1 总线驱动板载 GD25Q40ESIGR NOR Flash 芯片。
 *       支持扇区擦除、整片擦除、页写入、任意长度读写、ID 读取。
 *
 *       SPI1 引脚：SCK=PB13, MISO=PB14, MOSI=PB15, CS=PB12
 *       注意：GD30AD3344 也使用 SPI1 总线，两者通过不同 CS 片选区分。
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
************************************************************/

#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/

/* Flash 存储参数 */
#define SPI_FLASH_PAGE_SIZE           256       /* 页大小：256 字节 */
#define SPI_FLASH_SECTOR_SIZE         4096      /* 扇区大小：4096 字节（4KB） */

/* CS 片选引脚控制（PB12） */
#define SPI_FLASH_CS_LOW()            gpio_bit_reset(GPIOB, GPIO_PIN_12)   /* 拉低 CS，选中 Flash */
#define SPI_FLASH_CS_HIGH()           gpio_bit_set(GPIOB, GPIO_PIN_12)     /* 拉高 CS，释放 Flash */

/* Flash 芯片 ID（GD25Q40ESIGR） */
#define SPI_FLASH_ID                  0xC84013

/************************ 函数定义 ************************/

/* --- 初始化 --- */
void spi_flash_init(void);                     /* SPI1 GPIO 和参数初始化 */

/* --- 擦除 --- */
void spi_flash_sector_erase(uint32_t sector_addr);                        /* 擦除指定扇区（4KB） */
void spi_flash_bulk_erase(void);                                          /* 整片擦除 */
void spi_flash_buffer_erase(uint32_t sector_addr, uint32_t num_byte_to_erase);  /* 擦除指定地址起的若干字节（自动处理扇区边界） */

/* --- 写入 --- */
void spi_flash_page_write(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write);    /* 单页写入（最多256字节） */
void spi_flash_buffer_write(uint8_t *pbuffer, uint32_t write_addr, uint32_t num_byte_to_write);   /* 任意长度写入（自动翻页） */

/* --- 读取 --- */
void spi_flash_buffer_read(uint8_t *pbuffer, uint32_t read_addr, uint16_t num_byte_to_read);      /* 任意长度读取 */
void spi_flash_start_read_sequence(uint32_t read_addr);                   /* 启动连续读序列 */
uint8_t spi_flash_read_byte(void);                                        /* 读取一个字节 */

/* --- ID 和状态 --- */
uint32_t spi_flash_read_id(void);                /* 读取 Flash JEDEC ID（3字节） */
void spi_flash_write_enable(void);               /* 发送写使能命令 */
void spi_flash_wait_for_write_end(void);         /* 等待写操作完成（轮询 WIP 标志） */

/* --- 底层 SPI 通信 --- */
uint8_t spi_flash_send_byte(uint8_t byte);             /* SPI 收发一个字节 */
uint16_t spi_flash_send_halfword(uint16_t half_word);  /* SPI 收发一个半字（16位） */

#endif /* __SPI_FLASH_H */
