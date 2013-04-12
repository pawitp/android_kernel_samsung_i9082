
/* drivers/input/touchscreen/goodix_touch.c
 *
 * Copyright (C) 2011 Goodix, Inc.
 * 
 * Date: 2011.07.01
 *
 */



//add GT818 Touch panel driver
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/proc_fs.h>
#include <linux/i2c/gt818-ts.h>
#include <linux/i2c/gt818-update.h>


#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <asm/uaccess.h>

static void goodix_ts_work_func(struct work_struct *work);

#define DEBUG
#define HAVE_TOUCH_KEY

//*************************Touchkey Surpport Part*****************************


#ifdef HAVE_TOUCH_KEY
const uint16_t touch_key_array[]={
                                  KEY_MENU,

				  KEY_HOME,
	                          KEY_BACK,
                                  KEY_SEARCH 
									 }; 
	#define MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#endif
//*****************************End of Part II*********************************




#if !defined(GT801_PLUS) && !defined(GT801_NUVOTON)
#error The code does not match this touchscreen.
#endif

static u8  gt818_update_proc( u8 *nvram, u16 length, struct goodix_ts_data *ts );


static struct workqueue_struct *goodix_wq;
static const char *s3c_ts_name = "Goodix Capacitive TouchScreen";
static struct point_queue finger_list;
struct i2c_client * i2c_connect_client = NULL;
//EXPORT_SYMBOL(i2c_connect_client);
static struct proc_dir_entry *goodix_proc_entry;
	
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif
//used by firmware update CRC
unsigned int oldcrc32 = 0xFFFFFFFF;
unsigned int crc32_table[256];
unsigned int ulPolynomial = 0x04c11db7;

/*******************************************************	
功能：	
	读取从机数据
	每个读操作用两条i2c_msg组成，第1条消息用于发送从机地址，
	第2条用于发送读取地址和取回数据；每条消息前发送起始信号
参数：
	client:	i2c设备，包含设备地址
	buf[0]：	首字节为读取地址
	buf[1]~buf[len]：数据缓冲区
	len：	读取数据长度
return：
	执行消息数
*********************************************************/
/*Function as i2c_master_send */
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret=-1;
	//发送写地址
	msgs[0].flags=0;//写消息
	msgs[0].addr=client->addr;
	msgs[0].len=2;
	msgs[0].buf=&buf[0];
	//接收数据
	msgs[1].flags=I2C_M_RD;//读消息
	msgs[1].addr=client->addr;
	msgs[1].len=len-2;
	msgs[1].buf=&buf[2];
	
	ret=i2c_transfer(client->adapter,msgs, 2);
	return ret;
}

/*******************************************************	
功能：
	向从机写数据
参数：
	client:	i2c设备，包含设备地址
	buf[0]：	首字节为写地址
	buf[1]~buf[len]：数据缓冲区
	len：	数据长度	
return：
	执行消息数
*******************************************************/
/*Function as i2c_master_send */
static int i2c_write_bytes(struct i2c_client *client,uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;
	//发送设备地址
	msg.flags=0;//写消息
	msg.addr=client->addr;
	msg.len=len;
	msg.buf=data;		
	
	ret=i2c_transfer(client->adapter,&msg, 1);
	return ret;
}

/*******************************************************
功能：
	发送前缀命令
	
	ts:	client私有数据结构体
return：

	执行结果码，0表示正常执行
*******************************************************/
static int i2c_pre_cmd(struct goodix_ts_data *ts)
{
	int ret;
	uint8_t pre_cmd_data[2]={0};	
	pre_cmd_data[0]=0x0f;
	pre_cmd_data[1]=0xff;
	ret=i2c_write_bytes(ts->client,pre_cmd_data,2);
	udelay(20);
	return ret;
}

/*******************************************************
功能：
	发送后缀命令
	
	ts:	client私有数据结构体
return：

	执行结果码，0表示正常执行
*******************************************************/
static int i2c_end_cmd(struct goodix_ts_data *ts)
{
	int ret;
	uint8_t end_cmd_data[2]={0};	
	end_cmd_data[0]=0x80;
	end_cmd_data[1]=0x00;
	ret=i2c_write_bytes(ts->client,end_cmd_data,2);
	udelay(20);//msleep(2);
	return ret;
}

static short get_chip_version( unsigned int sw_ver )
{
    if ( (sw_ver&0xff) < TPD_CHIP_VERSION_C_FIRMWARE_BASE )
        return TPD_GT818_VERSION_B;
    else if ( (sw_ver&0xff) < TPD_CHIP_VERSION_D_FIRMWARE_BASE )
        return TPD_GT818_VERSION_C;
    else
        return TPD_GT818_VERSION_D;
}
/*******************************************************
功能：
	获取版本信息
参数：
	ts:	client私有数据结构体
return：
	执行结果码，0表示正常执行
*******************************************************/

static int  goodix_read_vendor(struct goodix_ts_data *ts)
{
	int ret;
	uint8_t vendor_data[5]={0};	//store touchscreen version infomation
	memset(vendor_data, 0, 5);
	vendor_data[0]=0x07;
	vendor_data[1]=0x10;	
	msleep(2);
	ret=i2c_read_bytes(ts->client, vendor_data, 4);
	if (ret < 0) 
		return ret;
	dev_info(&ts->client->dev," Guitar Vendor: %d.%d\n",vendor_data[3],vendor_data[2]);
	
	ts->vendor=(((vendor_data[3]<<8)|vendor_data[2])&0x0007);
	return 0;
}
static int  goodix_read_version(struct goodix_ts_data *ts)
{
	int ret;
	uint8_t version_data[5]={0};	//store touchscreen version infomation
	memset(version_data, 0, 5);
	version_data[0]=0x07;
	version_data[1]=0x17;	
	msleep(2);
	ret=i2c_read_bytes(ts->client, version_data, 4);
	if (ret < 0) 
		return ret;
	dev_info(&ts->client->dev," Guitar Version: %d.%d\n",version_data[3],version_data[2]);
	
	ts->version=((version_data[3]<<8)|version_data[2]);
	return 0;


	
}


