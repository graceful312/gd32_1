/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：RTC.c
 * 功能：RTC 实时时钟模块实现
 *
 *       基于 GD32F470VET6 内置 RTC 外设，使用 GD32F4xx SPL 库。
 *       默认使用外部 32.768kHz LXTAL 晶振作为时钟源，
 *       预分频器 syn=0xFF, asyn=0x7F，产生精确 1Hz 基准时钟。
 *
 *       时间/日期寄存器使用 BCD 编码：
 *         十进制 25 → BCD 0x25，十进制 59 → BCD 0x59
 *
 * 硬件需求：PC14/PC15 焊接 32.768kHz 晶振（LXTAL）
 * 平台：GD32F470VET6 (CIMC IHD V0.4)
************************************************************/

#include "RTC.h"

/************************* 全局变量 *************************/

/* RTC初始化参数结构体 */
rtc_parameter_struct rtc_time_para;

/* 闹钟结构体（闹钟0和闹钟1共用） */
rtc_alarm_struct rtc_alarm_para;

/* 预分频器值（由rtc_pre_config根据时钟源设置） */
static __IO uint32_t prescaler_a = 0;   /* 异步预分频 */
static __IO uint32_t prescaler_s = 0;   /* 同步预分频 */

/* RTC时钟源标志 */
static uint32_t rtcsrc_flag = 0;

/************************ 内部函数声明 ************************/

static void rtc_pre_config(void);       /* 时钟源配置 */
static void rtc_first_setup(void);      /* 首次时间配置（默认值） */

/************************ 函数实现 ************************/

/*!
    \brief      十进制转BCD码
    \param[in]  dec: 十进制数（0~99）
    \param[out] none
    \retval     BCD码
    \note       例如：dec=25 → 返回 0x25
*/
uint8_t RTC_DecToBCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

/*!
    \brief      BCD码转十进制
    \param[in]  bcd: BCD码
    \param[out] none
    \retval     十进制数
    \note       例如：bcd=0x25 → 返回 25
*/
uint8_t RTC_BCDToDec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/*!
    \brief      RTC时钟源和预分频器配置（内部函数）
    \param[in]  none
    \param[out] none
    \retval     none
    \note       根据 RTC_CLOCK_SOURCE_LXTAL / RTC_CLOCK_SOURCE_IRC32K 宏选择时钟源。
                LXTAL: 32768Hz / (255+1) / (127+1) = 1Hz
                IRC32K: ~32000Hz / (319+1) / (99+1) ≈ 1Hz
*/
static void rtc_pre_config(void)
{
#if defined (RTC_CLOCK_SOURCE_LXTAL)
    /* 使能外部32.768kHz低速晶振，等待其稳定 */
    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    /* 选择LXTAL作为RTC时钟源 */
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
    /* 预分频：syn=0xFF(255), asyn=0x7F(127) → 32768/(256*128)=1Hz */
    prescaler_s = 0xFF;
    prescaler_a = 0x7F;

#elif defined (RTC_CLOCK_SOURCE_IRC32K)
    /* 使能内部32kHz RC振荡器，等待其稳定 */
    rcu_osci_on(RCU_IRC32K);
    rcu_osci_stab_wait(RCU_IRC32K);
    /* 选择IRC32K作为RTC时钟源 */
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
    /* 预分频：syn=0x13F(319), asyn=0x63(99) → ~32000/(320*100)=1Hz */
    prescaler_s = 0x13F;
    prescaler_a = 0x63;

#else
    #error "请定义 RTC_CLOCK_SOURCE_LXTAL 或 RTC_CLOCK_SOURCE_IRC32K"
#endif

    /* 使能RTC外设时钟 */
    rcu_periph_clock_enable(RCU_RTC);
    /* 等待RTC寄存器同步（确保影子寄存器数据有效） */
    rtc_register_sync_wait();
}

