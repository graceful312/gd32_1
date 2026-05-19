#include "Function.h"
#include "ff.h"           /* FatFs 文件系统 API */
#include "diskio.h"       /* FatFs 底层磁盘 I/O */

/* FatFs 全局对象 */
static FATFS fatfs;        /* 文件系统对象 */

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

/*!
    \brief      FatFs 文件系统初始化
    \param[in]  none
    \param[out] none
    \retval     none
    \note       初始化流程：
                1. 初始化磁盘（SPI Flash）
                2. 尝试挂载文件系统
                3. 如果挂载失败（Flash 未格式化），自动格式化后重新挂载
*/
void FatFs_Init(void)
{
    FRESULT res;
    uint8_t retry = 3;

    /* 初始化磁盘驱动 */
    while (retry--) {
        if (disk_initialize(0) == 0) {
            break;
        }
    }

    /* 尝试挂载文件系统 */
    res = f_mount(0, &fatfs);
    if (res == FR_NO_FILESYSTEM) {
        /* Flash 尚无文件系统，执行格式化 */
        printf("[FatFs] No filesystem, formatting...\r\n");
        res = f_mkfs(0, 0, 0);     /* 默认格式化：驱动器0, FDISK分区, 自动分配单元 */
        if (res == FR_OK) {
            printf("[FatFs] Format OK\r\n");
            /* 格式化后重新挂载 */
            res = f_mount(0, &fatfs);
        } else {
            printf("[FatFs] Format failed: %d\r\n", res);
        }
    }

    if (res == FR_OK) {
        printf("[FatFs] Mount OK\r\n");
    } else {
        printf("[FatFs] Mount failed: %d\r\n", res);
    }
}


void UsrFunction(void)
{

    while(1)
   {

        OLED_Refresh();

    }

}
