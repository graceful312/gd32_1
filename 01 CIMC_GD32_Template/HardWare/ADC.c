#include "ADC.h"



void ADC_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOC);
	gpio_mode_set(GPIOC,GPIO_MODE_ANALOG,GPIO_PUPD_NONE,GPIO_PIN_0);
    
    adc_clock_config(ADC_ADCCK_PCLK2_DIV6);
	
}
