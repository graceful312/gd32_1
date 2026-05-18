/************************************************************
 * 文件：GD30AD3344.c
 * 功能：GD30AD3344 高精度ADC芯片驱动实现
 *
 *       实现SPI通信、配置寄存器读写、转换数据读取、
 *       初始化、停止和复位等功能。
 *
 * 硬件连接（默认SPI1）：
 *       CS   → PB12（软件控制）
 *       SCK  → PB13
 *       MISO → PB14（兼DRDY中断输入）
 *       MOSI → PB15
 *
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
************************************************************/

#include "GD30AD3344.h"

/* ADC配置缓冲区：用于32位传输模式下的回读数据 */
uint16_t ADC_Config[2] = {0};
/* 全局配置寄存器值，各字段由应用层按位或组合 */
uint16_t AD3344_CONFIG;

/*!
    \brief      初始化SPI外设和GPIO引脚
    \param[in]  none
    \param[out] none
    \retval     none
    \note       配置CS为推挽输出，SCK/MISO/MOSI为SPI复用功能。
                SPI参数：主模式、16位帧、CPOL=0/CPHA=1、MSB优先。
*/
void ad3344_spi_init(void)
{
    /* 开启时钟 */
    rcu_periph_clock_enable(AD3344_GPIO_RCU);
    rcu_periph_clock_enable(AD3344_SPI_RCU);

    /* CS引脚：普通推挽输出 */
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AD3344_CS_PIN);
    gpio_output_options_set(AD3344_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_CS_PIN);

    /* SCK/MISO/MOSI引脚：SPI复用功能 */
    gpio_af_set(AD3344_GPIO_PORT, AD3344_AF, AD3344_SPI_SCK);
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, AD3344_SPI_SCK);
    gpio_output_options_set(AD3344_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_SPI_SCK);

    gpio_af_set(AD3344_GPIO_PORT, AD3344_AF, AD3344_SPI_MISO);
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, AD3344_SPI_MISO);
    gpio_output_options_set(AD3344_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_SPI_MISO);

    gpio_af_set(AD3344_GPIO_PORT, AD3344_AF, AD3344_SPI_MOSI);
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, AD3344_SPI_MOSI);
    gpio_output_options_set(AD3344_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_SPI_MOSI);

    /* SPI参数配置 */
    spi_parameter_struct spi_init_struct;
    spi_i2s_deinit(AD3344_SPI);
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;  /* 全双工 */
    spi_init_struct.device_mode          = SPI_MASTER;                /* 主模式 */
    spi_init_struct.frame_size           = SPI_FRAMESIZE_16BIT;       /* 16位帧 */
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;    /* CPOL=0, CPHA=1 */
    spi_init_struct.nss                  = SPI_NSS_SOFT;              /* 软件NSS */
    spi_init_struct.prescale             = SPI_PSC_32;                /* 32分频 */
    spi_init_struct.endian               = SPI_ENDIAN_MSB;            /* MSB优先 */
    spi_init(AD3344_SPI, &spi_init_struct);

    spi_enable(AD3344_SPI);

    /* CS默认拉高（不选中） */
    SPI_SET_CS();
}

/*!
    \brief      SPI全双工收发16位数据
    \param[in]  tx_byte: 要发送的16位数据
    \param[out] none
    \retval     从机返回的16位数据
*/
uint16_t ad3344_spi_txrx16bit(uint16_t tx_byte)
{
    /* 等待发送缓冲区空 */
    while (RESET == spi_i2s_flag_get(AD3344_SPI, SPI_FLAG_TBE));

    /* 发送数据 */
    spi_i2s_data_transmit(AD3344_SPI, tx_byte);

    /* 等待接收缓冲区非空 */
    while (RESET == spi_i2s_flag_get(AD3344_SPI, SPI_FLAG_RBNE));

    /* 返回接收到的数据 */
    return spi_i2s_data_receive(AD3344_SPI);
}

/*!
    \brief      微秒级粗略延时
    \param[in]  t: 延时时间（微秒），实际精度受主频和编译优化影响
    \param[out] none
    \retval     none
*/
void delay_us(uint32_t t)
{
    uint16_t i;
    while (t--) {
        i = 10;
        while (i--);
    }
}

/*!
    \brief      使能DRDY外部中断（MISO引脚下降沿触发）
    \param[in]  none
    \param[out] none
    \retval     none
    \note       MISO引脚切换为浮空输入模式，配置EXTI下降沿中断。
                转换完成后AD3344拉低DRNY信号触发中断。
*/
void ad3344_exit_enable(void)
{
    /* 使能GPIO和SYSCFG时钟（F4xx EXTI映射需要SYSCFG） */
    rcu_periph_clock_enable(AD3344_GPIO_RCU);
    rcu_periph_clock_enable(RCU_SYSCFG);

    /* MISO引脚设为浮空输入（用作DRDY检测） */
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, AD3344_SPI_MISO);

    /* 将MISO引脚连接到EXTI线路 */
    syscfg_exti_line_config(AD3344_EXTI_PORT, AD3344_EXTI_PIN);

    /* 配置EXTI为下降沿触发的中断模式 */
    exti_init(AD3344_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(AD3344_EXTI_LINE);

    /* 使能EXTI中断 */
    nvic_irq_enable(AD3344_EXTI_IRQn, 2U, 0U);
    exti_interrupt_enable(AD3344_EXTI_LINE);
}

