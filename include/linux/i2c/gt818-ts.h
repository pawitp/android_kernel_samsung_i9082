/*
 * include/linux/goodix_touch.h
 *
 * Copyright (C) 2011 Goodix, Inc.
 * 
 */
#ifndef 	__GT818_TS_H
#define		__GT818_TS_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/i2c-kona.h>

//*************************TouchScreen Work Part*****************************

#define GOODIX_I2C_NAME "gt818-ts"
#define GT801_PLUS
#define GT801_NUVOTON
#define GUITAR_UPDATE_STATE 0x02
//#define NO_DEFAULT_ID
//define resolution of the touchscreen
//#ifdef CONFIG_BOARD_L400
#define TOUCH_MAX_HEIGHT    800//    480	
#define TOUCH_MAX_WIDTH		  480
//define resolution of the LCD
#define SCREEN_MAX_HEIGHT	  800				
#define SCREEN_MAX_WIDTH	  480
//#endif


#define FLAG_UP		0
#define FLAG_DOWN		1
//set GT801 PLUS trigger mode,只能设置0或1 
#define INT_TRIGGER		0	//0
#define POLL_TIME		10	//actual query spacing interval:POLL_TIME+6

#define GOODIX_MULTI_TOUCH
#ifdef GOODIX_MULTI_TOUCH
	#define MAX_FINGER_NUM	2	
#else
	#define MAX_FINGER_NUM	1	
#endif

//#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)

#define READ_TOUCH_ADDR_H 0x07
#define READ_TOUCH_ADDR_L 0x12
#define READ_KEY_ADDR_H 0x07
#define READ_KEY_ADDR_L 0x21
#define READ_COOR_ADDR_H 0x07
#define READ_COOR_ADDR_L 0x22
#define READ_ID_ADDR_H 0x00
#define READ_ID_ADDR_L 0xff
struct goodix_i2c_rmi_platform_data {
	struct i2c_slave_platform_data i2c_pdata;

	/* Screen area definition */
	int            scr_x_min;
	int            scr_x_max;
	int            scr_y_min;
	int            scr_y_max;
    int            int_port;
	int            tp_rst;
};


struct goodix_ts_data {
	uint16_t addr;
	uint8_t bad_data;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct goodix_i2c_rmi_platform_data *pdata;
	int use_reset;		//use RESET flag
	int use_irq;		//use EINT flag
	int read_mode;		//read moudle mode,20110221 by andrew
	struct hrtimer timer;
	struct work_struct  work;
	char phys[32];
	int retry;
	uint16_t version;
	
	uint16_t vendor;
	struct early_suspend early_suspend;
	int (*power)(struct goodix_ts_data * ts, int on);
};

//*****************************End of Part I *********************************



//*************************Firmware Update part*******************************
#define CONFIG_TOUCHSCREEN_GOODIX_IAP //开启APK升级 功
#define AUTO_UPDATA_GT818  //启动开机自动升级 功

#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
/////////////////////////////// UPDATE STEP 5 START /////////////////////////////////////////////////////////////////
#define TPD_CHIP_VERSION_C_FIRMWARE_BASE 0x5A
#define TPD_CHIP_VERSION_D_FIRMWARE_BASE 0x7A
enum
{
    TPD_GT818_VERSION_B,
    TPD_GT818_VERSION_C,
    TPD_GT818_VERSION_D
};

#define APK_UPDATE_TP               1
#define APK_READ_FUN                 10

/////////////////////////////// UPDATE STEP 5 END /////////////////////////////////////////////////////////////////
#define APK_WRITE_CFG               11

//fun cmd
//#define CMD_DISABLE_TP             0
//#define CMD_ENABLE_TP              1
#define CMD_READ_VER               2
//#define CMD_READ_RAW               3
//#define CMD_READ_DIF               4
#define CMD_READ_CFG               5
//#define CMD_SYS_REBOOT             101

//read mode
#define MODE_RD_VER                1
#define MODE_RD_RAW                2
#define MODE_RD_DIF                3
#define MODE_RD_CFG                4


struct tpd_firmware_info_t
{
    int magic_number_1;
    int magic_number_2;
    unsigned short version;
    unsigned short length;
    unsigned short checksum;
    unsigned char data;
};




#define  NVRAM_LEN               0x0FF0   //    nvram total space
#define  NVRAM_BOOT_SECTOR_LEN   0x0100 // boot sector 
#define  NVRAM_UPDATE_START_ADDR 0x4100

#define  BIT_NVRAM_STROE            0
#define  BIT_NVRAM_RECALL           1
#define BIT_NVRAM_LOCK 2
#define  REG_NVRCS_H 0X12
#define  REG_NVRCS_L 0X01
#define GT818_SET_INT_PIN( level ) gpio_direction_output(INT_PORT, level) //null macro now


static int goodix_update_write(struct file *filp, const char __user *buff, unsigned long len, void *data);
static int goodix_update_read( char *page, char **start, off_t off, int count, int *eof, void *data );

#define PACK_SIZE 					64					//update file package size
#define MAX_TIMEOUT					30000				//update time out conut
#define MAX_I2C_RETRIES				10					//i2c retry times

//I2C buf address
#define ADDR_CMD					80
#define ADDR_STA					81
#define ADDR_DAT					82

//moudle state
#define UPDATE_START				0x02
#define SLAVE_READY					0x08
#define UNKNOWN_ERROR				0x10
#define CHECKSUM_ERROR				0x20
#define TRANSLATE_ERROR				0x40
#define FLASH_ERROR					0X80

//error no
#define ERROR_NO_FILE				2//ENOENT
#define ERROR_FILE_READ				23//ENFILE
#define ERROR_FILE_TYPE				21//EISDIR
#define ERROR_GPIO_REQUEST			4//EINTR
#define ERROR_I2C_TRANSFER			5//EIO
#define ERROR_NO_RESPONSE			16//EBUSY
#define ERROR_TIMEOUT				110//ETIMEDOUT

#endif
//*****************************End of Part III********************************
//struct goodix_i2c_rmi_platform_data {
	//uint32_t version;	/* Use this entry for panels with */
	//reservation
//};


#endif /* _LINUX_GOODIX_TOUCH_H */

