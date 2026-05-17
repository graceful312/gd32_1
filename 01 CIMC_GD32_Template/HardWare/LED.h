#ifndef __LED_H
#define __LED_H
#include "HeaderFiles.h"

#define LED_CLK RCU_GPIOA
#define LED_Port GPIOA
#define LED1_Pin GPIO_PIN_4
#define LED2_Pin GPIO_PIN_5
#define LED3_Pin GPIO_PIN_0
#define LED4_Pin GPIO_PIN_1

#define LED1_ON() gpio_bit_set(LED_Port,LED1_Pin)
#define LED1_OFF() gpio_bit_reset(LED_Port,LED1_Pin)

#define LED2_ON() gpio_bit_set(LED_Port,LED2_Pin)
#define LED2_OFF() gpio_bit_reset(LED_Port,LED2_Pin)

#define LED3_ON() gpio_bit_set(LED_Port,LED3_Pin)
#define LED3_OFF() gpio_bit_reset(LED_Port,LED3_Pin)

#define LED4_ON() gpio_bit_set(LED_Port,LED4_Pin)
#define LED4_OFF() gpio_bit_reset(LED_Port,LED4_Pin)

void LED_Init(void);
#endif
