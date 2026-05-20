/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：gd32f4xx_it.c
 * 功能：Bootloader 中断处理函数（仅 SysTick）
************************************************************/

#include "gd32f4xx.h"
#include "systick.h"

void SysTick_Handler(void)
{
    delay_decrement();
}

void NMI_Handler(void)          {}
void HardFault_Handler(void)    { while (1); }
void MemManage_Handler(void)    { while (1); }
void BusFault_Handler(void)     { while (1); }
void UsageFault_Handler(void)   { while (1); }
void SVC_Handler(void)          {}
void DebugMon_Handler(void)     {}
void PendSV_Handler(void)       {}