/*******************************************************
功能：
	Guitar初始化函数，用于发送配置信息，获取版本信息
参数：
	ts:	client私有数据结构体
return：
	执行结果码，0表示正常执行
*******************************************************/
static int goodix_init_panel(struct goodix_ts_data *ts)
{
	int ret=-1; 					

	uint8_t config_info_d_lc[] = {		//lianchuang_Touch key devlop board
         //lianchuanconfig 20111104 
        0x06,0xA2,
       //when tp has water,the tp will  no use
		0x12,0x10,0x0E,0x0C,0x0A,0x08,0x06,0x04,
		0x02,0x00,0x81,0x11,0x91,0x11,0xA1,0x11,
		0xB1,0x11,0xC1,0x11,0xD1,0x11,0xE1,0x11,
		0xF1,0x11,0x71,0x11,0x61,0x11,0x51,0x11,
		0x41,0x11,0x31,0x11,0x21,0x11,0x11,0x11,
		0x01,0x11,0x0B,0x13,0x10,0x10,0x10,0x26,
	    0x26,0x26,0x10,0x0F,0x0A,0x48,0x2A,0x4D,
		0x03,0x00,MAX_FINGER_NUM,(TOUCH_MAX_HEIGHT&0xff),(TOUCH_MAX_HEIGHT>>8),(TOUCH_MAX_WIDTH&0xff),(TOUCH_MAX_WIDTH>>8),0x00,
		0x00,0x5A,0x5D,0x5E,0x61,0x00,0x00,0x05,
	    0x20,0x05,0x08,0x00,0x00,0x00,0x00,0x00,
		0x14,0x10,0xEF,0x03,0x00,0x00,0x00,0x00,
	    0x00,0x00,0x1F,0x45,0x6C,0x90,0x0D,0x40,
	    0x30,0x25,0x20,0x00,0x00,0x00,0x00,0x00,
		0x00,0x01
          } ;
	
        uint8_t config_info_d_bm[] = {		//baoming_Touch key devlop board 
	
	0x06,0xA2,
        0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,
	0x10,0x12,0x71,0x11,0x61,0x11,0x51,0x11,
	0x41,0x11,0x31,0x11,0x21,0x11,0x11,0x11,
	0x01,0x11,0xF1,0x11,0xE1,0x11,0xD1,0x11,
	0xC1,0x11,0x81,0x11,0x91,0x11,0xA1,0x11,
	0xB1,0x11,0x07,0x13,0x10,0x10,0x10,0x20,
	0x20,0x20,0x10,0x0F,0x0A,0x40,0x30,0x4D,
//	0x03,0x00,0x05,0x40,0x01,0xE0,0x01,0x00,

        0x03,0x00,MAX_FINGER_NUM,(TOUCH_MAX_HEIGHT&0xff),(TOUCH_MAX_HEIGHT>>8),(TOUCH_MAX_WIDTH&0xff),(TOUCH_MAX_WIDTH>>8),0x00,

	0x00,0x5A,0x5D,0x5E,0x61,0x00,0x00,0x05,

	0x14,0x00,0x08,0x00,0x00,0x00,0x00,0x00,
	0x14,0x10,0xEF,0x03,0x00,0x00,0x00,0x00,
	0x00,0x00,0x1F,0x49,0x6E,0x90,0x0D,0x40,
	0x30,0x25,0x20,0x00,0x00,0x00,0x00,0x00,
	0x00,0x01
	   };

        uint8_t config_info_d_jia[] = {             //Touch key devlop board

        0x06,0xA2,
        0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,
	    0x12,0x00,0x01,0x11,0x11,0x11,0x21,0x11,
        0x31,0x11,0x41,0x11,0x51,0x11,0x61,0x11,
        0x71,0x11,0x81,0x11,0x91,0x11,0xA1,0x11,
        0xB1,0x11,0xC1,0x11,0xD1,0x11,0xE1,0x11,
        0xF1,0x11,0x07,0x03,0x10,0x10,0x10,0x2A,
        0x2A,0x2A,0x0D,0x0C,0X09,0X45,0X3A,0x4F,
//        0x03,0x00,0x05,0xF0,0x00,0x63,0x01,0x00,

        0x03,0x30,MAX_FINGER_NUM,(TOUCH_MAX_HEIGHT&0xff),(TOUCH_MAX_HEIGHT>>8),(TOUCH_MAX_WIDTH&0xff),(TOUCH_MAX_WIDTH>>8),0x00,
     //   0x03,0x00,0x05,0xF0,0x00,0x63,0x01,0x00,
        0x00,0x5E,0x5F,0x62,0x63,0x00,0x00,0x03,
        0x19,0x45,0x05,0x00,0x00,0x00,0x00,0x00,//  0x19,0x10,0x04,0x00,0x00,0x00,0x00,0x00,

        0x20,0x10,0x1C,0x04,0x00,0x00,0x00,0x00,
        0x00,0x00,0x1B,0x4F,0x84,0x00,0x0A,0x40,
        0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x01
             
            }; 
 uint8_t config_info_c[] = {

 //2.8inch //GT818_D 2011.08.31	
	0x06,0xA2,
	0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,
	0x12,0x00,0x62,0x22,0x72,0x22,0x82,0x22,
	0x92,0x22,0xA2,0x22,0xB2,0x22,0xC2,0x22,
	0x02,0x22,0x12,0x22,0x22,0x22,0x32,0x22,
	0x42,0x22,0x52,0x22,0xD2,0x22,0xE2,0x22,
	0x02,0x22,0x37,0x03,0x50,0x50,0x50,0x2A,
	0x2A,0x2A,0x0D,0x0C,0x09,0x3A,0x2A,0x0D,//0x05,//0x09,
	0x05,0x3C,MAX_FINGER_NUM,(TOUCH_MAX_HEIGHT&0xff),(TOUCH_MAX_HEIGHT>>8),(TOUCH_MAX_WIDTH&0xff),(TOUCH_MAX_WIDTH>>8),0x00,
	0x00,0x5E,0x65,0x62,0x69,0x00,0x00,0x06,
	0x19,0x45,0x00,0x00,0x00,0x00,0x00,0x00,
	0x14,0x10,0x1C,0x04,0x00,0x00,0x00,0x00,
	0x00,0x00,0x20,0x50,0x80,0x00,0x06,0x35,
	0x25,0x25,0x20,0x00,0x00,0x00,0x00,0x00,
	0x00,0x01

	};

/* yaogangxiang@wind-mobi.com 20120207 begin */
//add for gt818,800 * 480
    uint8_t config_info_new[] = {
	0x06,0xA2,    
	0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,
	0x10,0x12,0x00,0x00,0x10,0x00,0x20,0x00,
	0x30,0x00,0x40,0x00,0x50,0x00,0x60,0x00,
	0x70,0x00,0x80,0x00,0x90,0x00,0xA0,0x00,
	0xB0,0x00,0xC0,0x00,0xD0,0x00,0xE0,0x00,
	0xF0,0x00,0x07,0x03,0x50,0x50,0x50,0x30,
	0x30,0x30,0x10,0x0F,0x0A,0x40,0x30,0x09,
	0x03,0x00,MAX_FINGER_NUM,(TOUCH_MAX_WIDTH&0xff),(TOUCH_MAX_WIDTH>>8),(TOUCH_MAX_HEIGHT&0xff),(TOUCH_MAX_HEIGHT>>8),0x00,
	0x00,0x55,0x4F,0x58,0x52,0x00,0x00,0x14,
	0x14,0x25,0x05,0x00,0x00,0x00,0x00,0x00,
	0x14,0x10,0x74,0x03,0x00,0x00,0x00,0x00,
	0x00,0x00,0x1A,0x43,0x6E,0x95,0x0C,0x3E,
	0x2E,0x25,0x20,0x00,0x00,0x00,0x00,0x00,
	0x00,0x01
	};
/* yaogangxiang@wind-mobi.com 20120207 end */	
    ret= goodix_read_version(ts);
	if(ret < 0)
		return ret;

   if ((ts->version & 0xff)< TPD_CHIP_VERSION_D_FIRMWARE_BASE)
   {
       dev_info(&ts->client->dev,"Gutitar Version:C\n");
	   ret=i2c_write_bytes(ts->client,config_info_c, (sizeof(config_info_c)/sizeof(config_info_c[0])));
   }
   else
   {

		dev_info(&ts->client->dev,"Gutitar Version:D\n");
	        
                ret= goodix_read_vendor(ts);
                if((ts->vendor) == 1 ){
                     ret=i2c_write_bytes(ts->client,config_info_d_jia, (sizeof(config_info_d_jia)/sizeof(config_info_d_jia[0])));		         dev_info(&ts->client->dev,"Gutitar vendor=2:D\n");
              } else if ((ts->vendor) == 2){
//#ifdef CONFIG_BOARD_L400
                     ret=i2c_write_bytes(ts->client,config_info_new, (sizeof(config_info_d_lc)/sizeof(config_info_d_lc[0])));
//#else
//                     ret=i2c_write_bytes(ts->client,config_info_d_lc, (sizeof(config_info_d_lc)/sizeof(config_info_d_lc[0])));
//#endif
              } else {
//                      ret=i2c_write_bytes(ts->client,config_info_d_bm, (sizeof(config_info_d_bm)/sizeof(config_info_d_bm[0])));
		        ret=i2c_write_bytes(ts->client,config_info_d_bm, (sizeof(config_info_d_bm)/sizeof(config_info_d_bm[0])));
              }
   }
	//ret=i2c_write_bytes(ts->client,config_info, (sizeof(config_info)/sizeof(config_info[0])));
	if (ret < 0) 
		return ret;
	msleep(10);
	return 0;

}


