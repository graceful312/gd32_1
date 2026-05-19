/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：LowPower.c
 * 功能：低功耗模式驱动实现
 *
 *       支持 Deep-sleep（深度睡眠）和 Standby（待机）两种模式。
 *       唤醒源：按键 EXTI(PA6)、RTC 唤醒定时器、WKUP 引脚(PA0)。
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
 * 版本：V0.01, 2025/05/19
************************************************************/

#include "LowPower.h"

/************************* 内部函数 *************************/

/************************************************************
 * 函 数 名: lp_prepare_sleep
 * 功能说明: 进入 Deep-sleep 前的外设关闭流程
 *          停止 Timer1、关闭 OLED、DAC、ADC、LED
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
static void lp_prepare_sleep(void)
{
    /* 停止 Timer1（1kHz 按键扫描中断），否则立即唤醒 */
    timer_disable(TIMER1);

    /* 关闭 OLED 充电泵（节省约 10mA） */
    OLED_DisPlay_Off();

    /* 关闭 DAC 输出 */
    dac_disable(DAC0, DAC_OUT0);
    dac_disable(DAC0, DAC_OUT1);

    /* 关闭 ADC */
    adc_disable(ADC0);

    /* 关闭所有 LED */
    LED1_OFF();
    LED2_OFF();
    LED3_OFF();
    LED4_OFF();
}

/************************************************************
 * 函 数 名: lp_resume_sleep
 * 功能说明: 从 Deep-sleep 唤醒后的外设恢复流程
 *          恢复 Timer1、OLED、DAC、ADC
 * 参    数: 无
 * 返 回 值: 无
 * 注    意: pmu_to_deepsleepmode 内部已自动恢复 SysTick 和 NVIC
************************************************************/
static void lp_resume_sleep(void)
{
    /* 恢复 Timer1（按键扫描） */
    timer_enable(TIMER1);

    /* 重新初始化 OLED（充电泵和显示配置需要重新设置） */
    OLED_Init();

    /* 重新使能 DAC */
    dac_enable(DAC0, DAC_OUT0);
    dac_enable(DAC0, DAC_OUT1);

    /* 重新使能 ADC */
    adc_enable(ADC0);
}

/************************************************************
 * 函 数 名: lp_config_exti_key
 * 功能说明: 配置 PA6(KEY3) 为 EXTI 下降沿唤醒源
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
static void lp_config_exti_key(void)
{
    /* 使能 SYSCFG 时钟（EXTI 线映射需要） */
    rcu_periph_clock_enable(RCU_SYSCFG);

    /* 将 EXTI 线 6 映射到 GPIOA（PA6 = KEY3） */
    syscfg_exti_line_config(EXTI_SOURCE_GPIOA, EXTI_SOURCE_PIN6);

    /* 配置 EXTI 线 6 为中断模式，下降沿触发（按键按下为低电平） */
    exti_init(EXTI_6, EXTI_INTERRUPT, EXTI_TRIG_FALLING);

    /* 使能 EXTI5_9 中断（NVIC） */
    nvic_irq_enable(EXTI5_9_IRQn, 2U, 0U);

    /* 使能 EXTI 线 6 中断 */
    exti_interrupt_enable(EXTI_6);
}

/************************* 公共函数 *************************/

/************************************************************
 * 函 数 名: LP_EnterDeepSleep
 * 功能说明: 进入深度睡眠模式
 *          - PLL/IRC16M/HXTAL 停止，LDO 低功耗
 *          - SRAM 和寄存器内容保持
 *          - 唤醒后从本函数下一条指令继续执行
 *          - pmu_to_deepsleepmode 内部自动保存/恢复 SysTick 和 NVIC
 * 参    数: wakeup_src - 唤醒源，LP_WAKEUP_KEY / LP_WAKEUP_RTC / LP_WAKEUP_PIN 的或组合
 *           rtc_sec    - RTC 唤醒间隔（秒），仅 wakeup_src 含 LP_WAKEUP_RTC 时有效
 * 返 回 值: 无
************************************************************/
void LP_EnterDeepSleep(uint8_t wakeup_src, uint16_t rtc_sec)
{
    /* 1. 配置唤醒源 */
    if (wakeup_src & LP_WAKEUP_KEY) {
        lp_config_exti_key();
    }
    if (wakeup_src & LP_WAKEUP_RTC) {
        RTC_SetWakeup(rtc_sec);
    }
    if (wakeup_src & LP_WAKEUP_PIN) {
        pmu_wakeup_pin_enable();
    }

    /* 2. 关闭不需要的外设 */
    lp_prepare_sleep();

    /* 3. 进入深度睡眠（WFI，LDO 低功耗，禁止低驱动） */
    /*    内部自动保存 SysTick + NVIC ISER，唤醒后恢复 */
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_DISABLE, WFI_CMD);

    /* === 唤醒后从这里继续 === */

    /* 4. 恢复外设 */
    lp_resume_sleep();

    /* 5. 清除 EXTI 挂起位（避免残留标志导致误中断） */
    if (wakeup_src & LP_WAKEUP_KEY) {
        exti_flag_clear(EXTI_6);
    }
}

