/************************************************************
 * 文件：GD30AD3344_spi.h
 * 功能：GD30AD3344 SPI硬件抽象层
 *       所有硬件参数集中在此文件定义，修改宏即可切换SPI端口和引脚
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
************************************************************/

#ifndef __GD30AD3344_SPI_H
#define __GD30AD3344_SPI_H

#include "HeaderFiles.h"

/**************************** SPI 端口配置（修改此处即可切换） ****************************/

#define AD3344_SPI              SPI1                /* SPI外设：SPI0/SPI1/SPI2 */
#define AD3344_SPI_RCU          RCU_SPI1            /* SPI时钟使能 */
#define AD3344_GPIO_RCU         RCU_GPIOB           /* GPIO时钟使能 */
#define AD3344_GPIO_PORT        GPIOB               /* GPIO端口 */

#define AD3344_SPI_SCK          GPIO_PIN_13         /* SCK  引脚 */
#define AD3344_SPI_MISO         GPIO_PIN_14         /* MISO 引脚（兼DRDY） */
#define AD3344_SPI_MOSI         GPIO_PIN_15         /* MOSI 引脚 */
#define AD3344_CS_PIN           GPIO_PIN_12         /* CS   片选引脚 */

#define AD3344_AF               GPIO_AF_5           /* SPI1复用功能编号 */
#define AD3344_EXTI_PORT        EXTI_SOURCE_GPIOB   /* EXTI端口源 */
#define AD3344_EXTI_PIN         EXTI_SOURCE_PIN14   /* EXTI引脚源（MISO/DRDY） */
#define AD3344_EXTI_LINE        EXTI_14             /* EXTI线路 */
#define AD3344_EXTI_IRQn        EXTI10_15_IRQn      /* EXTI中断号 */

/**************************** CS 片选控制宏 ****************************/

/* CS拉高：取消选中从机 */
#define SPI_SET_CS()    GPIO_BOP(AD3344_GPIO_PORT) = AD3344_CS_PIN
/* CS拉低：选中从机 */
#define SPI_CLR_CS()    GPIO_BC(AD3344_GPIO_PORT) = AD3344_CS_PIN

/**************************** 函数声明 ****************************/

/* 初始化SPI外设和GPIO引脚 */
void ad3344_spi_init(void);
/* SPI全双工收发16位数据 */
uint16_t ad3344_spi_txrx16bit(uint16_t tx_byte);

#endif /* __GD30AD3344_SPI_H */