/********************************************************************************	
功能：
	触摸屏工作函数
	由中断触发，接受1组坐标数据，校验后再分析输出
参数：
	ts:	client私有数据结构体
return：
	执行结果码，0表示正常执行
*********************************************************************************/
static void goodix_ts_work_func_old(struct work_struct *work)
{	
	uint8_t  touch_data[3] = {READ_TOUCH_ADDR_H,READ_TOUCH_ADDR_L,0};
	uint8_t  key_data[3] ={READ_KEY_ADDR_H,READ_KEY_ADDR_L,0};
	uint8_t  point_data[8*MAX_FINGER_NUM+2]={ 0 };  
	static uint8_t   finger_last[MAX_FINGER_NUM+1]={0};		//上次触摸按键的手指索引
	uint8_t  finger_current[MAX_FINGER_NUM+1] = {0};		//当前触摸按键的手指索引
	uint8_t  coor_data[6*MAX_FINGER_NUM] = {0};				//对应手指的数据
	static uint8_t  last_key = 0;
	uint8_t  finger = 0;
	uint8_t  key = 0;
        u8  track_id = 0;
	unsigned int  count = 0;
	unsigned int position = 0;	
	int ret=-1;
	int tmp = 0;
	int temp = 0;
	uint16_t *coor_point;
	
	struct goodix_ts_data *ts = container_of(work, struct goodix_ts_data, work);
	i2c_pre_cmd(ts);
COORDINATE_POLL:
	if( tmp > 9) {
		dev_info(&(ts->client->dev), "Because of transfer error,touchscreen stop working.\n");
		goto XFER_ERROR ;
	}

	ret=i2c_read_bytes(ts->client, touch_data,sizeof(touch_data)/sizeof(touch_data[0]));  //读0x712，触摸
	
	   if((touch_data[2]&0x0f) == 0x0f)
       {
           goodix_init_panel(ts);
            goto DATA_NO_READY;
        }
	
	if(ret <= 0) {
		dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
		ts->bad_data = 1;
		tmp ++;
		ts->retry++;
	if (!ts->pdata->int_port)
		goto COORDINATE_POLL;
	else
		goto XFER_ERROR;
	
	}
	
#ifdef HAVE_TOUCH_KEY	
	ret=i2c_read_bytes(ts->client, key_data,sizeof(key_data)/sizeof(key_data[0]));  //读0x721，按键 
	if(ret <= 0) {
		dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
		ts->bad_data = 1;
		tmp ++;
		ts->retry++;
	if (!ts->pdata->int_port)
		goto COORDINATE_POLL;
	else
		goto XFER_ERROR;
	
	}
	key = key_data[2]&0x0f;
#endif

	if(ts->bad_data)
		//TODO:Is sending config once again (to reset the chip) useful?	
		msleep(20);
	
    
	if((touch_data[2]&0x30)!=0x20)
	{
		goto DATA_NO_READY;		
	}	
      // if((touch_data[2]&0x0f) == 0x0f)
      // {
        //    goodix_init_panel(ts);
          //  goto DATA_NO_READY;
       // }
	ts->bad_data = 0;
	
	finger = touch_data[2]&0x0f;
	if(finger != 0)
	{
		point_data[0] = READ_COOR_ADDR_H;		//read coor high address
		point_data[1] = READ_COOR_ADDR_L;		//read coor low address
		ret=i2c_read_bytes(ts->client, point_data, finger*8+2);
		if(ret <= 0)	
		{
			dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
			ts->bad_data = 1;
			tmp ++;
			ts->retry++;
		if (!ts->pdata->int_port)
			goto COORDINATE_POLL;
		else
			goto XFER_ERROR;
		
		}		
		for(position=2; position<((finger-1)*8+2+1); position += 8)
		{
			temp = point_data[position];
			if(temp<(MAX_FINGER_NUM+1))
			{
				finger_current[temp] = 1;
				for(count=0; count<6; count++)
				{
					coor_data[(temp-1)*6+count] = point_data[position+1+count];		//记录当前手指索引，并装载坐标数据
				}
			}
			else
			{
			dev_err(&(ts->client->dev),"Track Id error:%d\n ", ret);
			ts->bad_data = 1;
			tmp ++;
			ts->retry++;
			if (!ts->pdata->int_port)
				goto COORDINATE_POLL;
			else
				goto XFER_ERROR;
			
			}		
		}
		//coor_point = (uint16_t *)coor_data;		
	
	}
	
	else
	{
		for(position=1;position < MAX_FINGER_NUM+1; position++)		
		{
			finger_current[position] = 0;
		}
	}
	coor_point = (uint16_t *)coor_data;	
	for(position=1;position < MAX_FINGER_NUM+1; position++)
	{
		if((finger_current[position] == 0)&&(finger_last[position] != 0))
			{
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, 0);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, 0);
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	//			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);	
				input_mt_sync(ts->input_dev);
			}
		else if(finger_current[position])
			{ 	

               input_report_abs(ts->input_dev, ABS_MT_POSITION_X, (*(coor_point+3*(position-1))));//2.8inch
			   input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,(*(coor_point+3*(position-1)+1))*SCREEN_MAX_WIDTH/(SCREEN_MAX_WIDTH));


			   input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,1);
                           input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, position);
				input_mt_sync(ts->input_dev);
	
			}
			
	}
	input_sync(ts->input_dev);	
	
	for(position=1;position<MAX_FINGER_NUM+1; position++)
	{
		finger_last[position] = finger_current[position];
	}
#ifdef HAVE_TOUCH_KEY
	if((last_key == 0)&&(key == 0))
		;
	else
	{
         for(count = 0; count < MAX_KEY_NUM; count++)
		//for(count = 0; count < 4; count++)
		{
			input_report_key(ts->input_dev, touch_key_array[count], !!(key&(0x01<<count)));	
		}
	}		
	last_key = key;	
#endif

DATA_NO_READY:
XFER_ERROR:
	i2c_end_cmd(ts);
//	if(ts->use_irq)
		//enable_irq(gpio_to_irq(ts->pdata->int_port));

}

