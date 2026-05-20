/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：boot_uart.c
 * 功能：Bootloader USART2 轮询驱动
 *
 *       PB10=TX(AF7), PC5=RX(AF7), 115200 8N1
 *       纯轮询模式，不使用中断
************************************************************/

#include "boot_uart.h"

/************************************************************
 * 函 数 名: Boot_UART_Init
 * 功能说明: 初始化 USART2 为轮询收发模式
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void Boot_UART_Init(void)
{
    /* 使能时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_USART2);

    /* GPIO 复用：AF7 = USART2 */
    gpio_af_set(GPIOB, GPIO_AF_7, GPIO_PIN_10);
    gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_5);

    /* TX: PB10, AF 推挽输出 */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* RX: PC5, AF 输入 */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);

    /* USART2 配置 */
    usart_deinit(USART2);
    usart_baudrate_set(USART2, 115200U);
    usart_word_length_set(USART2, USART_WL_8BIT);
    usart_stop_bit_set(USART2, USART_STB_1BIT);
    usart_parity_config(USART2, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART2, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART2, USART_CTS_DISABLE);
    usart_receive_config(USART2, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
    usart_enable(USART2);
}

/************************************************************
 * 函 数 名: Boot_UART_SendByte
 * 功能说明: 发送单个字节（阻塞等待发送完成）
 * 参    数: byte - 要发送的字节
 * 返 回 值: 无
************************************************************/
void Boot_UART_SendByte(uint8_t byte)
{
    usart_data_transmit(USART2, byte);
    while (RESET == usart_flag_get(USART2, USART_FLAG_TBE));
}

/************************************************************
 * 函 数 名: Boot_UART_SendString
 * 功能说明: 发送字符串
 * 参    数: str - 以 '\0' 结尾的字符串
 * 返 回 值: 无
************************************************************/
void Boot_UART_SendString(const char *str)
{
    while (*str)
        Boot_UART_SendByte((uint8_t)*str++);
}

/************************************************************
 * 函 数 名: Boot_UART_ByteReady
 * 功能说明: 检查是否有数据可读
 * 参    数: 无
 * 返 回 值: 1=有数据, 0=无数据
************************************************************/
uint8_t Boot_UART_ByteReady(void)
{
    return (usart_flag_get(USART2, USART_FLAG_RBNE) != RESET) ? 1U : 0U;
}

/************************************************************
 * 函 数 名: Boot_UART_ReceiveByte
 * 功能说明: 阻塞接收单个字节
 * 参    数: 无
 * 返 回 值: 接收到的字节
************************************************************/
uint8_t Boot_UART_ReceiveByte(void)
{
    while (!Boot_UART_ByteReady());
    return (uint8_t)usart_data_receive(USART2);
}