/*!
    \brief      首次配置RTC时间（内部函数）
    \param[in]  none
    \param[out] none
    \retval     none
    \note       使用默认时间 2025-01-01 00:00:00 初始化RTC。
                实际使用时应调用 RTC_SetTime() 设置正确时间。
*/
static void rtc_first_setup(void)
{
    rtc_time_para.factor_asyn  = prescaler_a;
    rtc_time_para.factor_syn   = prescaler_s;
    rtc_time_para.year         = 0x25;           /* 2025年（BCD） */
    rtc_time_para.month        = RTC_JAN;        /* 1月 */
    rtc_time_para.date         = 0x01;           /* 1日（BCD） */
    rtc_time_para.day_of_week  = RTC_WEDNESDAY;  /* 星期三 */
    rtc_time_para.hour         = 0x00;           /* 0时（BCD） */
    rtc_time_para.minute       = 0x00;           /* 0分（BCD） */
    rtc_time_para.second       = 0x00;           /* 0秒（BCD） */
    rtc_time_para.display_format = RTC_24HOUR;   /* 24小时制 */
    rtc_time_para.am_pm        = RTC_AM;

    /* 写入RTC寄存器（进入配置模式 → 写预分频+时间 → 退出配置模式） */
    if (ERROR == rtc_init(&rtc_time_para)) {
        printf("\r\n[RTC] 首次配置失败!\r\n");
    } else {
        printf("\r\n[RTC] 首次配置成功: 2025-01-01 00:00:00\r\n");
        /* 写入标记值到备份寄存器0，表示RTC已配置过 */
        RTC_BKP0 = RTC_BKP_FLAG;
    }
}

/*!
    \brief      RTC初始化（入口函数）
    \param[in]  none
    \param[out] none
    \retval     none
    \note       初始化流程：
                1. 使能PMU时钟，开放备份域写保护
                2. 配置RTC时钟源（LXTAL或IRC32K）
                3. 读取BKP0判断RTC是否已配置过
                   - 未配置：使用默认时间初始化
                   - 已配置：RTC保持运行，读取当前时间
                4. 清除所有复位标志
*/
void RTC_Init(void)
{
    /* 使能PMU（电源管理单元）时钟 */
    rcu_periph_clock_enable(RCU_PMU);
    /* 允许访问备份域寄存器（RTC寄存器和BKP寄存器属于备份域） */
    pmu_backup_write_enable();

    /* 配置RTC时钟源和预分频器 */
    rtc_pre_config();

    /* 读取RCU_BDCTL寄存器的RTC时钟源选择位 [9:8] */
    rtcsrc_flag = GET_BITS(RCU_BDCTL, 8, 9);

    /* 检查RTC是否已配置过（通过备份寄存器0的标记值判断） */
    if ((RTC_BKP_FLAG != RTC_BKP0) || (0x00 == rtcsrc_flag)) {
        /* 首次运行或VBAT掉电后数据丢失，需要重新配置 */
        rtc_first_setup();
    } else {
        /* RTC已配置过且时钟源有效，无需重新配置 */
        /* 检测复位来源 */
        if (RESET != rcu_flag_get(RCU_FLAG_PORRST)) {
            printf("[RTC] 上电复位，RTC保持运行\r\n");
        } else if (RESET != rcu_flag_get(RCU_FLAG_EPRST)) {
            printf("[RTC] 外部复位，RTC保持运行\r\n");
        }
        /* 读取并显示当前时间 */
        RTC_PrintTime();
    }

    /* 清除所有复位标志 */
    rcu_all_reset_flag_clear();

    /* 使能RTC闹钟和唤醒中断的NVIC */
    nvic_irq_enable(RTC_Alarm_IRQn, 3U, 0U);   /* 抢占3，子0 */
    nvic_irq_enable(RTC_WKUP_IRQn,  3U, 0U);   /* 抢占3，子0 */
}

/*!
    \brief      程序化设置RTC时间
    \param[in]  year:   年（十进制，0~99，表示2000~2099）
    \param[in]  month:  月（十进制，1~12）
    \param[in]  date:   日（十进制，1~31）
    \param[in]  hour:   时（十进制，0~23）
    \param[in]  minute: 分（十进制，0~59）
    \param[in]  second: 秒（十进制，0~59）
    \param[out] none
    \retval     none
    \note       接受十进制参数，内部自动转换为BCD码。
                调用此函数会重新配置整个RTC（进入配置模式）。
                设置成功后更新BKP0标记。
*/
void RTC_SetTime(uint8_t year, uint8_t month, uint8_t date,
                 uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_time_para.factor_asyn    = prescaler_a;
    rtc_time_para.factor_syn     = prescaler_s;
    rtc_time_para.display_format = RTC_24HOUR;
    rtc_time_para.am_pm          = RTC_AM;
    rtc_time_para.day_of_week    = RTC_SATURDAY;  /* 星期几暂不计算 */

    /* 十进制 → BCD 转换 */
    rtc_time_para.year   = RTC_DecToBCD(year);
    rtc_time_para.month  = RTC_DecToBCD(month);
    rtc_time_para.date   = RTC_DecToBCD(date);
    rtc_time_para.hour   = RTC_DecToBCD(hour);
    rtc_time_para.minute = RTC_DecToBCD(minute);
    rtc_time_para.second = RTC_DecToBCD(second);

    /* 写入RTC */
    if (ERROR == rtc_init(&rtc_time_para)) {
        printf("[RTC] 时间设置失败!\r\n");
    } else {
        printf("[RTC] 时间已设置: 20%02X-%02X-%02X %02X:%02X:%02X\r\n",
               rtc_time_para.year, rtc_time_para.month, rtc_time_para.date,
               rtc_time_para.hour, rtc_time_para.minute, rtc_time_para.second);
        RTC_BKP0 = RTC_BKP_FLAG;
    }
}