/******************************t*************************	
功能：
	计时器响应函数
	由计时器触发，调度触摸屏工作函数运行；之后重新计时
参数：
	timer：函数关联的计时器	
return：
	计时器工作模式，HRTIMER_NORESTART表示不需要自动重启
********************************************************/
static enum hrtimer_restart goodix_ts_timer_func(struct hrtimer *timer)
{
	struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);
	queue_work(goodix_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, (POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

/*******************************************************	
功能：
	中断响应函数
	由中断触发，调度触摸屏处理函数运行
参数：
	timer：函数关联的计时器	
return：
	计时器工作模式，HRTIMER_NORESTART表示不需要自动重启
********************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
	struct goodix_ts_data *ts = dev_id;

	//printk(KERN_INFO"-------------------ts_irq_handler------------------\n");
	//disable_irq_nosync(ts->client->irq);
	queue_work(goodix_wq, &ts->work);
	
	return IRQ_HANDLED;
}

/*******************************************************	
功能：
	管理GT801的电源，允许GT801 PLUS进入睡眠或将其唤醒
参数：
	on:	0表示使能睡眠，1为唤醒
return：
	是否设置成功，0为成功
	错误码：-1为i2c错误，-2为GPIO错误；-EINVAL为参数on错误
********************************************************/
//#if defined(ts->pdata->int_port)
static int goodix_ts_power(struct goodix_ts_data * ts, int on)
{
	int ret = -1;

	unsigned char i2c_control_buf[3] = {0x06,0x92,0x01};		//suspend cmd

	
	if (ts->pdata->int_port)
	{
	    if(ts != NULL && !ts->use_irq)
	    {
			return -2;
	    }
	}		
	switch(on)
	{
		case 0:
			printk(KERN_DEBUG"rina >>>>GT818 sleep!!!! \n");
			i2c_pre_cmd(ts);
			ret = i2c_write_bytes(ts->client, i2c_control_buf, 3);
			printk(KERN_INFO"Send suspend cmd\n");
			if(ret > 0)						//failed
				ret = 0;
    		i2c_end_cmd(ts);
			return ret;
			
		case 1:
			printk("rina >>>>GT818 wakeup !!!!\n");
			if (ts->pdata->int_port)
			{
	                printk("rina >>>>Send resume cmd\n");
				   
					gpio_direction_output(ts->pdata->int_port, 0);
					msleep(20);
					gpio_free(ts->pdata->int_port);
					if(ts->use_irq) 
					{
					 printk("rina >>>>GT818 sleep use_irq!!!! \n");
					 
					 gpio_request(ts->pdata->int_port, "gt818-ts");	//Request IO
					 gpio_direction_input(ts->pdata->int_port);
					 gpio_to_irq(ts->pdata->int_port);
					 
					 }	//s3c_gpio_cfgpin(ts->pdata->int_port, INT_CFG);	//Set IO port as interrupt port	
				else 
					gpio_direction_input(ts->pdata->int_port);
			}
			else if(ts->pdata->tp_rst)
			{
				gpio_direction_output(ts->pdata->tp_rst,0);
				msleep(1);
				gpio_direction_input(ts->pdata->tp_rst);		
			}					
			msleep(40);
			ret = 0;
			return ret;
				
		default:
			printk(KERN_DEBUG "%s: Cant't support this command.", s3c_ts_name);
			return -EINVAL;
	}

}

//check  if the chip is  GT818 
static int verify_gt818( struct goodix_ts_data *ts)
{
	//uint8_t version_data[5]={0};	//store touchscreen version infomation

	uint8_t data[4] = {0,0,0,0};
   
	
	/*check if i2c with the dedicated adress could return value*/
	if(i2c_read_bytes(ts->client,data,3)<0)
	{
		printk("Could not get GT818 i2c register content\r\n");
		return -1;
	}

	/*check if the register conent is valid*/
	if(goodix_read_version(ts)<0)
	{
		printk("This is not a valid GT818 device, data[0] is 0X%02X\r\n", goodix_read_version(ts));
		return -1;
	}

	return 0;
}
static  int gt818_reset( struct goodix_ts_data *ts )
{
    int ret = 1;
    u8 retry;

    unsigned char outbuf[3] = {0,0xff,0};
    unsigned char inbuf[3] = {0,0xff,0};
    //outbuf[1] = 1;

    gpio_direction_output(ts->pdata->tp_rst,0);
    msleep(20);
    gpio_direction_input(ts->pdata->tp_rst);
    msleep(100);
    for(retry=0;retry < 80; retry++)
    {
        ret =i2c_write_bytes(ts->client, inbuf, 0);     //Test I2C connection.
        if (ret > 0)
        {
            msleep(10);
            ret =i2c_read_bytes(ts->client, inbuf, 3);  //Test I2C connection.
            if (ret > 0)
            {
                if(inbuf[2] == 0x55)
                    {
                            ret =i2c_write_bytes(ts->client, outbuf, 3);
                            msleep(10);
                            break;
                        }
                                }
                        }

                }
    printk(KERN_INFO"Detect address %0X\n", ts->client->addr);
    //msleep(500);
    return ret;
}
//********************************************************************************************
static u8  is_equal( u8 *src , u8 *dst , int len )
{
    int i;
    
    for( i = 0 ; i < len ; i++ )
    {
        //printk(KERN_INFO"[%02X:%02X]\n", src[i], dst[i]);
    }

    for( i = 0 ; i < len ; i++ )
    {
        if ( src[i] != dst[i] )
        {
            return 0;
        }
    }
    
    return 1;
}


static  u8 gt818_nvram_store( struct goodix_ts_data *ts )
{
    int ret;
    int i;
    u8 inbuf[3] = {REG_NVRCS_H,REG_NVRCS_L,0};
    //u8 outbuf[3] = {};
    ret = i2c_read_bytes( ts->client, inbuf, 3 );

    if ( ret < 0 )
    {
        return 0;
    }

    if ( ( inbuf[2] & BIT_NVRAM_LOCK ) == BIT_NVRAM_LOCK )
    {
        return 0;
    }

    inbuf[2] = (1<<BIT_NVRAM_STROE);            //store command

    for ( i = 0 ; i < 300 ; i++ )
    {
        ret = i2c_write_bytes( ts->client, inbuf, 3 );

        if ( ret < 0 )
            break;
    }

    return ret;
}
static u8  gt818_nvram_recall( struct goodix_ts_data *ts )
{
    int ret;
    u8 inbuf[3] = {REG_NVRCS_H,REG_NVRCS_L,0};

    ret = i2c_read_bytes( ts->client, inbuf, 3 );

    if ( ret < 0 )
    {
        return 0;
    }

    if ( ( inbuf[2]&BIT_NVRAM_LOCK) == BIT_NVRAM_LOCK )
    {
        return 0;
    }

    inbuf[2] = ( 1 << BIT_NVRAM_RECALL );               //recall command
    ret = i2c_write_bytes( ts->client , inbuf, 3);
    return ret;
}


static  int gt818_set_address_2( struct goodix_ts_data *ts )
{
    unsigned char inbuf[3] = {0,0,0};
    int i;

    for ( i = 0 ; i < 12 ; i++ )
    {
        if ( i2c_read_bytes( ts->client, inbuf, 3) )
        {
            printk(KERN_INFO"Got response\n");
            return 1;
        }
        printk(KERN_INFO"wait for retry\n");
        msleep(50);
    }
    return 0;
}

static u8  gt818_update_firmware( u8 *nvram, u16 length, struct goodix_ts_data *ts)
{
    u8 ret,err,retry_time,i;
    u16 cur_code_addr;
    u16 cur_frame_num, total_frame_num, cur_frame_len;
    u32 gt80x_update_rate;

    unsigned char i2c_data_buf[PACK_SIZE+2] = {0,};       
	
    unsigned char i2c_chk_data_buf[PACK_SIZE+2] = {0,}; 
	
    if( length > NVRAM_LEN - NVRAM_BOOT_SECTOR_LEN )
    {
        printk(KERN_INFO"length too big %d %d\n", length, NVRAM_LEN - NVRAM_BOOT_SECTOR_LEN );
        return 0;
    }

    total_frame_num = ( length + PACK_SIZE - 1) / PACK_SIZE;

    //gt80x_update_sta = _UPDATING;
    gt80x_update_rate = 0;

    for( cur_frame_num = 0 ; cur_frame_num < total_frame_num ; cur_frame_num++ )
    {
        retry_time = 5;

        cur_code_addr = NVRAM_UPDATE_START_ADDR + cur_frame_num * PACK_SIZE;
        i2c_data_buf[0] = (cur_code_addr>>8)&0xff;
        i2c_data_buf[1] = cur_code_addr&0xff;

        i2c_chk_data_buf[0] = i2c_data_buf[0];
        i2c_chk_data_buf[1] = i2c_data_buf[1];

        if( cur_frame_num == total_frame_num - 1 )
        {
            cur_frame_len = length - cur_frame_num * PACK_SIZE;
        }
        else
        {
            cur_frame_len = PACK_SIZE;
        }

        //strncpy(&i2c_data_buf[2], &nvram[cur_frame_num*PACK_SIZE], cur_frame_len);
        for(i=0;i<cur_frame_len;i++)
        {
            i2c_data_buf[2+i] = nvram[cur_frame_num*PACK_SIZE+i];
            }
        do
        {
            err = 0;

            ret = i2c_write_bytes(ts->client, i2c_data_buf, (cur_frame_len+2));

            if ( ret <= 0 )
            {
                printk(KERN_INFO"write fail\n");
                err = 1;
            }

            ret = i2c_read_bytes(ts->client, i2c_chk_data_buf, (cur_frame_len+2));
           // ret = gt818_i2c_read( guitar_i2c_address, cur_code_addr, inbuf, cur_frame_len);

            if ( ret <= 0 )
            {
                printk(KERN_INFO"read fail\n");
                err = 1;
            }

            if( is_equal( &i2c_data_buf[2], &i2c_chk_data_buf[2], cur_frame_len ) == 0 )
            {
                printk(KERN_INFO"not equal\n");
                err = 1;
            }

        } while ( err == 1 && (--retry_time) > 0 );

        if( err == 1 )
        {
            break;
        }

        gt80x_update_rate = ( cur_frame_num + 1 )*128/total_frame_num;

    }

    if( err == 1 )
    {
        printk(KERN_INFO"write nvram fail\n");
        return 0;
    }

    ret = gt818_nvram_store(ts);

    msleep( 20 );

    if( ret == 0 )
    {
        printk(KERN_INFO"nvram store fail\n");
        return 0;
    }

    ret = gt818_nvram_recall(ts);

    msleep( 20 );

    if( ret == 0 )
    {
        printk(KERN_INFO"nvram recall fail\n");
        return 0;
    }

    for ( cur_frame_num = 0 ; cur_frame_num < total_frame_num ; cur_frame_num++ )                //     read out all the code
    {

        cur_code_addr = NVRAM_UPDATE_START_ADDR + cur_frame_num*PACK_SIZE;
        retry_time=5;
        i2c_chk_data_buf[0] = (cur_code_addr>>8)&0xff;
        i2c_chk_data_buf[1] = cur_code_addr&0xff;


        if ( cur_frame_num == total_frame_num-1 )
        {
            cur_frame_len = length - cur_frame_num*PACK_SIZE;
        }
        else
        {
            cur_frame_len = PACK_SIZE;
        }

        do
        {
            err = 0;
            //ret = gt818_i2c_read( guitar_i2c_address, cur_code_addr, inbuf, cur_frame_len);
            ret = i2c_read_bytes(ts->client, i2c_chk_data_buf, (cur_frame_len+2));

            if ( ret == 0 )
            {
                err = 1;
            }

            if( is_equal( &nvram[cur_frame_num*PACK_SIZE], &i2c_chk_data_buf[2], cur_frame_len ) == 0 )
            {
                err = 1;
            }
        } while ( err == 1 && (--retry_time) > 0 );

        if( err == 1 )
        {
            break;
        }

        gt80x_update_rate = 127 + ( cur_frame_num + 1 )*128/total_frame_num;
    }

    gt80x_update_rate = 255;
    //gt80x_update_sta = _UPDATECHKCODE;

    if( err == 1 )
    {
        printk(KERN_INFO"nvram validate fail\n");
        return 0;
    } 
    //
    i2c_chk_data_buf[0] = 0xff;
    i2c_chk_data_buf[1] = 0x00;
    i2c_chk_data_buf[2] = 0x0;
    ret = i2c_write_bytes(ts->client, i2c_chk_data_buf, 3);

    if( ret <= 0 )
    {
        printk(KERN_INFO"nvram validate fail\n");
        return 0;
    }

    return 1;
}

static struct file * update_file_open(char * path, mm_segment_t * old_fs_p)
{
	struct file * filp = NULL;
	int errno = -1;
		
	filp = filp_open(path, O_RDONLY, 0644);
	
	if(!filp || IS_ERR(filp))
	{
		if(!filp)
			errno = -ENOENT;
		else 
			errno = PTR_ERR(filp);					
		printk(KERN_ERR "The update file for Guitar open error.\n");
		return NULL;
	}
	*old_fs_p = get_fs();
	set_fs(get_ds());

	filp->f_op->llseek(filp,0,0);
	return filp ;
}
static void update_file_close(struct file * filp, mm_segment_t old_fs)
{
	set_fs(old_fs);
	if(filp)
		filp_close(filp, NULL);
}
static int update_get_flen(char * path)
{
	struct file * file_ck = NULL;
	mm_segment_t old_fs;
	int length ;
	
	file_ck = update_file_open(path, &old_fs);
	if(file_ck == NULL)
		return 0;

	length = file_ck->f_op->llseek(file_ck, 0, SEEK_END);
	//printk("File length: %d\n", length);
	if(length < 0)
		length = 0;
	update_file_close(file_ck, old_fs);
	return length;	
}
#if 0
static int update_file_check(char * path)
{
	unsigned char buffer[64] = { 0 } ;
	struct file * file_ck = NULL;
	mm_segment_t old_fs;
	int count, ret, length ;
	
	file_ck = update_file_open(path, &old_fs);
	
	if(path != NULL)
		printk("File Path:%s\n", path);
	
	if(file_ck == NULL)
		return -ERROR_NO_FILE;

	length = file_ck->f_op->llseek(file_ck, 0, SEEK_END);
#ifdef GUITAR_MESSAGE
	printk(KERN_INFO "gt801 update: File length: %d\n",length);
#endif	
	if(length <= 0 || (length%4) != 0)
	{
		update_file_close(file_ck, old_fs);
		return -ERROR_FILE_TYPE;
	}
	
	//set file point to the begining of the file
	file_ck->f_op->llseek(file_ck, 0, SEEK_SET);	
	oldcrc32 = 0xFFFFFFFF;
	//init_crc32_table();
	while(length > 0)
	{
		ret = file_ck->f_op->read(file_ck, buffer, sizeof(buffer), &file_ck->f_pos);
		if(ret > 0)
		{
			for(count = 0; count < ret;  count++) 	
				GenerateCRC32(&buffer[count],1);			
		}
		else 
		{
			update_file_close(file_ck, old_fs);
			return -ERROR_FILE_READ;
		}
		length -= ret;
	}
	oldcrc32 = ~oldcrc32;
#ifdef GUITAR_MESSAGE	
	printk("CRC_Check: %u\n", oldcrc32);
#endif	
	update_file_close(file_ck, old_fs);
	return 1;	
}

#endif
int  gt818_downloader( struct goodix_ts_data *ts,  unsigned char * data, unsigned char * path)
{
    struct tpd_firmware_info_t *fw_info = (struct tpd_firmware_info_t *)data;
    int i;
    unsigned short checksum = 0;
    unsigned char *data_ptr = &(fw_info->data);
    int retry = 0,ret;
    int err = 0;

    struct file * file_data = NULL;
    mm_segment_t old_fs;
    //unsigned int rd_len;
    unsigned int file_len = 0;
    //unsigned char i2c_data_buf[PACK_SIZE] = {0,};

    const int MAGIC_NUMBER_1 = 0x4D454449;
    const int MAGIC_NUMBER_2 = 0x4154454B;

    if(path[0] == 0)
    {
        printk(KERN_INFO"%s\n", __func__ );
        printk(KERN_INFO"magic number 0x%08X 0x%08X\n", fw_info->magic_number_1, fw_info->magic_number_2 );
        printk(KERN_INFO"magic number 0x%08X 0x%08X\n", MAGIC_NUMBER_1, MAGIC_NUMBER_2 );
        printk(KERN_INFO"current version 0x%04X, target verion 0x%04X\n", ts->version, fw_info->version );
        printk(KERN_INFO"size %d\n", fw_info->length );
        printk(KERN_INFO"checksum %d\n", fw_info->checksum );

        if ( fw_info->magic_number_1 != MAGIC_NUMBER_1 && fw_info->magic_number_2 != MAGIC_NUMBER_2 )
        {
            printk(KERN_INFO"Magic number not match\n");
            err = 0;
            goto exit_downloader;
        }
        if(((ts->version&0xff)> 0x99)||((ts->version&0xff) < 0x4a))
        {
            goto update_start;
        }
        if ( ts->version >= fw_info->version )
        {
            printk(KERN_INFO"No need to upgrade\n");
            err = 0;
            goto exit_downloader;
        }

        if ( get_chip_version( ts->version ) != get_chip_version( fw_info->version ) )
            {
                printk(KERN_INFO"Chip version incorrect");
                err = 0;
                goto exit_downloader;
            }
update_start:
        for ( i = 0 ; i < fw_info->length ; i++ )
            checksum += data_ptr[i];

        checksum = checksum%0xFFFF;

        if ( checksum != fw_info->checksum )
        {
            printk(KERN_INFO"Checksum not match 0x%04X\n", checksum);
            err = 0;
            goto exit_downloader;
        }
    }
    else
    {
        printk(KERN_INFO"Write cmd arg is:\n");
        file_data = update_file_open(path, &old_fs);
        printk(KERN_INFO"Write cmd arg is\n");
        if(file_data == NULL)   //file_data has been opened at the last time
        {
            err = -1;
            goto exit_downloader;
        }


    file_len = (update_get_flen(path))-2;

        printk(KERN_INFO"current length:%d\n", file_len);

            ret = file_data->f_op->read(file_data, &data_ptr[0], file_len, &file_data->f_pos);

            if(ret <= 0)
            {
               err = -1;
                goto exit_downloader;
            }



            update_file_close(file_data, old_fs);


        }
    printk(KERN_INFO"STEP_0:\n");
    //adapter = client->adapter;
    gpio_free(ts->pdata->int_port);
    ret = gpio_request(ts->pdata->int_port, "TS_INT");     //Request IO
    if (ret < 0)
    {
        printk(KERN_INFO"Failed to request GPIO:%d, ERRNO:%d\n",ts->pdata->int_port,ret);
        err = -1;
        goto exit_downloader;
    }

    printk(KERN_INFO"STEP_1:\n");
    err = -1;
    while (  retry < 3 )
        {
            ret = gt818_update_proc( data_ptr, fw_info->length, ts);
            if(ret == 1)
            {
                err = 1;
                break;
            }
            retry++;
    }

exit_downloader:
    //mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
   // mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
       // gpio_direction_output(INT_PORT,1);
       // msleep(1);
        gpio_free(ts->pdata->int_port);

    return err;

}


static u8  gt818_update_proc( u8 *nvram, u16 length, struct goodix_ts_data *ts )
{
    u8 ret;
    u8 error = 0;
    //struct tpd_info_t tpd_info;
    
	gpio_direction_output(ts->pdata->int_port, 0);

    msleep( 20 );
    ret = gt818_reset(ts);
    if ( ret < 0 )
    {
        error = 1;
        printk(KERN_INFO"reset fail\n");
        goto end;
    }

    ret = gt818_set_address_2( ts );
    if ( ret == 0 )
    {
        error = 1;
        printk(KERN_INFO"set address fail\n");
        goto end;
    }

    ret = gt818_update_firmware( nvram, length, ts);
    if ( ret == 0 )
    {
        error=1;
        printk(KERN_INFO"firmware update fail\n");
        goto end;
    }

end:
    
    gpio_direction_output(ts->pdata->int_port, 1);
    msleep(100);
    gpio_free(ts->pdata->int_port);
	

    
    ret = gt818_reset(ts);
    if ( ret < 0 )
    {
        error=1;
        printk(KERN_INFO"final reset fail\n");
        goto end;
    }
    if ( error == 1 )
    {
        return 0;
    }

    i2c_pre_cmd(ts);
    while(goodix_read_version(ts)<0);

    i2c_end_cmd(ts);
    return 1;
}

#if defined(VKEY_SYS)
static ssize_t gt818_virtual_keys_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
#if 0//chenglong for S801 tp
		return sprintf(buf,
			__stringify(EV_KEY) ":" __stringify(KEY_BACK)  ":70:850:90:70"
		   ":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":240:850:90:70"
		   ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":410:850:90:70"
		   "\n");
#else
	return sprintf(buf,
			__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":60:840:90:60"
		   ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":180:840:90:60"
		   ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":300:840:90:60"
		   ":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH)   ":420:840:90:60"
		   "\n");
#endif
} 
static struct kobj_attribute gt818_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.gt818-ts",//"virtualkeys.Synaptics-RMI4",
		.mode = S_IRUGO,
	},
	.show = &gt818_virtual_keys_show,
};
 
