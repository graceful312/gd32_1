#ifndef __SERIAL_H
#define __SERIAL_H
#include "gd32f4xx.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "HeaderFiles.h"
// 函数声明
void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_Receive_Byte(uint8_t data);
void Deal_Digital_Data(void);
void Deal_Analog_Data(void);

// 全局变量声明
extern volatile uint8_t new_packet_flag;
extern uint8_t digital_values[8];
extern uint16_t analog_values[8];
extern uint8_t complete_packet[];

#endif
