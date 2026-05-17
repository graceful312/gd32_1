#include "Key.h"

#define KEY_PRESSED             1
#define KEY_UNPRESSED           0

#define KEY_TIME_DOUBLE         200
#define KEY_TIME_LONG           2000
#define KEY_TIME_REPEAT         100

uint8_t Key_Flag[KEY_COUNT];

/**
  * @brief  按键 GPIO 初始化
  */
void Key_Init(void)
{
    /* 使能 GPIOA 时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* KEY1 - PA4  上拉输入 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_4);
    /* KEY2 - PA5 上拉输入 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_5);
    /* KEY3 - PA6 下拉输入 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_6);
    /* KEY4 - PA7 下拉输入 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_7);
}

/**
  * @brief  获取按键当前物理状态（按下/抬起）
  */
static uint8_t Key_GetState(uint8_t n)
{
    switch (n)
    {
        case KEY_1:
            return (gpio_input_bit_get(GPIOA, GPIO_PIN_4) == RESET) ? KEY_PRESSED : KEY_UNPRESSED;
        case KEY_2:
            return (gpio_input_bit_get(GPIOA, GPIO_PIN_5) == RESET) ? KEY_PRESSED : KEY_UNPRESSED;
        case KEY_3:
            return (gpio_input_bit_get(GPIOA, GPIO_PIN_6) == RESET) ? KEY_PRESSED : KEY_UNPRESSED;
        case KEY_4:
            return (gpio_input_bit_get(GPIOA, GPIO_PIN_7) == RESET) ? KEY_PRESSED : KEY_UNPRESSED;
        default:
            return KEY_UNPRESSED;
    }
}

/**
  * @brief  检查指定按键的指定事件是否发生
  */
uint8_t Key_Check(uint8_t n, uint8_t Flag)
{
    if (Key_Flag[n] & Flag)
    {
        if (Flag != KEY_HOLD)
            Key_Flag[n] &= ~Flag;
        return 1;
    }
    return 0;
}

/**
  * @brief  按键扫描状态机（需定时调用，如每 1ms 一次）
  */
void Key_Tick(void)
{
    static uint8_t Count, i;
    static uint8_t CurrState[KEY_COUNT], PrevState[KEY_COUNT];
    static uint8_t S[KEY_COUNT];
    static uint16_t Time[KEY_COUNT];

    for (i = 0; i < KEY_COUNT; i++)
    {
        if (Time[i] > 0)
            Time[i]--;
    }

    Count++;
    if (Count >= 20)        // 20ms 消抖周期（假设调用周期为 1ms）
    {
        Count = 0;

        for (i = 0; i < KEY_COUNT; i++)
        {
            PrevState[i] = CurrState[i];
            CurrState[i] = Key_GetState(i);

            if (CurrState[i] == KEY_PRESSED)
                Key_Flag[i] |= KEY_HOLD;
            else
                Key_Flag[i] &= ~KEY_HOLD;

            if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
                Key_Flag[i] |= KEY_DOWN;

            if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
                Key_Flag[i] |= KEY_UP;

            /* 状态机 */
            switch (S[i])
            {
                case 0:
                    if (CurrState[i] == KEY_PRESSED)
                    {
                        Time[i] = KEY_TIME_LONG;
                        S[i] = 1;
                    }
                    break;
                case 1:
                    if (CurrState[i] == KEY_UNPRESSED)
                    {
                        Time[i] = KEY_TIME_DOUBLE;
                        S[i] = 2;
                    }
                    else if (Time[i] == 0)
                    {
                        Time[i] = KEY_TIME_REPEAT;
                        Key_Flag[i] |= KEY_LONG;
                        S[i] = 4;
                    }
                    break;
                case 2:
                    if (CurrState[i] == KEY_PRESSED)
                    {
                        Key_Flag[i] |= KEY_DOUBLE;
                        S[i] = 3;
                    }
                    else if (Time[i] == 0)
                    {
                        Key_Flag[i] |= KEY_SINGLE;
                        S[i] = 0;
                    }
                    break;
                case 3:
                    if (CurrState[i] == KEY_UNPRESSED)
                        S[i] = 0;
                    break;
                case 4:
                    if (CurrState[i] == KEY_UNPRESSED)
                        S[i] = 0;
                    else if (Time[i] == 0)
                    {
                        Time[i] = KEY_TIME_REPEAT;
                        Key_Flag[i] |= KEY_REPEAT;
                    }
                    break;
            }
        }
    }
}
