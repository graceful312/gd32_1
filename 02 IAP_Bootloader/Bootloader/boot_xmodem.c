/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：boot_xmodem.c
 * 功能：XMODEM-CRC 文件传输协议接收端实现
 *
 *       协议格式：SOH + 块号 + 块号补码 + 128B 数据 + CRC16_Hi + CRC16_Lo
 *       CRC-16/CCITT：多项式 0x1021，初始值 0x0000
 *       接收方先发送 'C' 请求 CRC 模式
************************************************************/

#include "boot_xmodem.h"
#include "boot_uart.h"
#include "boot_flash.h"
#include "systick.h"

/************************ 协议常量 ************************/

#define XMODEM_SOH   0x01
#define XMODEM_EOT   0x04
#define XMODEM_ACK   0x06
#define XMODEM_NAK   0x15
#define XMODEM_CAN   0x18
#define XMODEM_CRC   'C'           /* 请求 CRC 模式 */

#define BLOCK_SIZE   128            /* XMODEM 数据块大小 */
#define RETRY_MAX    10             /* 最大超时重试次数 */
#define TIMEOUT_MS   3000           /* 每块超时时间（毫秒）*/

/************************ 内部函数 ************************/

/************************************************************
 * 函 数 名: crc16_ccitt
 * 功能说明: 计算 CRC-16/CCITT
 * 参    数: data - 数据指针
 *           len  - 数据长度
 * 返 回 值: CRC16 值
************************************************************/
static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0x0000;

    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

/************************ 公共函数 ************************/

/************************************************************
 * 函 数 名: Boot_Xmodem_Receive
 * 功能说明: 通过 XMODEM-CRC 协议接收固件并写入 Flash
 *
 * 协议流程：
 *   1. 接收方发送 'C' 请求 CRC 模式
 *   2. 发送方回复：SOH + 块号 + ~块号 + 128B 数据 + CRC16
 *   3. 校验块号和 CRC，正确→ACK+写Flash，错误→NAK
 *   4. 发送方发 EOT → 接收方回 ACK → 传输完成
 *
 * 参    数: app_base - 应用程序 Flash 起始地址
 * 返 回 值: >0 = 接收字节数（成功）, -1 = 失败
************************************************************/
int Boot_Xmodem_Receive(uint32_t app_base)
{
    uint8_t block_num = 1;
    uint32_t flash_addr = app_base;
    uint32_t total_bytes = 0;
    int retry;

    /* 发送 'C' 请求 CRC 模式 */
    Boot_UART_SendByte(XMODEM_CRC);

    while (1)
    {
        retry = 0;
        int got_packet = 0;

        /* 等待 SOH 或 EOT，超时则 NAK 重试 */
        while (retry < RETRY_MAX)
        {
            uint32_t timer = TIMEOUT_MS;
            while (timer > 0)
            {
                if (Boot_UART_ByteReady())
                {
                    uint8_t c = Boot_UART_ReceiveByte();
                    if (c == XMODEM_SOH)
                    {
                        got_packet = 1;
                        break;
                    }
                    else if (c == XMODEM_EOT)
                    {
                        /* 传输完成 */
                        Boot_UART_SendByte(XMODEM_ACK);
                        return (int)total_bytes;
                    }
                    else if (c == XMODEM_CAN)
                    {
                        return -1;  /* 发送方取消 */
                    }
                }
                delay_1ms(1);
                timer--;
            }

            if (got_packet)
                break;

            /* 超时，发送 NAK 请求重发 */
            Boot_UART_SendByte(XMODEM_NAK);
            retry++;
        }

        if (!got_packet)
            return -1;  /* 超时次数过多 */

        /* 读取块号（1 字节）+ 块号补码（1 字节） */
        uint8_t blk     = Boot_UART_ReceiveByte();
        uint8_t blk_inv = Boot_UART_ReceiveByte();

        /* 读取 128 字节数据 */
        uint8_t data[BLOCK_SIZE];
        for (int i = 0; i < BLOCK_SIZE; i++)
            data[i] = Boot_UART_ReceiveByte();

        /* 读取 CRC（2 字节，大端序） */
        uint8_t crc_hi = Boot_UART_ReceiveByte();
        uint8_t crc_lo = Boot_UART_ReceiveByte();
        uint16_t recv_crc = ((uint16_t)crc_hi << 8) | crc_lo;

        /* 校验块号 */
        if (blk != block_num || blk_inv != (uint8_t)(~block_num))
        {
            Boot_UART_SendByte(XMODEM_NAK);
            continue;  /* 不递增块号，重试同一块 */
        }

        /* 校验 CRC */
        uint16_t calc_crc = crc16_ccitt(data, BLOCK_SIZE);
        if (calc_crc != recv_crc)
        {
            Boot_UART_SendByte(XMODEM_NAK);
            continue;
        }

        /* 写入 Flash */
        if (Boot_Flash_WriteBlock(flash_addr, data, BLOCK_SIZE) != 0)
        {
            /* Flash 写入失败，发送 CAN 中止传输 */
            Boot_UART_SendByte(XMODEM_CAN);
            Boot_UART_SendByte(XMODEM_CAN);
            return -1;
        }

        flash_addr   += BLOCK_SIZE;
        total_bytes  += BLOCK_SIZE;
        block_num++;

        Boot_UART_SendByte(XMODEM_ACK);
    }
}