/*!
    \brief      禁用DRDY外部中断，恢复SPI引脚功能
    \param[in]  none
    \param[out] none
    \retval     none
    \note       MISO引脚恢复为SPI复用功能，EXTI中断关闭。
*/
void ad3344_exit_disable(void)
{
    /* 禁用EXTI中断 */
    exti_interrupt_disable(AD3344_EXTI_LINE);
    exti_interrupt_flag_clear(AD3344_EXTI_LINE);
    nvic_irq_disable(AD3344_EXTI_IRQn);

    /* 恢复MISO引脚为SPI复用功能 */
    gpio_af_set(AD3344_GPIO_PORT, AD3344_AF, AD3344_SPI_MISO);
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, AD3344_SPI_MISO);
    gpio_output_options_set(AD3344_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_SPI_MISO);
}

/*!
    \brief      通过SPI发送16位数据并返回从机响应
    \param[in]  config_d: 要发送的16位数据
    \param[out] none
    \retval     从机返回的16位数据
*/
uint16_t AD3344_Send_Data(uint16_t config_d)
{
    uint16_t Data;
    Data = ad3344_spi_txrx16bit(config_d);
    return Data;
}

/*!
    \brief      32位SPI传输模式：发送配置+读取回读值
    \param[in]  config_d: 要写入的16位配置数据
    \param[in]  *config:  指向回读数据的指针
    \param[out] none
    \retval     第一帧收到的数据（通常是转换结果）
*/
uint16_t ad3344_read_data32(uint16_t config_d, uint16_t *config)
{
    uint16_t data;
    data = AD3344_Send_Data(config_d);
    *config = AD3344_Send_Data(0);
    return data;
}

/*!
    \brief      16位SPI传输模式：带CS控制的转换数据读取
    \param[in]  config_d: 要写入的16位配置数据
    \param[out] none
    \retval     接收到的16位转换结果
*/
uint16_t ad3344_read_data16(uint16_t config_d)
{
    uint16_t data;

    SPI_CLR_CS();
    delay_us(1000);

    data = AD3344_Send_Data(config_d);

    SPI_SET_CS();
    delay_us(10000);

    SPI_CLR_CS();

    return data;
}

/*!
    \brief      读取指定地址的寄存器值
    \param[in]  addr: 寄存器地址
      \arg      0x01: 配置寄存器（CONFIG）
    \param[out] none
    \retval     读取到的寄存器值
*/
uint16_t ad3344_read_regs(uint8_t addr)
{
    uint8_t reg_addr = addr;
    uint16_t reg_rtu = 0;

    SPI_CLR_CS();
    delay_us(1000);

    AD3344_Send_Data(reg_addr);
    reg_rtu = AD3344_Send_Data(0x00);

    SPI_SET_CS();
    delay_us(10000);

    SPI_CLR_CS();

    return reg_rtu;
}

/*!
    \brief      GD30AD3344 初始化
    \param[in]  config_d: 组合好的16位配置寄存器值
    \param[out] none
    \retval     none
    \note       将配置值写入CONFIG寄存器，启动ADC转换。
*/
void ad3344_init(uint16_t config_d)
{
    SPI_CLR_CS();
    delay_us(1000);

    AD3344_Send_Data(config_d);

    delay_us(100);

    SPI_SET_CS();
    delay_us(1000);

    SPI_CLR_CS();
    delay_us(1000);
}

/*!
    \brief      停止ADC转换，进入掉电模式
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ad3344_stop_conver(void)
{
    AD3344_CONFIG |= AD3344_REG_CONFIG_MODE_SINGLE;
    AD3344_Send_Data(AD3344_CONFIG);
}

/*!
    \brief      复位GD30AD3344，恢复默认配置
    \param[in]  none
    \param[out] none
    \retval     none
    \note       默认配置：差分AIN0/AIN1，±2.048V，100SPS，单次转换模式。
*/
void ad3344_reset(void)
{
    AD3344_Send_Data(AD3344_CONFIG_DEFAULT);
}

/*!
    \brief      GD30AD3344模块初始化
    \param[in]  none
    \param[out] none
    \retval     none
    \note       初始化SPI外设，配置ADC参数，启动连续转换。
*/
void GD30AD3344_Init(void)
{
    ad3344_spi_init();

    AD3344_CONFIG = 0;
    AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_SINGLE_0;   /* 单端AIN0 */
    AD3344_CONFIG |= AD3344_REG_CONFIG_DR_1000SPS;     /* 1000采样/秒 */
    AD3344_CONFIG |= AD3344_REG_CONFIG_PGA_4_096V;     /* ±4.096V量程 */
    AD3344_CONFIG |= AD3344_REG_CONFIG_PULL_UP_EN;     /* 使能上拉 */
    AD3344_CONFIG |= AD3344_REG_CONFIG_NOP_VALID;      /* 有效写入 */
    AD3344_CONFIG |= AD3344_REG_CONFIG_MODE_CONTIN;    /* 连续转换模式 */

    ad3344_init(AD3344_CONFIG);
}

/*!
    \brief      EXTI10_15中断处理（GD30AD3344 DRDY信号）
    \param[in]  none
    \param[out] none
    \retval     none
    \note       PB14下降沿触发，ADC转换完成。
*/
void EXTI10_15_IRQHandler(void)
{
    if (RESET != exti_interrupt_flag_get(EXTI_14))
    {
        exti_interrupt_flag_clear(EXTI_14);
        /* TODO: 在此添加ADC数据读取和处理逻辑 */
    }
}