static struct attribute *gt818_properties_attrs[] = {
	&gt818_virtual_keys_attr.attr,
	NULL
};
static struct attribute_group gt818_properties_attr_group = {
	.attrs = gt818_properties_attrs,
};
#endif
/*******************************************************	
功能：
	触摸屏探测函数
	在注册驱动时调用(要求存在对应的client)；
	用于IO,中断等资源申请；设备注册；触摸屏初始化等工作
参数：
	client：待驱动的设备结构体
	id：设备ID
return：
	执行结果码，0表示正常执行
********************************************************/
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	//TODO:在测试失败后需要释放ts
	int ret = 0;
	int retry=0;
	uint8_t goodix_id[3] = {0,0xff,0};
	unsigned char update_path[1] = {0};
	struct goodix_ts_data *ts;
#if defined(VKEY_SYS)        
        struct kobject *properties_kobj;
	ret = -1;
	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		ret = sysfs_create_group(properties_kobj,
					&gt818_properties_attr_group);
	if (!properties_kobj || ret) {
		pr_err("failed to create board_properties\n");
	}
#endif
	//dev_dbg(&client->dev,"Install touch driver.\n")
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ts->pdata = client->dev.platform_data;
	
	i2c_connect_client = client;	//used by Guitar_Update
	   
	//check if Touch panel is GT818	
	ts->client = client;
	gpio_request(ts->pdata->tp_rst, "tp_reset");
	
	
	for(retry=0;retry < 3; retry++){
	    gpio_direction_output(ts->pdata->tp_rst,1);
	    msleep(20);
	    gpio_direction_output(ts->pdata->tp_rst,0);
	    msleep(20);
	    gpio_direction_output(ts->pdata->tp_rst,1);
	    gpio_free(ts->pdata->tp_rst);
             printk("goodix_ts_probe 3\n");

	    msleep(50);
            ret =i2c_write_bytes(client, NULL, 0);	//Test I2C connection.
		if (ret > 0)
			break;

	} 
         printk("goodix_ts_probe 4\n");

	//Check I2C function
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
	    dev_err(&client->dev, "Must have I2C_FUNC_I2C.\n");
	    ret = -ENODEV;
	    goto err_check_functionality_failed;
	}

	
	if(verify_gt818(ts)) {
		printk("gt818 TP  probe failed\r\n");
		ret = -ENOENT;
		goto err_i2c_failed;
	}


