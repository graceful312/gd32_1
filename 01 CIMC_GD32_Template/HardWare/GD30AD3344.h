/************************************************************
 * 文件：GD30AD3344.h
 * 功能：GD30AD3344 高精度ADC芯片驱动头文件
 *
 *       GD30AD3344 是一颗 16 位、最高 1000SPS 的 ΔΣ 模数转换器，
 *       内置可编程增益放大器（PGA）和 4 通道多路复用器（MUX），
 *       通过 SPI 接口与 MCU 通信。
 *
 *       寄存器说明：
 *         - CONVERSION (0x00): 转换结果寄存器，16位只读数据
 *         - CONFIG (0x01): 配置寄存器，控制通道选择、PGA增益、
 *                          采样率、工作模式等
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
 * 版本：V1.0.0, 2024
************************************************************/

#ifndef __GD30AD3344_H
#define __GD30AD3344_H

#include "HeaderFiles.h"
#include "GD30AD3344_spi.h"

/* 全局配置寄存器值，由应用层组合各字段后传给初始化函数 */
extern uint16_t AD3344_CONFIG;

/*=========================================================================
 * 输入模式定义
 *   差分输入：两路模拟信号做差分测量，抑制共模干扰
 *   单端输入：每路信号以 GND 为参考，适合绝对电压测量
 */
#define AD3344_DUAL_END         (0)     /* 差分输入模式 */
#define AD3344_SINGLE_END       (1)     /* 单端输入模式 */
/*=========================================================================*/

/*=========================================================================
    GD30AD3344 复位值
*/
#define AD3344_CONVERSION_RESET     ((uint32_t)0x0)     /* 转换寄存器复位值 */
#define AD3344_CONFIG_RESET         ((uint32_t)0x58b)   /* 配置寄存器复位值 */
/*=========================================================================*/

/******************  CONVERSION 寄存器位定义  *********************/
#define AD3344_CONVERSION_CNVDATA_Msk   ((uint32_t)0xffff)  /* 转换数据位掩码 */
#define AD3344_CONVERSION_CNVDATA_Pos   ((uint32_t)0)       /* 转换数据位位置 */

/******************  CONFIG 寄存器位定义  *********************/
#define AD3344_CONFIG_NOP_Msk           ((uint32_t)0x6)     /* NOP字段掩码 */
#define AD3344_CONFIG_NOP_Pos           ((uint32_t)1)       /* NOP字段位置 */
#define AD3344_CONFIG_PULL_UP_EN_Msk    ((uint32_t)0x8)     /* DOUT上拉使能掩码 */
#define AD3344_CONFIG_PULL_UP_EN_Pos    ((uint32_t)3)       /* DOUT上拉使能位置 */
#define AD3344_CONFIG_DR_Msk            ((uint32_t)0xe0)    /* 数据速率掩码 */
#define AD3344_CONFIG_DR_Pos            ((uint32_t)5)       /* 数据速率位置 */
#define AD3344_CONFIG_MODE_Msk          ((uint32_t)0x100)   /* 工作模式掩码 */
#define AD3344_CONFIG_MODE_Pos          ((uint32_t)8)       /* 工作模式位置 */
#define AD3344_CONFIG_PGA_Msk           ((uint32_t)0xe00)   /* PGA增益掩码 */
#define AD3344_CONFIG_PGA_Pos           ((uint32_t)9)       /* PGA增益位置 */
#define AD3344_CONFIG_MUX_Msk           ((uint32_t)0x7000)  /* MUX通道选择掩码 */
#define AD3344_CONFIG_MUX_Pos           ((uint32_t)12)      /* MUX位置 */
#define AD3344_CONFIG_OS_Msk            ((uint32_t)0x8000)  /* 单次转换启动位掩码 */
#define AD3344_CONFIG_OS_Pos            ((uint32_t)15)      /* OS位置 */

/*
 * CONVERSION 寄存器 (地址 0x00)
 * CONV[15:0]: 16位转换结果，补码格式
 */
#define AD3344_CONVERSION_ADDRESS       ((uint8_t) 0x00)
#define AD3344_CONVERSION_DEFAULT       ((uint16_t) 0x0000)
#define AD3344_CONVERSION_CONV_MASK     ((uint16_t) 0xFFFF)

/*
 * CONFIG 寄存器 (地址 0x01)
 * |  SS  | MUX[2:0] | PGA[2:0] | MODE | DR[2:0] | RSV | PULL_UP | NOP[1:0] | RSV |
 */
#define AD3344_CONFIG_ADDRESS           ((uint8_t) 0x01)
#define AD3344_CONFIG_DEFAULT           ((uint16_t) 0x058b)

/* --- OS：启动单次转换 / 状态查询 --- */
#define AD3344_REG_CONFIG_OS_MASK       (0x8000)
#define AD3344_REG_CONFIG_OS_SINGLE     (0x8000)    /* 写：置1启动单次转换 */
#define AD3344_REG_CONFIG_OS_BUSY       (0x0000)    /* 读：0=转换进行中 */
#define AD3344_REG_CONFIG_OS_NOTBUSY    (0x8000)    /* 读：1=设备空闲 */

