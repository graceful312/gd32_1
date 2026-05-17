#include "Function.h"

void System_Init(void)
{
	systick_config();     // 时钟配置
	Key_Init();
	LED_Init();
	
	OLED_Init();
	Serial_Init();
    Timer1_Init();
}


void UsrFunction(void)
{
    
    while(1)
   {   

        OLED_Refresh();
        
    }

}
