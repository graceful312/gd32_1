#include "Serial.h"

#define PACKAGE_SIZE       100
#define SENSOR_COUNT       8
#define HEADER_CHAR        '$'
#define FOOTER_CHAR        '#'
#define DIGITAL_PREFIX     'D'
#define ANALOG_PREFIX      'A'

// 全局变量（保持不变）
uint8_t calibration_flag = 0;
uint8_t analog_mode_flag = 0;
uint8_t digital_mode_flag = 1;

uint8_t rx_buffer[PACKAGE_SIZE];
uint8_t complete_packet[PACKAGE_SIZE];
volatile uint8_t new_packet_flag = 0;

uint8_t digital_values[SENSOR_COUNT] = {0};
uint16_t analog_values[SENSOR_COUNT] = {0};

/**
  * 函    数：串口初始化（使用 USART0，PA9 TX，PA10 RX）
  * 参    数：无
  * 返 回 值：无
  */
void Serial_Init(void)
{
    /* 开启时钟 */
    rcu_periph_clock_enable(RCU_GPIOC);      // GPIOA 时钟
		rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_USART2);     // USART2 时钟

    /* 配置 GPIO 复用功能 */
    gpio_af_set(GPIOB, GPIO_AF_7,GPIO_PIN_10);   
		gpio_af_set(GPIOC, GPIO_AF_7,GPIO_PIN_5);  

    /* 配置 TX (PB10) 为复用推挽输出 */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* 配置 RX (PC5) 为复用输入，无上拉下拉（相当于浮空） */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);

    /* USART 复位并初始化 */
    usart_deinit(USART2);
    usart_baudrate_set(USART2, 115200U);                     // 波特率 115200
    usart_word_length_set(USART2, USART_WL_8BIT);            // 8 位数据
    usart_stop_bit_set(USART2, USART_STB_1BIT);              // 1 位停止位
    usart_parity_config(USART2, USART_PM_NONE);              // 无校验
    usart_hardware_flow_rts_config(USART2, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART2, USART_CTS_DISABLE);
    usart_receive_config(USART2, USART_RECEIVE_ENABLE);      // 使能接收
    usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);    // 使能发送
    usart_enable(USART2);                                     // 使能 USART0

    /* 使能接收中断 */
    usart_interrupt_enable(USART2, USART_INT_RBNE);           // 接收缓冲区非空中断

    /* NVIC 配置 */
    nvic_irq_enable(USART2_IRQn, 2, 0);                       // 优先级 0，子优先级 0
}

/**
  * 函    数：串口发送一个字节（使用 USART0）
  * 参    数：Byte 要发送的字节
  * 返 回 值：无
  */
void Serial_SendByte(uint8_t Byte)
{
    usart_data_transmit(USART2, Byte);
    while (usart_flag_get(USART2, USART_FLAG_TBE) == RESET);  // 等待发送缓冲区空
}

/**
  * 函    数：串口发送数组
  * 参    数：Array 数组首地址
  * 参    数：Length 数组长度
  * 返 回 值：无
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    for (uint16_t i = 0; i < Length; i++) {
        Serial_SendByte(Array[i]);
    }
}

/**
  * 函    数：串口发送字符串
  * 参    数：String 字符串首地址
  * 返 回 值：无
  */
void Serial_SendString(char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++) {
        Serial_SendByte(String[i]);
    }
}

/**
  * 函    数：次方计算（内部使用）
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) Result *= X;
    return Result;
}

/**
  * 函    数：串口发送数字
  * 参    数：Number 要发送的数字
  * 参    数：Length 数字长度
  * 返 回 值：无
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++) {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
    }
}



/**
  * 函    数：串口接收字节处理
  * 参    数：data 接收到的字节
  * 返 回 值：无
  */
void Serial_Receive_Byte(uint8_t data) {
    static uint8_t receiving = 0;
    static uint16_t index = 0;
    
    switch(receiving) {
        case 0: // 等待包头
            if(data == HEADER_CHAR) {
                rx_buffer[0] = data;
                index = 1;
                receiving = 1;
            }
            break;
            
        case 1: // 接收数据中
            if(index >= PACKAGE_SIZE) {
                receiving = 0;
                index = 0;
                return;
            }
            
            rx_buffer[index++] = data;
            
            if(data == FOOTER_CHAR) {
                memcpy(complete_packet, rx_buffer, index);
                complete_packet[index] = '\0';
                new_packet_flag = 1;
                receiving = 0;
                index = 0;
            }
            break;
    }
}

/**
  * 函    数：处理数字型传感器数据
  * 参    数：无
  * 返 回 值：无
  */
void Deal_Digital_Data(void) {
    if(complete_packet[0] != HEADER_CHAR || 
       complete_packet[1] != DIGITAL_PREFIX) {
        return;
    }
    
    for(uint8_t i = 0; i < SENSOR_COUNT; i++) {
        uint8_t data_pos = 6 + i*5;
        if(data_pos < PACKAGE_SIZE) {
            digital_values[i] = complete_packet[data_pos] - '0';
        }
    }
    
    memset(complete_packet, 0, PACKAGE_SIZE);
    new_packet_flag = 0;
}

/**
  * 函    数：字符串转整数
  */
uint16_t String_To_Int(const char *str) {
    uint16_t result = 0;
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            result = result * 10 + (*str - '0');
        }
        str++;
    }
    return result;
}

/**
  * 函    数：处理模拟型传感器数据
  * 参    数：无
  * 返 回 值：无
  */
void Deal_Analog_Data(void) {
    if(complete_packet[0] != HEADER_CHAR || 
       complete_packet[1] != ANALOG_PREFIX) {
        return;
    }
    
    uint8_t state = 0;
    uint8_t sensor_index = 0;
    char value_buf[6] = {0};
    uint8_t value_index = 0;
    
    for(uint16_t i = 2; i < PACKAGE_SIZE && complete_packet[i] != '\0'; i++) {
        char c = complete_packet[i];
        
        switch(state) {
            case 0:
                if(c == ':') state = 1;
                break;
            case 1:
                if(c >= '0' && c <= '9') {
                    value_buf[value_index++] = c;
                    state = 2;
                }
                break;
            case 2:
                if(c >= '0' && c <= '9') {
                    if(value_index < sizeof(value_buf) - 1) {
                        value_buf[value_index++] = c;
                    }
                } else {
                    value_buf[value_index] = '\0';
                    analog_values[sensor_index++] = String_To_Int(value_buf);
                    value_index = 0;
                    state = (c == ',') ? 0 : 3;
                }
                break;
        }
        
        if(sensor_index >= SENSOR_COUNT) break;
    }
    
    memset(complete_packet, 0, PACKAGE_SIZE);
    new_packet_flag = 0;
}

/**
  * 函    数：USART0 中断服务函数
  * 参    数：无
  * 返 回 值：无
  */
void USART2_IRQHandler(void)
{
    if (usart_interrupt_flag_get(USART2, USART_INT_FLAG_RBNE) != RESET) {
        uint8_t data = usart_data_receive(USART2);
        Serial_Receive_Byte(data);
    }
}