#if defined(NO_DEFAULT_ID)
	if(ts->pdata->int_port){
		for(retry = 0; retry < 3; retry++){
			gpio_direction_output(10,0);
			msleep(1);
			gpio_direction_input(10);
			msleep(20);
		
			ret =i2c_write_bytes(client, NULL, 0);	//Test I2C connection.
			if (ret > 0)
				break;
		}
		if(ret <= 0){
			gpio_direction_output(11,0);
			msleep(1);
			gpio_direction_output(10,0);
			msleep(20);
			gpio_direction_input(10);
			for(retry=0;retry < 80; retry++){
				ret =i2c_write_bytes(client, NULL, 0);	//Test I2C connection.
				if (ret > 0){
					msleep(10);
					ret =i2c_read_bytes(client, goodix_id, 3);	//Test I2C connection.
					if (ret > 0){
						if(goodix_id[2] == 0x55){
							gpio_direction_output(ts->pdata->int_port,1);
							msleep(1);
							gpio_free(ts->pdata->int_port);
							msleep(10);
							break;						
						}
					}			
				}	
			
			}
		}
	}
#endif


		
	if(ret <= 0){
		dev_err(&client->dev, "Warnning: I2C communication might be ERROR!\n");
		goto err_i2c_failed;
        }	
	
	INIT_WORK(&ts->work, goodix_ts_work_func);		//init work_struct
	ts->client = client;

	i2c_set_clientdata(client, ts);

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		dev_dbg(&client->dev,"goodix_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE); 						// absolute coor (x,y)
#ifdef HAVE_TOUCH_KEY
	for(retry = 0; retry < MAX_KEY_NUM; retry++)	{
		input_set_capability(ts->input_dev,EV_KEY,touch_key_array[retry]);	
	}
#endif

	input_set_abs_params(ts->input_dev, ABS_Y, 0, SCREEN_MAX_HEIGHT, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_X, 0, SCREEN_MAX_WIDTH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, MAX_FINGER_NUM, 0, 0);
        set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit); 	
#ifdef GOODIX_MULTI_TOUCH
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_HEIGHT, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_WIDTH, 0, 0);	
#endif	

	sprintf(ts->phys, "input/ts");
	ts->input_dev->name = s3c_ts_name;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0xDEAD;
	ts->input_dev->id.product = 0xBEEF;
	ts->input_dev->id.version = 10427;	//screen firmware version
	
	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev,"Probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	ts->bad_data = 0;

	if (ts->pdata->int_port){
		ret = gpio_request(ts->pdata->int_port, "gt818-ts");	//Request IO
		if (ret < 0) {
			dev_err(&client->dev, "Failed to request GPIO:%d, ERRNO:%d\n",(int)ts->pdata->int_port,ret);
			goto err_gpio_request_failed;
		}
		  
		gpio_direction_output(ts->pdata->int_port, 0);
#ifdef AUTO_UPDATA_GT818 //begin AUTO updata firmware
       goodix_read_version(ts);
       ret = gt818_downloader(ts,goodix_gt818_firmware,update_path);
       if(ret < 0){

		dev_err(&client->dev,"warning:GT818 updata maight be erron:\n");

       }
    
#endif
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
       goodix_proc_entry = create_proc_entry("goodix-update",0666,NULL); 
       if(goodix_proc_entry ==NULL){
           dev_info(&client->dev,"Couldn't create proc entry!\n");
	   ret = -ENOMEM;
	   goto err_create_proc_entry;
	
       }
	
       else{
		   
           goodix_proc_entry ->write_proc = goodix_update_write;
           goodix_proc_entry ->read_proc = goodix_update_read;
       }
#endif
	gpio_free(ts->pdata->int_port);
	gpio_request(ts->pdata->int_port, "gt818-ts");	//Request IO
	 gpio_direction_input(11);
				



	#if INT_TRIGGER==0
		#define GT801_PLUS_IRQ_TYPE IRQ_TYPE_EDGE_RISING
	#elif INT_TRIGGER==1
		#define GT801_PLUS_IRQ_TYPE IRQ_TYPE_EDGE_FALLING
//	#elif INT_TRIGGER==2
//		#define GT801_PLUS_IRQ_TYPE IRQ_TYPE_LEVEL_LOW
//	#elif INT_TRIGGER==3
//		#define GT801_PLUS_IRQ_TYPE IRQ_TYPE_LEVEL_HIGH
	#endif
		ret  = request_irq(gpio_to_irq(ts->pdata->int_port), goodix_ts_irq_handler ,  GT801_PLUS_IRQ_TYPE,
			client->name, ts);
		if (ret != 0) {
			dev_err(&client->dev,"Cannot allocate ts INT!ERRNO:%d\n", ret);
			gpio_direction_input(11);
			gpio_free(11);
			goto err_gpio_request_failed;
		}
		else 
		{	
			//disable_irq(client->irq);
			ts->use_irq = 1;
			dev_dbg(&client->dev,"Reques EIRQ %d succesd on GPIO:%d\n",gpio_to_irq(ts->pdata->int_port),ts->pdata->int_port);
		}	
	}
	
err_gpio_request_failed:
	
	if (!ts->use_irq) 
	{
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = goodix_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	i2c_pre_cmd(ts);
	msleep(2);
	
	for(retry=0; retry<3; retry++)
	{
		ret=goodix_init_panel(ts);
		dev_info(&client->dev,"the config ret is :%d\n",ret);
		msleep(2);
		if(ret != 0)	//Initiall failed
			continue;
		else
			break;
	}
	if(ret != 0) {
		ts->bad_data=1;
		goto err_init_godix_ts;
	}
	
	//if(ts->use_irq)
	//	enable_irq(gpio_to_irq(ts->pdata->int_port));
		
	ts->power = goodix_ts_power;

	goodix_read_version(ts);
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = goodix_ts_early_suspend;
	ts->early_suspend.resume = goodix_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif


	i2c_end_cmd(ts);
	dev_info(&client->dev,"Start %s in %s mode\n", 
		ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");
	return 0;

err_init_godix_ts:
	i2c_end_cmd(ts);
	if(ts->use_irq)
	{
		ts->use_irq = 0;
		free_irq(client->irq,ts);
	if (ts->pdata->int_port)
		{
			//gpio_direction_input(11);
			gpio_free(11);
		}	
	}
	else 
		hrtimer_cancel(&ts->timer);

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
	i2c_set_clientdata(client, NULL);
err_i2c_failed:	
	kfree(ts);
	if (ts->pdata->int_port)
	{
		gpio_direction_input(11);
		gpio_free(11);
	}	
	
err_alloc_data_failed:
err_check_functionality_failed:
	
err_create_proc_entry:
probe_exit:

	return ret;
}

/*******************************************************
Function:
	Touch down report function.

Input:
	ts:private data.
	id:tracking id.
	x:input x.
	y:input y.
	w:input weight.
	
Output:
	None.
*******************************************************/
static void gtp_touch_down(struct goodix_ts_data* ts,s32 id,s32 x,s32 y,s32 w)
{
#if GTP_CHANGE_X2Y
    GTP_SWAP(x, y);
#endif
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);			
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_mt_sync(ts->input_dev);
   // GTP_DEBUG("ID=%d, X=%d, Y=%d, W=%d ", id, x, y, w);
}