/*!
    \brief      读取当前RTC时间
    \param[in]  none
    \param[out] time: 指向 rtc_parameter_struct 结构体的指针
    \retval     none
    \note       读取 RTC_TIME 和 RTC_DATE 寄存器，
                结果为BCD编码，可用 RTC_BCDToDec() 转为十进制。
*/
void RTC_GetTime(rtc_parameter_struct *time)
{
    rtc_current_time_get(time);
}

/*!
    \brief      串口打印当前时间
    \param[in]  none
    \param[out] none
    \retval     none
    \note       格式：20YY-MM-DD HH:MM:SS（BCD值直接格式化输出）
*/
void RTC_PrintTime(void)
{
    rtc_current_time_get(&rtc_time_para);

    printf("[RTC] 当前时间: 20%02X-%02X-%02X %02X:%02X:%02X\r\n",
           rtc_time_para.year, rtc_time_para.month, rtc_time_para.date,
           rtc_time_para.hour, rtc_time_para.minute, rtc_time_para.second);
}

/*!
    \brief      配置闹钟0
    \param[in]  date:   日期（BCD编码，0x01~0x31）
    \param[in]  hour:   时（BCD编码，0x00~0x23）
    \param[in]  minute: 分（BCD编码，0x00~0x59）
    \param[in]  second: 秒（BCD编码，0x00~0x59）
    \param[out] none
    \retval     none
    \note       默认屏蔽日期字段（每天触发），使能闹钟0中断。
                闹钟触发时会置位 ALRM0IF 标志。
*/
void RTC_SetAlarm0(uint8_t date, uint8_t hour, uint8_t minute, uint8_t second)
{
    /* 先禁用闹钟0（需等待写入完成标志） */
    rtc_alarm_disable(RTC_ALARM0);

    /* 配置闹钟参数 */
    rtc_alarm_para.alarm_mask       = RTC_ALARM_NONE_MASK;  /* 不屏蔽任何字段，精确匹配 */
    rtc_alarm_para.weekday_or_date  = RTC_ALARM_DATE_SELECTED;
    rtc_alarm_para.alarm_day        = date;
    rtc_alarm_para.alarm_hour       = hour;
    rtc_alarm_para.alarm_minute     = minute;
    rtc_alarm_para.alarm_second     = second;
    rtc_alarm_para.am_pm            = RTC_AM;

    /* 写入闹钟0寄存器 */
    rtc_alarm_config(RTC_ALARM0, &rtc_alarm_para);
    /* 使能闹钟0中断 */
    rtc_interrupt_enable(RTC_INT_ALARM0);
    /* 使能闹钟0 */
    rtc_alarm_enable(RTC_ALARM0);

    printf("[RTC] 闹钟0已设置: %02X:%02X:%02X\r\n", hour, minute, second);
}

/*!
    \brief      配置闹钟1
    \param[in]  date:   日期（BCD编码）
    \param[in]  hour:   时（BCD编码）
    \param[in]  minute: 分（BCD编码）
    \param[in]  second: 秒（BCD编码）
    \param[out] none
    \retval     none
*/
void RTC_SetAlarm1(uint8_t date, uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_alarm_disable(RTC_ALARM1);

    rtc_alarm_para.alarm_mask       = RTC_ALARM_NONE_MASK;
    rtc_alarm_para.weekday_or_date  = RTC_ALARM_DATE_SELECTED;
    rtc_alarm_para.alarm_day        = date;
    rtc_alarm_para.alarm_hour       = hour;
    rtc_alarm_para.alarm_minute     = minute;
    rtc_alarm_para.alarm_second     = second;
    rtc_alarm_para.am_pm            = RTC_AM;

    rtc_alarm_config(RTC_ALARM1, &rtc_alarm_para);
    rtc_interrupt_enable(RTC_INT_ALARM1);
    rtc_alarm_enable(RTC_ALARM1);

    printf("[RTC] 闹钟1已设置: %02X:%02X:%02X\r\n", hour, minute, second);
}