/************************************************************
 * 函 数 名: LP_EnterStandby
 * 功能说明: 进入待机模式（最低功耗）
 *          - LDO 完全关闭，SRAM 和寄存器全部丢失
 *          - 唤醒 = 系统复位，从 main() 重新执行
 *          - 通过 LP_GetWakeupReason() 可判断是否从 Standby 唤醒
 * 参    数: wakeup_src - 唤醒源，LP_WAKEUP_RTC / LP_WAKEUP_PIN 的或组合
 *                                  LP_WAKEUP_KEY 在 Standby 模式不可用
 *           rtc_sec    - RTC 唤醒间隔（秒），仅 wakeup_src 含 LP_WAKEUP_RTC 时有效
 * 返 回 值: 无（不会返回，唤醒后系统复位）
************************************************************/
void LP_EnterStandby(uint8_t wakeup_src, uint16_t rtc_sec)
{
    /* 1. 配置唤醒源 */
    if (wakeup_src & LP_WAKEUP_RTC) {
        RTC_SetWakeup(rtc_sec);
    }
    if (wakeup_src & LP_WAKEUP_PIN) {
        /* PA0 是 WKUP 引脚，也是 LED3，需要先关闭 LED 并设为上拉输入 */
        LED3_OFF();
        gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_0);
        pmu_wakeup_pin_enable();
    }

    /* 2. 关闭 LED（避免 Standby 期间 PA0 误触发 WKUP） */
    LED1_OFF();
    LED2_OFF();
    LED3_OFF();
    LED4_OFF();

    /* 3. 清除上次 Standby 唤醒标志 */
    pmu_flag_clear(PMU_FLAG_RESET_STANDBY);

    /* 4. 进入待机模式（不可逆，以下代码不会执行） */
    pmu_to_standbymode();
}

/************************************************************
 * 函 数 名: LP_GetWakeupReason
 * 功能说明: 检测唤醒来源（在 System_Init 开头调用）
 * 参    数: 无
 * 返 回 值: LP_REASON_POWERON   = 正常上电
 *           LP_REASON_DEEPSLEEP = 从 Deep-sleep 唤醒
 *           LP_REASON_STANDBY   = 从 Standby 唤醒
************************************************************/
uint8_t LP_GetWakeupReason(void)
{
    /* 检查 Standby 唤醒标志 */
    if (RESET != pmu_flag_get(PMU_FLAG_STANDBY)) {
        pmu_flag_clear(PMU_FLAG_RESET_STANDBY);
        return LP_REASON_STANDBY;
    }

    /* 检查 Deep-sleep 唤醒标志 */
    if (RESET != pmu_flag_get(PMU_FLAG_WAKEUP)) {
        pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
        return LP_REASON_DEEPSLEEP;
    }

    return LP_REASON_POWERON;
}

/************************* 中断处理函数 *************************/

/************************************************************
 * 函 数 名: EXTI5_9_IRQHandler
 * 功能说明: EXTI 线 5~9 中断处理（PA6 按键唤醒）
 *          Deep-sleep 唤醒时由硬件自动触发，
 *          pmu_to_deepsleepmode 内部恢复流程后接管返回
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void EXTI5_9_IRQHandler(void)
{
    if (RESET != exti_interrupt_flag_get(EXTI_6)) {
        exti_interrupt_flag_clear(EXTI_6);
    }
}