/*******************************************************
Function:
	Touch up report function.

Input:
	ts:private data.
	
Output:
	None.
*******************************************************/

   #define GTP_MAX_WIDTH    480
    #define GTP_MAX_HEIGHT   800
    #define GTP_MAX_TOUCH    5
    #define GTP_INT_TRIGGER  1
    #define GTP_REFRESH      0

#define GTP_REG_CONFIG_DATA   0x6A2
#define GTP_REG_INDEX         0x712
#define GTP_REG_KEY           0x721
#define GTP_REG_COOR          0x722
#define GTP_REG_SLEEP         0x692
#define GTP_REG_SENSOR_ID     0x721
#define GTP_REG_VERSION       0x715
static void gtp_touch_up(struct goodix_ts_data* ts)
{
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
    input_mt_sync(ts->input_dev);
}

#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_LENGTH     106
static s32 gtp_i2c_pre_cmd(struct goodix_ts_data *ts)
{
    s32 ret;
    u8 pre_cmd_data[2]={0};	
    pre_cmd_data[0]=0x0f;
    pre_cmd_data[1]=0xff;
    ret = i2c_write_bytes(ts->client,pre_cmd_data,2);

    return ret;
}

static void goodix_ts_work_func(struct work_struct *work)
{	
    u8  index_data[3] = {(u8)(GTP_REG_INDEX>>8),(u8)GTP_REG_INDEX,0};
    u8  key_data[3] = {(u8)(GTP_REG_KEY>>8), (u8)GTP_REG_KEY, 0};
    u8  point_data[8*GTP_MAX_TOUCH+2] = {(u8)(GTP_REG_COOR>>8), (u8)GTP_REG_COOR, 0};  
    u8  track_id = 0;
    u8  touch_num = 0;
    u8  key_value = 0;
    u8  position = 0;
    u16 input_x = 0;
    u16 input_y = 0;
    u16 input_w = 0;
    u32 count = 0;
    s32 ret = -1;
     static uint8_t  last_key = 0;

    struct goodix_ts_data *ts;
    

    ts = container_of(work, struct goodix_ts_data, work);
    //gtp_i2c_pre_cmd(ts);
    i2c_pre_cmd(ts);
    //i2c_read_bytes
    ret = i2c_read_bytes(ts->client, index_data, 3);
    if (ret <= 0)
    {
        //GTP_ERROR("I2C transfer error. Number:%d\n ", ret);
        goto exit_work_func;
    }
	
#ifdef HAVE_TOUCH_KEY	
    ret = i2c_read_bytes(ts->client, key_data, 3); 
    if (ret <= 0)
    {
       // GTP_ERROR("I2C transfer error. Number:%d\n ", ret);
        goto exit_work_func;
    }
    key_value = key_data[GTP_ADDR_LENGTH]&0x0f;

#endif
  #if 0  
    //GTP_DEBUG_ARRAY(index_data, 3);
    if (index_data[GTP_ADDR_LENGTH] == 0x0f)
    {
        //GTP_INFO("Reload config!");
        ret = gtp_send_cfg(ts->client);
        if (ret)
        {
            printk("Send config error.");
        }
        goto exit_work_func;
    }	
  #endif  
    if ((index_data[GTP_ADDR_LENGTH]&0x30)!=0x20)
    {
       // GTP_ERROR("Data NO ready!");
        goto exit_work_func;
    }
    
    touch_num = index_data[GTP_ADDR_LENGTH]&0x0f;
    //GTP_DEBUG("Touch num:%d", touch_num);
    if(touch_num)
    {
        ret = i2c_read_bytes(ts->client, point_data, GTP_ADDR_LENGTH+touch_num*8);
        if(ret <= 0)	
        {
            //GTP_ERROR("I2C transfer error. Number:%d\n ", ret);
            goto exit_work_func;
        }
        
      //  GTP_DEBUG_ARRAY(point_data, GTP_ADDR_LENGTH+touch_num*8);        
        for (count=0; count<touch_num; count++)
        {
            position = GTP_ADDR_LENGTH + count*8;
            track_id = point_data[position] - 1;
            input_x = (u16)(point_data[position+2]<<8) + (u16)point_data[position+1];
            input_y = (u16)(point_data[position+4]<<8) + (u16)point_data[position+3];
            input_w = (u16)(point_data[position+6]<<8) + (u16)point_data[position+5];
            input_w = 20;
            
            if ((input_x > 480)||(input_y > 800))
            {
                continue;
            }
            gtp_touch_down(ts, track_id, input_x, input_y, input_w);
        }
    }
    else
    {
        //GTP_DEBUG("Touch Release!");
        gtp_touch_up(ts);
    }

#ifdef HAVE_TOUCH_KEY
   // for(count = 0; count < MAX_KEY_NUM; count++)
   // {
   //     input_report_key(ts->input_dev, touch_key_array[count], !!(key_value&(0x01<<count)));	
   // }
        if((last_key == 0)&&(key_value == 0))
                ;
        else
        {
         for(count = 0; count < MAX_KEY_NUM; count++)
                //for(count = 0; count < 4; count++)
                {
                        input_report_key(ts->input_dev, touch_key_array[count], !!(key_value&(0x01<<count)));
                }
        }
        last_key = key_value;


#endif
    input_report_key(ts->input_dev, BTN_TOUCH, (touch_num || key_value));
    input_sync(ts->input_dev);

exit_work_func:
    i2c_end_cmd(ts);
    //if(ts->use_irq)
   // {
    
               // enable_irq(gpio_to_irq(ts->pdata->int_port));

    //}
}





/*******************************************************	
功能：
	驱动资源释放
参数：
	client：设备结构体
return：
	执行结果码，0表示正常执行
********************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
	remove_proc_entry("goodix-update", NULL);
#endif
	if (ts && ts->use_irq) 
	{
		if (ts->pdata->int_port)
		{
			gpio_direction_input(ts->pdata->int_port);
			gpio_free(ts->pdata->int_port);
		 }
		free_irq(client->irq, ts);
	}	
	else if(ts)
		hrtimer_cancel(&ts->timer);
	
	dev_notice(&client->dev,"The driver is removing...\n");
	i2c_set_clientdata(client, NULL);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

//停用设备
static int goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
        unsigned long flag;
#if 0    
	if (ts->use_irq)
		//disable_irq(gpio_to_irq(ts->pdata->int_port));
                printk("TP enter suspend\n");
	else
		hrtimer_cancel(&ts->timer);

	if (ts->power) {	/* 必须在取消work后再执行，避免因GPIO导致坐标处理代码死循环	*/
		ret = ts->power(ts, 0);
		if (ret < 0)
			printk(KERN_ERR "goodix_ts_resume power off failed\n");
	}
#endif
	return 0;
}

