/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Key.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/02/16     V0.01    original
************************************************************/

#include "Key.h"

/************************* 内部宏定义 *************************/

#define KEY_PRESSED             1       /* 按下状态 */
#define KEY_UNPRESSED           0       /* 未按下状态 */

#define KEY_TIME_DOUBLE         200     /* 双击判定时间 200ms */
#define KEY_TIME_LONG           2000    /* 长按判定时间 2000ms */
#define KEY_TIME_REPEAT         100     /* 连发间隔 100ms */

/************************* 全局变量 *************************/

uint8_t Key_Flag[KEY_COUNT];            /* 按键事件标志数组 */

/************************************************************
 * 函 数 名: Key_Init
 * 功能说明: 初始化 KEY1~KEY4（PA4~PA7）为上拉输入
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
void Key_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_4);  /* KEY1 - PA4 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_5);  /* KEY2 - PA5 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_6);  /* KEY3 - PA6 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_7);  /* KEY4 - PA7 */
}

/************************************************************
 * 函 数 名: Key_GetState
 * 功能说明: 读取指定按键的当前物理状态
 * 参    数: n - 按键编号（KEY_1 ~ KEY_4）
 * 返 回 值: KEY_PRESSED 或 KEY_UNPRESSED
************************************************************/
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

/************************************************************
 * 函 数 名: Key_Check
 * 功能说明: 检测指定按键是否发生指定事件，读取后自动清除（HOLD 除外）
 * 参    数: n    - 按键编号（KEY_1 ~ KEY_4）
 *           Flag - 事件类型（KEY_SINGLE / KEY_DOUBLE / KEY_LONG 等）
 * 返 回 值: 1 = 事件发生，0 = 未发生
************************************************************/
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

/************************************************************
 * 函 数 名: Key_Tick
 * 功能说明: 按键扫描状态机，由 Timer1 中断每 1ms 调用
 *          实际 GPIO 采样间隔为 20ms（Count 计数 20 次）
 *          状态机流程：0（空闲）→ 1（按下消抖）→ 2/4（等待松开/长按连发）
 * 参    数: 无
 * 返 回 值: 无
************************************************************/
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
    if (Count >= 20)        /* 20ms 采样间隔（Key_Tick 调用周期为 1ms） */
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
                case 0:                                 /* 空闲：等待按下 */
                    if (CurrState[i] == KEY_PRESSED)
                    {
                        Time[i] = KEY_TIME_LONG;
                        S[i] = 1;
                    }
                    break;
                case 1:                                 /* 已按下：等待松开或长按超时 */
                    if (CurrState[i] == KEY_UNPRESSED)
                    {
                        Time[i] = KEY_TIME_DOUBLE;
                        S[i] = 2;                       /* 松开后进入双击等待 */
                    }
                    else if (Time[i] == 0)
                    {
                        Time[i] = KEY_TIME_REPEAT;
                        Key_Flag[i] |= KEY_LONG;
                        S[i] = 4;                       /* 长按触发，进入连发 */
                    }
                    break;
                case 2:                                 /* 双击等待：200ms 内再次按下 */
                    if (CurrState[i] == KEY_PRESSED)
                    {
                        Key_Flag[i] |= KEY_DOUBLE;
                        S[i] = 3;
                    }
                    else if (Time[i] == 0)
                    {
                        Key_Flag[i] |= KEY_SINGLE;      /* 超时未再按，判定为单击 */
                        S[i] = 0;
                    }
                    break;
                case 3:                                 /* 双击后等待松开 */
                    if (CurrState[i] == KEY_UNPRESSED)
                        S[i] = 0;
                    break;
                case 4:                                 /* 连发中：每 100ms 重复触发 */
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
