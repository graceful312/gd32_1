#include "LED.h"



void LED_Init(void)
{
	rcu_periph_clock_enable(LED_CLK);
	gpio_mode_set(GPIOA,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLDOWN,LED3_Pin | LED4_Pin);
	gpio_output_options_set(GPIOA,GPIO_OTYPE_PP,GPIO_OSPEED_2MHZ,LED3_Pin | LED4_Pin);
	
}
