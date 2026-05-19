/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Serial.h
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#ifndef __SERIAL_H
#define __SERIAL_H

#include "gd32f4xx.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "HeaderFiles.h"

/* 函数声明 */
void Serial_Init(void);                                // 串口初始化（USART2，PB10 TX，PC5 RX）
void Serial_SendByte(uint8_t Byte);                    // 发送单字节
void Serial_SendArray(uint8_t *Array, uint16_t Length); // 发送数组
void Serial_SendString(char *String);                  // 发送字符串
void Serial_SendNumber(uint32_t Number, uint8_t Length); // 发送数字
void Serial_Receive_Byte(uint8_t data);                // 接收字节处理
void Deal_Digital_Data(void);                          // 处理数字传感器数据
void Deal_Analog_Data(void);                           // 处理模拟传感器数据

/* 全局变量声明 */
extern volatile uint8_t new_packet_flag;               // 新数据包标志
extern uint8_t digital_values[8];                      // 数字传感器值
extern uint16_t analog_values[8];                      // 模拟传感器值
extern uint8_t complete_packet[];                      // 完整数据包

#endif
