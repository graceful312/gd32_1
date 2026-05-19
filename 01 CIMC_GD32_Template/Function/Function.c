#include "Function.h"

void System_Init(void)
{
	systick_config();     // 时钟配置
	Key_Init();
	LED_Init();

	OLED_Init();
	Serial_Init();
    Timer1_Init();
    GD30AD3344_Init();    // GD30AD3344外部ADC初始化
	RTC_Init();           // RTC实时时钟初始化
    spi_flash_init();     // 外部 SPI Flash 初始化
    FatFs_Init();         // FatFs 文件系统初始化
}


void UsrFunction(void)
{

    while(1)
   {

        OLED_Refresh();

    }

}
