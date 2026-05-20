/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：main.c
 * 功能：GD32F470 IAP Bootloader 入口
 *
 *       启动流程：
 *       1. SystemInit()（startup 调用，240MHz 时钟）
 *       2. 检测 KEY3 按键 或 BKP1 标志
 *       3. 进入 Bootloader → USART2 菜单 → XMODEM 上传固件
 *       4. 直接跳转到应用程序（0x08010000）
************************************************************/

#include "gd32f4xx.h"
#include "systick.h"
#include "boot_uart.h"
#include "boot_flash.h"
#include "boot_xmodem.h"

/************************ 配置宏 ************************/

#define APP_ADDR         0x08010000U     /* 应用程序入口地址 */

/* RTC 备份寄存器 1：强制进入 Bootloader 标志 */
#define BOOT_FLAG_ADDR   ((volatile uint32_t *)(RTC_BASE + 0x50U + 1 * 4))  /* BKP1 */
#define BOOT_FLAG_MAGIC  0x424F4F54U     /* "BOOT" */

/************************ 内部函数 ************************/

/************************************************************
 * 函 数 名: jump_to_app
 * 功能说明: 跳转到应用程序
 *           1. 校验 APP_ADDR 处的栈指针是否合法
 *           2. 关闭所有中断，清除 SysTick
 *           3. 设置 VTOR、MSP，跳转到应用的 Reset_Handler
 * 参    数: 无
 * 返 回 值: 无（跳转成功不会返回）
************************************************************/
static void jump_to_app(void)
{
    uint32_t app_stack = *(volatile uint32_t *)APP_ADDR;
    uint32_t app_reset = *(volatile uint32_t *)(APP_ADDR + 4);

    /* 校验栈指针：应指向 SRAM 范围内 */
    if ((app_stack & 0x2FFE0000U) != 0x20000000U)
    {
        Boot_UART_SendString("No valid app found\r\n");
        return;
    }

    /* 关闭所有中断 */
    __disable_irq();

    /* 关闭 SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* 设置向量表偏移到应用程序 */
    SCB->VTOR = APP_ADDR;

    /* 设置主堆栈指针 */
    __set_MSP(app_stack);

    /* 跳转到应用程序 Reset_Handler */
    void (*app_entry)(void) = (void (*)(void))app_reset;
    app_entry();
}

/************************************************************
 * 函 数 名: key_pressed
 * 功能说明: 检测触发按键是否按下
 *           引脚通过 boot_uart.h 中的 BOOT_KEY_xxx 宏配置
 * 参    数: 无
 * 返 回 值: 1=按下, 0=未按下
************************************************************/
static uint8_t key_pressed(void)
{
    /* 使能 GPIO 时钟 */
    rcu_periph_clock_enable(BOOT_KEY_RCU);

    /* 配置为上拉输入 */
    gpio_mode_set(BOOT_KEY_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BOOT_KEY_PIN);

    /* 等待 GPIO 稳定 */
    for (volatile uint32_t i = 0; i < 100000; i++);

#if BOOT_KEY_ACTIVE_LOW
    return (gpio_input_bit_get(BOOT_KEY_GPIO, BOOT_KEY_PIN) == RESET) ? 1U : 0U;
#else
    return (gpio_input_bit_get(BOOT_KEY_GPIO, BOOT_KEY_PIN) != RESET) ? 1U : 0U;
#endif
}

/************************************************************
 * 函 数 名: boot_flag_set
 * 功能说明: 检查备份寄存器 BKP1 中的强制 Bootloader 标志
 *           应用可通过写入 0x424F4F54 + NVIC_SystemReset() 进入
 * 参    数: 无
 * 返 回 值: 1=需要进入 Bootloader, 0=正常启动
************************************************************/
static uint8_t boot_flag_set(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    uint32_t flag = *BOOT_FLAG_ADDR;
    if (flag == BOOT_FLAG_MAGIC)
    {
        /* 清除标志，防止循环重启 */
        *BOOT_FLAG_ADDR = 0;
        return 1U;
    }
    return 0U;
}

/************************ 主函数 ************************/

int main(void)
{
    /* SystemInit() 已由 startup 代码调用（240MHz 时钟配置） */

    systick_config();       /* 1ms SysTick */

    /* 判断是否进入 Bootloader 模式 */
    if (key_pressed() || boot_flag_set())
    {
        /* ====== Bootloader 模式 ====== */
        Boot_UART_Init();

        Boot_UART_SendString("\r\n");
        Boot_UART_SendString("=============================\r\n");
        Boot_UART_SendString(" GD32F470 IAP Bootloader\r\n");
        Boot_UART_SendString("=============================\r\n");
        Boot_UART_SendString(" [1] XMODEM Upload Firmware\r\n");
        Boot_UART_SendString(" [2] Jump to Application\r\n");
        Boot_UART_SendString(" Waiting 5s for choice...\r\n");

        /* 等待用户选择，超时 5 秒 */
        uint32_t timeout = 5000;
        uint8_t choice = 0;

        while (timeout > 0)
        {
            if (Boot_UART_ByteReady())
            {
                choice = Boot_UART_ReceiveByte();
                if (choice == '1' || choice == '2')
                    break;
            }
            delay_1ms(1);
            timeout--;
        }

        if (choice == '1')
        {
            Boot_UART_SendString("\r\nErasing application area...\r\n");

            if (Boot_Flash_EraseApp() != 0)
            {
                Boot_UART_SendString("Erase FAILED!\r\n");
                jump_to_app();
            }

            Boot_UART_SendString("Erase OK. Send file via XMODEM-CRC...\r\n");

            int result = Boot_Xmodem_Receive(APP_ADDR);
            if (result > 0)
            {
                Boot_UART_SendString("\r\nUpload OK (");
                /* 简单打印字节数 */
                char buf[16];
                int n = result;
                int i = 0;
                if (n == 0) { buf[i++] = '0'; }
                else { while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; } }
                /* 反转 */
                for (int j = 0; j < i / 2; j++) { char t = buf[j]; buf[j] = buf[i-1-j]; buf[i-1-j] = t; }
                buf[i] = '\0';
                Boot_UART_SendString(buf);
                Boot_UART_SendString(" bytes)\r\n");
                Boot_UART_SendString("Jumping to application...\r\n");
            }
            else
            {
                Boot_UART_SendString("\r\nUpload FAILED!\r\n");
            }

            jump_to_app();
        }
        else
        {
            /* 选择 '2' 或超时 */
            Boot_UART_SendString("\r\nJumping to application...\r\n");
            jump_to_app();
        }
    }
    else
    {
        /* ====== 直接跳转应用程序 ====== */
        jump_to_app();
    }

    /* 不应执行到这里 */
    while (1) {}
}
