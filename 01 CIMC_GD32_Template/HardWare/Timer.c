#include "Timer.h"

void Timer1_Init(void)
{
    /* -----------------------------------------------------------------------
	  系统主频168MHZ,timer_initpara.prescaler为167，timer_initpara.period为999，频率就为1KHZ
    ----------------------------------------------------------------------- */
    timer_parameter_struct timer_initpara;
    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);//AP1总线最高42MHZ,所以TIME1到168M需要4倍频
    timer_deinit(TIMER1);
    /* TIMER1 configuration */
    timer_initpara.prescaler         = 167;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 999;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1,&timer_initpara);
	nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
	nvic_irq_enable(TIMER1_IRQn, 2, 1);
	timer_interrupt_enable(TIMER1, TIMER_INT_UP);
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER1);
    /* auto-reload preload enable */
    timer_enable(TIMER1);
}

//定时器1中断服务程序，1ms中断一次
void TIMER1_IRQHandler(void)
{
	if(SET == timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_UP))//读取到溢出中断
	{
        if (timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_UP) == SET)
        {
            timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_UP);
            Key_Tick();
        }
	}
}
