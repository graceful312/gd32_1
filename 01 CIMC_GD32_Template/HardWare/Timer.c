/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Timer.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#include "Timer.h"

/************************************************************
 * 函 数 名: Timer1_Init
 * 功能说明: 初始化 Timer1 为 1kHz 周期中断
 *          时钟源：168MHz（APB1 x4 倍频）
 *          分频：168 → 1MHz 计数频率
 *          周期：1000 → 1kHz 中断频率
 *          中断中调用 Key_Tick() 驱动按键扫描
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void Timer1_Init(void)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);   /* APB1 总线 42MHz x4 = 168MHz */
    timer_deinit(TIMER1);

    timer_initpara.prescaler         = 167;                 /* 168MHz / (167+1) = 1MHz */
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 999;                 /* 1MHz / (999+1) = 1kHz */
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);       /* NVIC 优先级分组：2 位抢占 + 2 位子 */
    nvic_irq_enable(TIMER1_IRQn, 2, 1);                     /* 抢占优先级 2，子优先级 1 */
    timer_interrupt_enable(TIMER1, TIMER_INT_UP);            /* 使能更新中断 */
    timer_auto_reload_shadow_enable(TIMER1);                 /* 使能自动重载影子寄存器 */
    timer_enable(TIMER1);
}

/************************************************************
 * 函 数 名: TIMER1_IRQHandler
 * 功能说明: Timer1 中断服务函数，每 1ms 调用 Key_Tick()
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void TIMER1_IRQHandler(void)
{
    if (SET == timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_UP))
    {
        timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_UP);
        Key_Tick();
    }
}