//重新唤醒
static int goodix_ts_resume(struct i2c_client *client)
{
	int ret;
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
#if 0
	if (ts->power) {
		ret = ts->power(ts, 1);
		if (ret < 0)
			printk(KERN_ERR "goodix_ts_resume power on failed\n");
	}

	if (ts->use_irq)
                printk("TP resume\n");
		//enable_irq(gpio_to_irq(ts->pdata->int_port));
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

#endif
 	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
	struct goodix_ts_data *ts;
	ts = container_of(h, struct goodix_ts_data, early_suspend);
	goodix_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void goodix_ts_late_resume(struct early_suspend *h)
{
	struct goodix_ts_data *ts;
	ts = container_of(h, struct goodix_ts_data, early_suspend);
	goodix_ts_resume(ts->client);
}
#endif

//******************************Begin of firmware update surpport*******************************
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
static int  update_read_version(struct goodix_ts_data *ts, char **version)
{
	int ret = -1, count = 0;
	//unsigned char version_data[18];
	char *version_data;
	char *p;
	
	*version = (char *)vmalloc(5);
	version_data = *version;
	if(!version_data)
		return -ENOMEM;
	p = version_data;
	memset(version_data, 0, sizeof(version_data));
	version_data[0]=0x07;	
	version_data[1]=0x17;	
	ret=i2c_read_bytes(ts->client,version_data, 4);
	if (ret < 0) 
		return ret;
	version_data[5]='\0';
	
	if(*p == '\0')
		return 0; 	
	do 					
	{
		if((*p > 122) || (*p < 48 && *p != 32) || (*p >57 && *p  < 65) 
			||(*p > 90 && *p < 97 && *p  != '_'))		//check illeqal character
			count++;
	}while(*++p != '\0' );
	if(count > 2)
		return 0;
	else 
		return 1;	
}

/**
@brief CRC cal proc,include : Reflect,init_crc32_table,GenerateCRC32
@param global var oldcrc32
@return states
*/
static unsigned int Reflect(unsigned long int ref, char ch)
{
	unsigned int value=0;
	int i;
	for(i = 1; i < (ch + 1); i++)
	{
		if(ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}
/*---------------------------------------------------------------------------------------------------------*/
/*  CRC Check Program INIT								                                           		   */
/*---------------------------------------------------------------------------------------------------------*/
#if 0
static void init_crc32_table(void)
{
	unsigned int temp;
	unsigned int t1,t2;
	unsigned int flag;
	int i,j;
	for(i = 0; i <= 0xFF; i++)
	{
		temp=Reflect(i, 8);
		crc32_table[i]= temp<< 24;
		for (j = 0; j < 8; j++)
		{

			flag=crc32_table[i]&0x80000000;
			t1=(crc32_table[i] << 1);
			if(flag==0)
				t2=0;
			else
				t2=ulPolynomial;
			crc32_table[i] =t1^t2 ;

		}
		crc32_table[i] = Reflect(crc32_table[i], 32);
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/*  CRC main Program									                                           		   */
/*---------------------------------------------------------------------------------------------------------*/
static void GenerateCRC32(unsigned char * buf, unsigned int len)
{
	unsigned int i;
	unsigned int t;

	for (i = 0; i != len; ++i)
	{
		t = (oldcrc32 ^ buf[i]) & 0xFF;
		oldcrc32 = ((oldcrc32 >> 8) & 0xFFFFFF) ^ crc32_table[t];
	}
}

#endif


unsigned char wait_slave_ready(struct goodix_ts_data *ts, unsigned short *timeout)
{
	unsigned char i2c_state_buf[2] = {ADDR_STA, UNKNOWN_ERROR};
	int ret;
	while(*timeout < MAX_TIMEOUT)
	{
		ret = i2c_read_bytes(ts->client, i2c_state_buf, 2);
		if(ret <= 0)
			return ERROR_I2C_TRANSFER;
		if(i2c_state_buf[1] & SLAVE_READY)
		{
			return 1;
		}
		msleep(1);
		*timeout += 5;
	}
	return 0;
}

static int goodix_update_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
        unsigned char cmd[120];
        int ret = -1;
       int retry = 0;
        static unsigned char update_path[60];
        struct goodix_ts_data *ts;

        ts = i2c_get_clientdata(i2c_connect_client);
        if(ts==NULL)
        {
            printk(KERN_INFO"goodix write to kernel via proc file!@@@@@@\n");
                return 0;
        }

        //printk(KERN_INFO"goodix write to kernel via proc file!@@@@@@\n");
        if(copy_from_user(&cmd, buff, len))
        {
            printk(KERN_INFO"goodix write to kernel via proc file!@@@@@@\n");
                return -EFAULT;
        }
        //printk(KERN_INFO"Write cmd is:%d,write len is:%ld\n",cmd[0], len);
        switch(cmd[0])
        {
            case APK_UPDATE_TP:
            printk(KERN_INFO"Write cmd is:%d,cmd arg is:%s,write len is:%ld\n",cmd[0], &cmd[1], len);
            memset(update_path, 0, 60);
            strncpy(update_path, cmd+1, 60);
            goodix_read_version(ts);
            ret = gt818_downloader( ts, goodix_gt818_firmware, update_path);

             if(ret < 0)
            {
                printk(KERN_INFO"Warnning: GT818 update might be ERROR!\n");
                return 0;
            }

            i2c_pre_cmd(ts);
             msleep(2);

            for(retry=0; retry<3; retry++)
            {
                    ret=goodix_init_panel(ts);
                    printk(KERN_INFO"the config ret is :%d\n",ret);

                    msleep(2);
                    if(ret != 0)        //Initiall failed
                            continue;
                    else
                            break;
            }

           if(ts->use_irq)
           {


			       gpio_free(ts->pdata->int_port);
				   
				   gpio_request(ts->pdata->int_port, "gt818-ts");  //Request IO
				   gpio_direction_input(ts->pdata->int_port);

								   
			  // gpio_request(ts->pdata->int_port, "gt818-ts"); //Request IO
			  // gpio_direction_input(ts->pdata->int_port);
			      gpio_to_irq(ts->pdata->int_port);
		   }
               
            else
                //gpio_direction_input(INT_PORT);
            	gpio_direction_input(ts->pdata->int_port);

           i2c_end_cmd(ts);

            if(ret != 0)
            {
                    ts->bad_data=1;
                    return 1;
              }
            return 1;

            case APK_READ_FUN:                                                  //functional command
                        if(cmd[1] == CMD_READ_VER)
                        {
                                printk(KERN_INFO"Read version!\n");
                                ts->read_mode = MODE_RD_VER;
                        }
                    else if(cmd[1] == CMD_READ_CFG)
                        {
                                printk(KERN_INFO"Read config info!\n");
                                ts->read_mode = MODE_RD_CFG;
                        }
            return 1;

            case APK_WRITE_CFG:
                        printk(KERN_INFO"Begin write config info!Config length:%d\n",cmd[1]);
                        i2c_pre_cmd(ts);
                    ret = i2c_write_bytes(ts->client, cmd+2, cmd[1]+2);
                    i2c_end_cmd(ts);
                    if(ret != 1)
                    {
                        printk("Write Config failed!return:%d\n",ret);
                        return -1;
                    }
                    return 1;

                default:
                        return 0;
        }
        return 0;
}

static int goodix_update_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
        int ret = -1;
        struct goodix_ts_data *ts;
        int len = 0;
        unsigned char read_data[360] = {80, };

        ts = i2c_get_clientdata(i2c_connect_client);
        if(ts==NULL)
                return 0;

       printk("___READ__\n");
        if(ts->read_mode == MODE_RD_VER)                //read version data
        {
                i2c_pre_cmd(ts);
                ret = goodix_read_version(ts);
             i2c_end_cmd(ts);
                if(ret < 0)
                {
                        printk(KERN_INFO"Read version data failed!\n");
                        return 0;
                }

             read_data[1] = (char)(ts->version&0xff);
             read_data[0] = (char)((ts->version>>8)&0xff);

                printk(KERN_INFO"Gt818 ROM version is:%x%x\n", read_data[0],read_data[1]);
                memcpy(page, read_data, 2);
                //*eof = 1;
                return 2;
        }
       else if(ts->read_mode == MODE_RD_CFG)
        {

            read_data[0] = 0x06;
            read_data[1] = 0xa2;       // cfg start address
            printk("read config addr is:%x,%x\n", read_data[0],read_data[1]);

             len = 106;
           i2c_pre_cmd(ts);
             ret = i2c_read_bytes(ts->client, read_data, len+2);
            i2c_end_cmd(ts);
            if(ret <= 0)
                {
                        printk(KERN_INFO"Read config info failed!\n");
                        return 0;
                }

                memcpy(page, read_data+2, len);
                return len;
        }
        return len;
}
#endif
//******************************End of firmware update surpport*******************************
//可用于该驱动的 设备名—设备ID 列表
//only one client
static const struct i2c_device_id goodix_ts_id[] = {
	{ GOODIX_I2C_NAME, 0 },
	{ }
};

//设备驱动结构体
static struct i2c_driver goodix_ts_driver = {
	.probe		= goodix_ts_probe,
	.remove		= goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= goodix_ts_suspend,
	.resume		= goodix_ts_resume,
#endif
	.id_table	= goodix_ts_id,
	.driver = {
		.name	= GOODIX_I2C_NAME,
		.owner = THIS_MODULE,
	},
};

/*******************************************************	
功能：
	驱动加载函数
return：
	执行结果码，0表示正常执行
********************************************************/
static int __devinit goodix_ts_init(void)
{
	int ret;
	
	goodix_wq = create_workqueue("goodix_wq");		//create a work queue and worker thread
	if (!goodix_wq) {
		printk(KERN_ALERT "creat workqueue faiked\n");
		return -ENOMEM;
		
	}
	ret=i2c_add_driver(&goodix_ts_driver);
	return ret; 
}

/*******************************************************	
功能：
	驱动卸载函数
参数：
	client：设备结构体
********************************************************/
static void __exit goodix_ts_exit(void)
{
	printk(KERN_ALERT "Touchscreen driver of guitar exited.\n");
	i2c_del_driver(&goodix_ts_driver);
	if (goodix_wq)
		destroy_workqueue(goodix_wq);		//release our work queue
}

late_initcall(goodix_ts_init);				//最后初始化驱动felix
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("Goodix Touchscreen Driver");
MODULE_LICENSE("GPL");