/*!
    \brief      串口打印闹钟0时间
    \param[in]  none
    \param[out] none
    \retval     none
*/
void RTC_PrintAlarm0(void)
{
    rtc_alarm_get(RTC_ALARM0, &rtc_alarm_para);
    printf("[RTC] 闹钟0: %02X:%02X:%02X\r\n",
           rtc_alarm_para.alarm_hour, rtc_alarm_para.alarm_minute, rtc_alarm_para.alarm_second);
}

/*!
    \brief      串口打印闹钟1时间
    \param[in]  none
    \param[out] none
    \retval     none
*/
void RTC_PrintAlarm1(void)
{
    rtc_alarm_get(RTC_ALARM1, &rtc_alarm_para);
    printf("[RTC] 闹钟1: %02X:%02X:%02X\r\n",
           rtc_alarm_para.alarm_hour, rtc_alarm_para.alarm_minute, rtc_alarm_para.alarm_second);
}

/*!
    \brief      设置唤醒定时器
    \param[in]  count: 唤醒计数值（1~65535），单位为秒
    \param[out] none
    \retval     none
    \note       唤醒时钟源选择 ck_spre（即1Hz），
                每隔 count 秒触发一次 RTC_WAKEUP 中断。
                适合周期性唤醒（如低功耗定时采集）。
*/
void RTC_SetWakeup(uint16_t count)
{
    /* 先禁用唤醒定时器 */
    rtc_wakeup_disable();
    /* 选择唤醒时钟源为 ck_spre（1Hz，由预分频器产生） */
    rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    /* 设置唤醒计数值 */
    rtc_wakeup_timer_set(count);
    /* 使能唤醒中断 */
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    /* 使能唤醒定时器 */
    rtc_wakeup_enable();

    printf("[RTC] 唤醒定时器已设置: %d 秒\r\n", count);
}

/*!
    \brief      写备份寄存器
    \param[in]  reg:   寄存器编号（0~19）
    \param[in]  value: 要写入的32位值
    \param[out] none
    \retval     none
    \note       备份寄存器由VBAT供电，主电源掉电后数据不丢失。
                适合保存校准参数、运行状态标记等。
*/
void RTC_WriteBackup(uint8_t reg, uint32_t value)
{
    if (reg <= 19) {
        /* 备份寄存器从 RTC_BKP0（偏移0x50）开始，每个占4字节 */
        *(volatile uint32_t *)(RTC_BASE + 0x50U + reg * 4) = value;
    }
}

/*!
    \brief      读备份寄存器
    \param[in]  reg: 寄存器编号（0~19）
    \param[out] none
    \retval     读取到的32位值
*/
uint32_t RTC_ReadBackup(uint8_t reg)
{
    if (reg <= 19) {
        return *(volatile uint32_t *)(RTC_BASE + 0x50U + reg * 4);
    }
    return 0;
}

/************************* 中断处理函数 *************************/

/*!
    \brief      RTC闹钟中断处理函数
    \param[in]  none
    \param[out] none
    \retval     none
    \note       闹钟0或闹钟1触发时进入。
                清除中断标志后，可在下方添加用户处理逻辑。
*/
void RTC_Alarm_IRQHandler(void)
{
    /* 检查闹钟0中断标志 */
    if (RESET != rtc_flag_get(RTC_FLAG_ALRM0)) {
        rtc_flag_clear(RTC_FLAG_ALRM0);
        /* TODO: 闹钟0触发，添加用户处理逻辑 */
    }

    /* 检查闹钟1中断标志 */
    if (RESET != rtc_flag_get(RTC_FLAG_ALRM1)) {
        rtc_flag_clear(RTC_FLAG_ALRM1);
        /* TODO: 闹钟1触发，添加用户处理逻辑 */
    }
}

/*!
    \brief      RTC唤醒定时器中断处理函数
    \param[in]  none
    \param[out] none
    \retval     none
    \note       唤醒定时器溢出时进入（周期由 RTC_SetWakeup 设定）。
                清除中断标志后，可在下方添加用户处理逻辑。
*/
void RTC_WKUP_IRQHandler(void)
{
    if (RESET != rtc_flag_get(RTC_FLAG_WT)) {
        rtc_flag_clear(RTC_FLAG_WT);
        /* TODO: 唤醒定时器触发，添加用户处理逻辑 */
    }
}