/* --- MUX：输入通道选择 --- */
#define AD3344_REG_CONFIG_MUX_MASK          (0x7000)
#define AD3344_REG_CONFIG_MUX_DIFF_0_1      (0x0000)    /* 差分 AIN0/AIN1（默认） */
#define AD3344_REG_CONFIG_MUX_DIFF_0_3      (0x1000)    /* 差分 AIN0/AIN3 */
#define AD3344_REG_CONFIG_MUX_DIFF_1_3      (0x2000)    /* 差分 AIN1/AIN3 */
#define AD3344_REG_CONFIG_MUX_DIFF_2_3      (0x3000)    /* 差分 AIN2/AIN3 */
#define AD3344_REG_CONFIG_MUX_SINGLE_0      (0x4000)    /* 单端 AIN0 */
#define AD3344_REG_CONFIG_MUX_SINGLE_1      (0x5000)    /* 单端 AIN1 */
#define AD3344_REG_CONFIG_MUX_SINGLE_2      (0x6000)    /* 单端 AIN2 */
#define AD3344_REG_CONFIG_MUX_SINGLE_3      (0x7000)    /* 单端 AIN3 */

/* --- PGA：可编程增益放大器量程 --- */
#define AD3344_REG_CONFIG_PGA_MASK          (0x0E00)
#define AD3344_REG_CONFIG_PGA_6_144V        (0x0000)    /* ±6.144V，增益 2/3 */
#define AD3344_REG_CONFIG_PGA_4_096V        (0x0200)    /* ±4.096V，增益 1 */
#define AD3344_REG_CONFIG_PGA_2_048V        (0x0400)    /* ±2.048V，增益 2（默认） */
#define AD3344_REG_CONFIG_PGA_1_024V        (0x0600)    /* ±1.024V，增益 4 */
#define AD3344_REG_CONFIG_PGA_0_512V        (0x0800)    /* ±0.512V，增益 8 */
#define AD3344_REG_CONFIG_PGA_0_256V        (0x0A00)    /* ±0.256V，增益 16 */
#define AD3344_REG_CONFIG_PGA_0_064V        (0x0C00)    /* ±0.064V，增益 32 */

/* --- MODE：工作模式 --- */
#define AD3344_REG_CONFIG_MODE_MASK         (0x0100)
#define AD3344_REG_CONFIG_MODE_CONTIN       (0x0000)    /* 连续转换模式 */
#define AD3344_REG_CONFIG_MODE_SINGLE       (0x0100)    /* 单次转换+掉电（默认） */

/* --- DR：数据输出速率 --- */
#define AD3344_REG_CONFIG_DR_MASK           (0x00E0)
#define AD3344_REG_CONFIG_DR_6_25SPS        (0x0000)    /* 6.25 SPS */
#define AD3344_REG_CONFIG_DR_12_5SPS        (0x0020)    /* 12.5 SPS */
#define AD3344_REG_CONFIG_DR_25SPS          (0x0040)    /* 25 SPS */
#define AD3344_REG_CONFIG_DR_50SPS          (0x0060)    /* 50 SPS */
#define AD3344_REG_CONFIG_DR_100SPS         (0x0080)    /* 100 SPS（默认） */
#define AD3344_REG_CONFIG_DR_250SPS         (0x00A0)    /* 250 SPS */
#define AD3344_REG_CONFIG_DR_500SPS         (0x00C0)    /* 500 SPS */
#define AD3344_REG_CONFIG_DR_1000SPS        (0x00E0)    /* 1000 SPS */

/* --- PULL_UP_EN：DOUT引脚上拉 --- */
#define AD3344_REG_CONFIG_PULL_UP_EN_MASK   (0x0008)
#define AD3344_REG_CONFIG_PULL_UP_DIS       (0x0000)    /* 禁用上拉 */
#define AD3344_REG_CONFIG_PULL_UP_EN        (0x0008)    /* 使能上拉（默认） */

/* --- NOP：写操作有效控制 --- */
#define AD3344_REG_CONFIG_NOP_MASK          (0x0006)
#define AD3344_REG_CONFIG_NOP_INV_0         (0x0000)    /* 无效，不更新寄存器 */
#define AD3344_REG_CONFIG_NOP_VALID         (0x0002)    /* 有效，更新寄存器（默认） */
#define AD3344_REG_CONFIG_NOP_INV_1         (0x0004)    /* 无效，不更新寄存器 */
#define AD3344_REG_CONFIG_NOP_INV_2         (0x0006)    /* 无效，不更新寄存器 */

/* --- 保留位 --- */
#define AD3344_CONFIG_RESERVED_MASK         ((uint16_t) 0x0001)
#define AD3344_RESERVED_VALUE               ((uint16_t) 0x0001)

/*****************************************************************************************
                                API 函数声明
*****************************************************************************************/
void delay_us(uint32_t t);                          /* 微秒级粗略延时 */
void ad3344_exit_enable(void);                      /* 使能DRDY外部中断 */
void ad3344_exit_disable(void);                     /* 禁用外部中断，恢复SPI引脚 */
uint16_t AD3344_Send_Data(uint16_t config_d);       /* SPI发送并接收16位数据 */
uint16_t ad3344_read_data32(uint16_t config_d, uint16_t *config);  /* 32位传输模式 */
uint16_t ad3344_read_data16(uint16_t config_d);     /* 16位传输模式读取 */
uint16_t ad3344_read_regs(uint8_t addr);            /* 读寄存器 */
void ad3344_init(uint16_t config_d);                /* 初始化ADC */
void ad3344_stop_conver(void);                      /* 停止转换 */
void ad3344_reset(void);                            /* 复位芯片 */
void GD30AD3344_Init(void);                         /* 模块初始化（SPI+ADC配置） */

#endif /* __GD30AD3344_H */
