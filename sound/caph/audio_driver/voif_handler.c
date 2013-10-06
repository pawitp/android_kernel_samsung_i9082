/****************************************************************************
Copyright 2009 - 2011  Broadcom Corporation
 Unless you and Broadcom execute a separate written software license agreement
 governing use of this software, this software is licensed to you under the
 terms of the GNU General Public License version 2 (the GPL), available at
    http://www.broadcom.com/licenses/GPLv2.php

 with the following added to such license:
 As a special exception, the copyright holders of this software give you
 permission to link this software with independent modules, and to copy and
 distribute the resulting executable under terms of your choice, provided
 that you also meet, for each linked independent module, the terms and
 conditions of the license of that module.
 An independent module is a module which is not derived from this software.
 The special exception does not apply to any modifications of the software.
 Notwithstanding the above, under no circumstances may you combine this software
 in any way with any other Broadcom software provided under a license other than
 the GPL, without Broadcom's express prior written consent.
***************************************************************************/
/**
*
*   @file   voif_handler.c
*
*   @brief  PCM data interface to DSP.
*           It is used to hook with customer's downlink voice processing
*			module. Customer will implement this.

* ENABLE_DIAMOND_SOLUTION
* ENABLE_NXPEX_SOLUTION
*
****************************************************************************/
#include <linux/string.h>

#ifdef CONFIG_ENABLE_VOIF
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#endif

#include "mobcom_types.h"
#include "audio_consts.h"
#include "voif_handler.h"
#include "audio_ddriver.h"
#include "audio_trace.h"

#ifdef  VOICESOLUTION_DUMP
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#endif

#ifdef CONFIG_ENABLE_VOIF
#include <linux/wakelock.h>
#endif

/* APIs */

#ifndef CONFIG_ENABLE_VOIF
static int voifDelay; /* init to 0 */
static int voifGain = 0x4000; /* In Q14 format, 0x4000 in Q14 == 1.0 */
#endif

#ifdef CONFIG_ENABLE_VOIF
static void *drv_handle; /* init to NULL */
static VoIF_CallType_t voifCallType = VOIF_NO_CALL; /* init to VOIF_NO_CALL */
static VoIF_HeadsetType_t voifHeadsetType = VOIF_OTHER_TYPE; /* init to VOIF_NO_CALL */
static struct wake_lock voif_lock;
static int lock_init;
static short isLoopbackMode = 0;
static int curVolIndex = 4;

#define	VOICE_NB_FRAME	160
#define	VOICE_WB_FRAME	320

typedef struct
{
    Int16 pUldata_nb[VOICE_NB_FRAME];
    Int16 pDldata_nb[VOICE_NB_FRAME];
    Int16 pUldata_wb[VOICE_WB_FRAME];
    Int16 pDldata_wb[VOICE_WB_FRAME];
    
    Int16 pDlOutput_nb_buf[VOICE_NB_FRAME];
    Int16 pUlOutput_nb_buf[VOICE_NB_FRAME];
    Int16 pDlOutput_wb_buf[VOICE_WB_FRAME];
    Int16 pUlOutput_wb_buf[VOICE_WB_FRAME];

    Int16 callMode;
    Int16 isInit;
    
#ifdef ENABLE_DIAMOND_SOLUTION    
    void (*pDiamondVoiceExe)(void);
    short (*pDiamondVoice_Config)(int, int, short *, short *, short *, const short*, int);
    void (*pDiamondVoice_Volume_Config)(int, int, int);
    short DiamondVoice_Mode;
#endif  
#ifdef ENABLE_NXPEX_SOLUTION
    int (*pLVVEFS_Process)(short *, short *,unsigned int , unsigned char );
    int (*pLVVEFS_Set_Device)(unsigned int);
    int (*pLVVEFS_Set_Volume)(unsigned int);
    int (*pLVVEFS_Reset)(void);    
    static unsigned int deviceID;
#endif
}TVoIF_CallBackData;

static TVoIF_CallBackData sgCallbackData;

#ifdef ENABLE_DIAMOND_SOLUTION    

#define DHA_PARAM_MAX   14

short DiamondVoiceDhaDataChk = 0;
short DiamodVoiceDhaParams[DHA_PARAM_MAX] = {0};
extern void DiamondVoice_Exe(void);
extern short DiamondVoice_Config(int , int, Int16 *, Int16 *, Int16 *, const Int16*, int);
extern void DiamondVoice_Volume_Config(int, int, int);
#endif
#ifdef ENABLE_NXPEX_SOLUTION  
extern int LVVEFS_Process(short *ulData, short *dlData, unsigned int sampleCount, unsigned char  isCallWB );
extern int LVVEFS_Set_Device(unsigned int device);
extern int LVVEFS_Set_Volume(unsigned int volume);
extern int LVVEFS_Reset(void);
#endif

#endif

#ifdef  VOICESOLUTION_DUMP
#ifdef ENABLE_INJECTION
#define INJECTION__TEST_FILE_PATH "/data/inject.pcm"
#define INJECTION_TX_IN_FILE_PATH "/data/injection_tx_in.pcm"
#define INJECTION_TX_OUT_FILE_PATH "/data/injection_tx_out.pcm"
#define INJECTION_RX_OUT_FILE_PATH "/data/injection_rx_out.pcm"

typedef struct 
{
    struct file *fp_injection;
    struct work_struct work_read;   

    Int16 flag;
}TVoif_Inject;

typedef struct 
{
    struct file *fp_tx_in, *fp_tx_out, *fp_rx_out;
    struct work_struct work_write;   
}TVoif_Dump;

static TVoif_Dump sgDumpData;

static TVoif_Inject sgInjectData;

#define MAX_INJECT 1500

static Int16 inject_array[MAX_INJECT][160];
static Int16 tx_in_array[MAX_INJECT][160];
static Int16 tx_out_array[MAX_INJECT][160];
static Int16 rx_out_array[MAX_INJECT][160];
static Int16 index;

static void work_output_thread(struct work_struct *work)
{
    int i=0;

    sgDumpData.fp_tx_in = filp_open(INJECTION_TX_IN_FILE_PATH,  O_WRONLY |O_APPEND |O_LARGEFILE |O_CREAT,0666);
    sgDumpData.fp_tx_out = filp_open(INJECTION_TX_OUT_FILE_PATH,  O_WRONLY |O_APPEND |O_LARGEFILE |O_CREAT,0666);
    sgDumpData.fp_rx_out = filp_open(INJECTION_RX_OUT_FILE_PATH,  O_WRONLY |O_APPEND |O_LARGEFILE |O_CREAT,0666);

    for ( i=0 ; i < index ;i++) {
// Tx
        sgDumpData.fp_tx_in->f_op->write( sgDumpData.fp_tx_in, (const char __user *)tx_in_array[i], sizeof(Int16) * VOICE_NB_FRAME, &sgDumpData.fp_tx_in->f_pos );
        sgDumpData.fp_tx_out->f_op->write( sgDumpData.fp_tx_out, (const char __user *)tx_out_array[i], sizeof(Int16) * VOICE_NB_FRAME, &sgDumpData.fp_tx_out->f_pos );
// Rx
        sgDumpData.fp_rx_out->f_op->write( sgDumpData.fp_rx_out, (const char __user *)rx_out_array[i], sizeof(Int16) * VOICE_NB_FRAME, &sgDumpData.fp_rx_out->f_pos );
    }

    if ( sgDumpData.fp_tx_in )
        filp_close(sgDumpData.fp_tx_in, NULL);
    
    if ( sgDumpData.fp_tx_out )
        filp_close(sgDumpData.fp_tx_out, NULL);
    
    if ( sgDumpData.fp_rx_out )
        filp_close(sgDumpData.fp_tx_out, NULL);

    index = 0;
}

static void work_read_thread(struct work_struct *work)
{
    char line[320];
    int i=0;
    loff_t offset=0;
    loff_t result=0;
   
    if( !IS_ERR(sgInjectData.fp_injection)) {   
        
        while(i<MAX_INJECT)
        {
            result = kernel_read(sgInjectData.fp_injection,  offset , line, 320);             
            memcpy(inject_array[i++], line,320);           
            offset += result;            
        }
		sgInjectData.flag =1;

        if (  sgInjectData.fp_injection )
            filp_close( sgInjectData.fp_injection, NULL);
    }       
}

#else //ENABLE_INJECTION
#define DIAMOND_BEFORE_FILE_PATH "/data/Diamond_Before.pcm"
#define DIAMOND_AFTER_FILE_PATH "/data/Diamond_After.pcm"

typedef struct 
{
    struct file *fp_in, *fp_out;
    struct work_struct work_write;   
}TVoif_Dump;

static TVoif_Dump sgDumpData;

static void work_output_thread(struct work_struct *work)
{
//    printk("\n===============  work_output_thread  =============== \n");
    if(voifCallType == VOIF_VOICE_CALL_NB)
    {
        sgDumpData.fp_in->f_op->write( sgDumpData.fp_in, (const char __user *)sgCallbackData.pDldata_nb, sizeof(Int16) * VOICE_NB_FRAME, &sgDumpData.fp_in->f_pos );
        sgDumpData.fp_out->f_op->write( sgDumpData.fp_out, (const char __user *)sgCallbackData.pDlOutput_nb_buf, sizeof(Int16) * VOICE_NB_FRAME, &sgDumpData.fp_out->f_pos );
    }
    else if(voifCallType == VOIF_VOICE_CALL_WB)
    {
        sgDumpData.fp_in->f_op->write( sgDumpData.fp_in, (const char __user *)sgCallbackData.pDldata_wb, sizeof(Int16) * VOICE_WB_FRAME, &sgDumpData.fp_in->f_pos );
        sgDumpData.fp_out->f_op->write( sgDumpData.fp_out, (const char __user *)sgCallbackData.pDlOutput_wb_buf, sizeof(Int16) * VOICE_WB_FRAME, &sgDumpData.fp_out->f_pos );
    }
}

#endif // ENABLE_INJECTION
#endif // VOICESOLUTION_DUMP

#ifdef CONFIG_ENABLE_VOIF       
static void VOIF_CB_Fxn(
	Int16 *ulData,
	Int16 *dlData,
	UInt32 sampleCount,
	UInt8 isCall16K)
{
#ifdef VOICESOLUTION_DUMP
#ifdef ENABLE_INJECTION
    Int16 isMax = 0;
    if (sgInjectData.flag ==1 && index<=MAX_INJECT )
    {
        if(index == MAX_INJECT)
        {
            schedule_work(&sgDumpData.work_write);
            isMax = 1;
            printk("\n===============  VOIF_CB_Fxn  before=============== index : %d, isMax : %d\n", index, isMax);
        }
        else
        {
            memcpy(dlData, inject_array[index], sizeof(Int16) * VOICE_NB_FRAME);
            memcpy(tx_in_array[index], ulData, sizeof(Int16) * VOICE_NB_FRAME);
            printk("\n===============  VOIF_CB_Fxn  before=============== index : %d, isMax : %d\n", index, isMax);
        }
    }
#endif
#endif

#ifdef ENABLE_DIAMOND_SOLUTION    
    if ( sgCallbackData.DiamondVoice_Mode != 0x00 && (voifCallType == VOIF_VOICE_CALL_NB || voifCallType == VOIF_VOICE_CALL_WB) )
#endif        
    {            
        if(isCall16K == 0 && voifCallType == VOIF_VOICE_CALL_NB)
        {
            memcpy(sgCallbackData.pUldata_nb, ulData, sizeof(Int16) * VOICE_NB_FRAME);
            memcpy(sgCallbackData.pDldata_nb, dlData, sizeof(Int16) * VOICE_NB_FRAME);
        }
        else if(isCall16K == 1 && voifCallType == VOIF_VOICE_CALL_WB)
        {
            memcpy(sgCallbackData.pUldata_wb, ulData, sizeof(Int16) * VOICE_WB_FRAME);
            memcpy(sgCallbackData.pDldata_wb, dlData, sizeof(Int16) * VOICE_WB_FRAME);
        }

#ifdef ENABLE_DIAMOND_SOLUTION    
        sgCallbackData.pDiamondVoiceExe();
#endif
#ifdef ENABLE_NXPEX_SOLUTION 
        sgCallbackData.pLVVEFS_Process(sgCallbackData.pUldata_nb, sgCallbackData.pDldata_nb, sampleCount, isCall16K);
#endif

        if(isCall16K == 0 && voifCallType == VOIF_VOICE_CALL_NB)
        {
            memcpy(dlData, sgCallbackData.pDlOutput_nb_buf, sizeof(Int16) * VOICE_NB_FRAME);
            memcpy(ulData, sgCallbackData.pUldata_nb, sizeof(Int16) * VOICE_NB_FRAME);
        }
        else if(isCall16K == 1 && voifCallType == VOIF_VOICE_CALL_WB)
        {
            memcpy(dlData, sgCallbackData.pDlOutput_wb_buf, sizeof(Int16) * VOICE_WB_FRAME);
            memcpy(ulData, sgCallbackData.pUldata_wb, sizeof(Int16) * VOICE_WB_FRAME);
        }

#ifdef  VOICESOLUTION_DUMP
#ifdef ENABLE_INJECTION
    if (sgInjectData.flag ==1 && index<MAX_INJECT )
    {
        if(isMax == 0)
        {
            memcpy(tx_out_array[index], ulData, sizeof(Int16) * VOICE_NB_FRAME);
            memcpy(rx_out_array[index], dlData, sizeof(Int16) * VOICE_NB_FRAME);
            printk("\n===============  VOIF_CB_Fxn after  =============== index : %d, isMax : %d\n", index, isMax);
        }
        else
        {
            printk("\n===============  VOIF_CB_Fxn after  =============== index : %d, isMax : %d\n", index, isMax);
        }
    }
    index++;
#else
        schedule_work(&sgDumpData.work_write);
#endif
#endif
    }

    return;
}

#else
#if 0
static void VOIF_CB_Fxn(
	Int16 *ulData,
	Int16 *dlData,
	UInt32 sampleCount,
	UInt8 isCall16K)
{
	if (voifDelay == 0) {
		/* copy ulData to dlData without delay, hear own voice lpbk. */
		memcpy(dlData, ulData, sampleCount * sizeof(Int16));
    } 
    else if (voifDelay == 1) {
		Int32 t, i;
        /* Gain test, change gain of downlink, should hear volume difference */

		for (i = 0; i < sampleCount; i++) {
			t = (Int32) *dlData;
			t = (t * voifGain)>>14;
			*dlData++ = (Int16)(t&0xffff);
		}
    }
    else {
		/* delay test, mute the downlink */
		memset(dlData, 0, sampleCount * sizeof(Int16));
	}

    msleep(voifDelay);
	return;
}
#endif
#endif
/* Start voif */
void VoIF_init(AudioMode_t mode, VoIF_HeadsetType_t hstype, VoIF_CallType_t callType)
{
#ifdef CONFIG_ENABLE_VOIF    
    if(isLoopbackMode)
    {
        printk("\n===============  VoIF_init is returned because of loopback mode enabled =============== \n");
        return;
    }
    
    if (!lock_init)
    {
        pr_info("%s: initializing wake lock\n", __func__);
        wake_lock_init(&voif_lock, WAKE_LOCK_SUSPEND, "voif_lock");
        lock_init = 1;
    }
    wake_lock(&voif_lock);
#ifdef ENABLE_DIAMOND_SOLUTION    
    sgCallbackData.pDiamondVoice_Config = symbol_get(DiamondVoice_Config);
    sgCallbackData.pDiamondVoiceExe = symbol_get(DiamondVoice_Exe);
    sgCallbackData.pDiamondVoice_Volume_Config = symbol_get(DiamondVoice_Volume_Config);
#endif
#ifdef ENABLE_NXPEX_SOLUTION
    sgCallbackData.pLVVEFS_Process = symbol_get(LVVEFS_Process);
    sgCallbackData.pLVVEFS_Set_Device = symbol_get(LVVEFS_Set_Device);
    sgCallbackData.pLVVEFS_Set_Volume = symbol_get(LVVEFS_Set_Volume);
    sgCallbackData.pLVVEFS_Reset = symbol_get(LVVEFS_Reset);
    printk("\n===============  VoIF_init  =============== sgCallbackData.pLVVE_Process: %p, sgCallbackData.LVVE_Set_Device=%p\n", sgCallbackData.pLVVEFS_Process,sgCallbackData.pLVVEFS_Set_Device);    
#endif
     
    sgCallbackData.callMode = mode;
    sgCallbackData.isInit = 1;
    voifHeadsetType = hstype;
    voifCallType = callType;

#ifdef ENABLE_DIAMOND_SOLUTION       
    if(callType == VOIF_VOICE_CALL_WB || callType == VOIF_VT_CALL_WB)
    {
        sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(mode, curVolIndex, sgCallbackData.pDldata_wb, sgCallbackData.pUldata_wb, sgCallbackData.pDlOutput_wb_buf, DiamodVoiceDhaParams, VOIF_WIDE_BAND);
    }
    else
    {
        sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(mode, curVolIndex, sgCallbackData.pDldata_nb, sgCallbackData.pUldata_nb, sgCallbackData.pDlOutput_nb_buf, DiamodVoiceDhaParams, VOIF_NARROW_BAND);
    }
    printk("\n===  VoIF_init  === AudioMode_t : %d, DV Mode : %d, callType : 0x%x, curVolIndex : %d \n", mode, sgCallbackData.DiamondVoice_Mode, voifCallType, curVolIndex);
#endif

#ifdef ENABLE_NXPEX_SOLUTION
    //sgCallbackData.pLVVE_Set_Device(mode);
    sgCallbackData.pLVVEFS_Set_Device(mode);    
#endif

	drv_handle = AUDIO_DRIVER_Open(AUDIO_DRIVER_VOIF);
    AUDIO_DRIVER_Ctrl(drv_handle, AUDIO_DRIVER_SET_VOIF_CB, (void *)VOIF_CB_Fxn);
	AUDIO_DRIVER_Ctrl(drv_handle, AUDIO_DRIVER_START, NULL);
#endif

#ifdef VOICESOLUTION_DUMP
#ifdef ENABLE_INJECTION
    sgInjectData.fp_injection = filp_open(INJECTION__TEST_FILE_PATH, O_RDONLY, 0);

    sgInjectData.flag =0;   
    INIT_WORK(&sgInjectData.work_read, work_read_thread);
    schedule_work(&sgInjectData.work_read);
#else
        sgDumpData.fp_in = filp_open(DIAMOND_BEFORE_FILE_PATH,  O_WRONLY |O_TRUNC |O_LARGEFILE |O_CREAT,0666);
        sgDumpData.fp_out = filp_open(DIAMOND_AFTER_FILE_PATH,  O_WRONLY |O_TRUNC |O_LARGEFILE |O_CREAT,0666);
#endif

    INIT_WORK(&sgDumpData.work_write, work_output_thread);
#endif

	return;
}

/* Stop voif */
void VoIF_Deinit()
{
#ifdef CONFIG_ENABLE_VOIF
    if(isLoopbackMode)
    {
        printk("\n===============  VoIF_Deinit is returned because of loopback mode enabled =============== \n");
        return;
    }
#endif

#ifdef VOICESOLUTION_DUMP
#ifdef ENABLE_INJECTION
	schedule_work(&sgDumpData.work_write);
#endif
#endif
#ifdef CONFIG_ENABLE_VOIF
    printk("\n===============  VoIF_Deinit  =============== \n");

	AUDIO_DRIVER_Ctrl(drv_handle, AUDIO_DRIVER_STOP, NULL);
	AUDIO_DRIVER_Close(drv_handle);
	drv_handle = NULL;

#ifdef ENABLE_NXPEX_SOLUTION
    //sgCallbackData.pLVVE_Reset();
    sgCallbackData.pLVVEFS_Reset();
#endif

#endif // CONFIG_ENABLE_VOIF

#ifdef VOICESOLUTION_DUMP
#ifndef ENABLE_INJECTION
    if ( sgDumpData.fp_in )
        filp_close(sgDumpData.fp_in, NULL);
    
    if ( sgDumpData.fp_out )
        filp_close(sgDumpData.fp_out, NULL);
#endif
#endif

#ifdef CONFIG_ENABLE_VOIF    
#ifdef ENABLE_DIAMOND_SOLUTION    
    symbol_put(DiamondVoice_Config);
    symbol_put(DiamondVoice_Exe);
    symbol_put(DiamondVoice_Volume_Config);
#endif
#ifdef ENABLE_NXPEX_SOLUTION
    symbol_put(LVVEFS_Process);
    symbol_put(LVVEFS_Set_Device);
    symbol_put(LVVEFS_Set_Volume);
    symbol_put(LVVEFS_Reset);
#endif
    sgCallbackData.isInit = 0;
    if (lock_init)
        wake_unlock(&voif_lock);
#endif

    return;
}

#ifdef CONFIG_ENABLE_VOIF
/* Update audio mode for voif */
void VoIF_modeChange(AudioMode_t mode, VoIF_HeadsetType_t hstype)
{
    if(!sgCallbackData.isInit)
{
        printk("\n===============VoIF_modeChange()  VoIF_init is not called =============== \n");
        return;
    }
    
    sgCallbackData.callMode = mode;

    if(hstype != -1)
    voifHeadsetType = hstype;
    
#ifdef ENABLE_DIAMOND_SOLUTION
    if(voifCallType == VOIF_VOICE_CALL_WB || voifCallType == VOIF_VT_CALL_WB)
        sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(mode, curVolIndex, sgCallbackData.pDldata_wb, sgCallbackData.pUldata_wb, sgCallbackData.pDlOutput_wb_buf, DiamodVoiceDhaParams, VOIF_WIDE_BAND);
    else
        sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(mode, curVolIndex, sgCallbackData.pDldata_nb, sgCallbackData.pUldata_nb, sgCallbackData.pDlOutput_nb_buf, DiamodVoiceDhaParams, VOIF_NARROW_BAND);
    printk("\n===  VoIF_modeChange - AudioMode_t Mode : %d DV mode : %d, voifCallType : 0x%x, curVolIndex : %d  === \n", mode, sgCallbackData.DiamondVoice_Mode, voifCallType, curVolIndex);
#endif    
#ifdef ENABLE_NXPEX_SOLUTION
    deviceID = voifCallType | sgCallbackData.callMode;

    if(voifHeadsetType == VOIF_HEADPHONE_TYPE)
    {
        deviceID |= 0x100;
    }
    
    printk("\n===============  VoIF_modeChange - AudioMode_t Mode : %d deviceID : %d=============== \n", mode, deviceID);

    sgCallbackData.pLVVEFS_Set_Device(deviceID);
#endif
}

/* Update voice volume index for voif */
void VoIF_volumeSetting(int vol_index)
{
    if(!sgCallbackData.isInit)
    {
        printk("\n===============VoIF_volumeSetting()  VoIF_init is not called =============== \n");
	return;
}

	/* notification of revised voice volume during the call */
	// TO-DO: react at changing voice volume.
    printk("\n===============  VoIF_volumeSetting - callType : %d AudioMode_t mode : %d vol : %d =============== \n", voifCallType, sgCallbackData.callMode, vol_index);

    curVolIndex = vol_index;

#ifdef ENABLE_DIAMOND_SOLUTION
    if(voifCallType == VOIF_VOICE_CALL_WB || voifCallType == VOIF_VT_CALL_WB)
        sgCallbackData.pDiamondVoice_Volume_Config(sgCallbackData.callMode, curVolIndex, VOIF_WIDE_BAND);
    else
        sgCallbackData.pDiamondVoice_Volume_Config(sgCallbackData.callMode, curVolIndex, VOIF_NARROW_BAND);
#endif
    
#ifdef ENABLE_NXPEX_SOLUTION
    sgCallbackData.pLVVEFS_Set_Volume(curVolIndex);
#endif
}

/* Update call type for voif */
void VoIF_setCallType(VoIF_CallType_t callType)
{
    /* notification of revised call type during the call */
    // TO-DO: react at setting call type (voice / vt).

    if(!sgCallbackData.isInit)
    {
        printk("\n===============VoIF_setCallType()  VoIF_init is not called =============== \n");
        return;
    }

	voifCallType = callType;
    
#ifdef ENABLE_DIAMOND_SOLUTION
    if(voifCallType == VOIF_VOICE_CALL_WB || voifCallType == VOIF_VT_CALL_WB)
    {
        sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(sgCallbackData.callMode, curVolIndex, sgCallbackData.pDldata_wb, sgCallbackData.pUldata_wb, sgCallbackData.pDlOutput_wb_buf, DiamodVoiceDhaParams, VOIF_WIDE_BAND);
    }
    else
    {
        sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(sgCallbackData.callMode, curVolIndex, sgCallbackData.pDldata_nb, sgCallbackData.pUldata_nb, sgCallbackData.pDlOutput_nb_buf, DiamodVoiceDhaParams, VOIF_NARROW_BAND);
    }
#endif    

#ifdef ENABLE_NXPEX_SOLUTION
    deviceID = voifCallType | sgCallbackData.callMode;

    if(voifHeadsetType == VOIF_HEADPHONE_TYPE)
    {
        deviceID |= 0x100;
    }
    
    printk("\n===============  VoIF_setCallType - callType : %d AudioMode_t mode : %d, deviceID : %d =============== \n", voifCallType, sgCallbackData.callMode, deviceID);

    sgCallbackData.pLVVEFS_Set_Device(deviceID);
#else
    printk("\n===============  VoIF_setCallType - callType : %d AudioMode_t mode : %d, curVolIndex : %d =============== \n", voifCallType, sgCallbackData.callMode, curVolIndex);
#endif
}

/* Return call type for voif */
VoIF_CallType_t VoIF_getCallType(void)
{
    if(!sgCallbackData.isInit)
    {
        printk("\n===============VoIF_getCallType()  VoIF_init is not called =============== \n");
        return VOIF_NO_CALL;
    }

	return voifCallType;
}

void VoIF_setLoopbackMode(short isLoopback)
{
    isLoopbackMode = isLoopback;
    printk("\n===============VoIF_setLoopbackMode()  isLoopback : %d =============== \n", isLoopbackMode);
}

#ifdef ENABLE_DIAMOND_SOLUTION
void VoIF_setDhaVoiceEq(int mode, int arg1, int arg2, int arg3, int arg4)
{
    switch(mode)
    {
        case 0:
            DiamodVoiceDhaParams[0] = arg1 & 0x7;   // VEQ or DHA mode
            DiamodVoiceDhaParams[1] = arg1 >> 3;        // for DHA mode it's selection of channel
            DiamodVoiceDhaParams[2] = arg2 & 0xffff;    // Hereafter, parameters of DHA
            DiamodVoiceDhaParams[3] = arg2 >> 16;
            DiamodVoiceDhaParams[4] = arg3 & 0xffff;
            DiamodVoiceDhaParams[5] = arg3 >> 16;
            DiamodVoiceDhaParams[6] = arg4 & 0xffff;
            DiamodVoiceDhaParams[7] = arg4 >> 16;
            DiamondVoiceDhaDataChk |= 0x1;
            break;
        case 1:
            DiamodVoiceDhaParams[8] = arg1 & 0xffff;
            DiamodVoiceDhaParams[9] = arg1 >> 16;
            DiamodVoiceDhaParams[10] = arg2 & 0xffff;
            DiamodVoiceDhaParams[11] = arg2 >> 16;
            DiamodVoiceDhaParams[12] = arg3 & 0xffff;
            DiamodVoiceDhaParams[13] = arg3 >> 16;
            DiamondVoiceDhaDataChk |= 0x10;
            break;
        default:
            printk("\n===============VoIF_setDhaVoiceEq()  wrong mode =============== \n");
            break;
    }

    printk("\n===============VoIF_setDhaVoiceEq()  DiamondVoiceDhaDataChk = 0x%x=============== \n", DiamondVoiceDhaDataChk);

    if(DiamondVoiceDhaDataChk == 0x11)
    {
        DiamondVoiceDhaDataChk = 0;
        if(sgCallbackData.isInit)
        {
            printk("\n===============VoIF_setDhaVoiceEq()  Configuration =============== \n");
            /* if it's not RCV mode VEQ and DHA should be off */
            if(sgCallbackData.callMode != AUDIO_MODE_HANDSET)
                DiamodVoiceDhaParams[0] = 0;
                
            if(voifCallType == VOIF_VOICE_CALL_WB || voifCallType == VOIF_VT_CALL_WB)
            {
                sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(mode, curVolIndex, sgCallbackData.pDldata_wb, sgCallbackData.pUldata_wb, sgCallbackData.pDlOutput_wb_buf, DiamodVoiceDhaParams, VOIF_WIDE_BAND);
            }
            else
            {
                sgCallbackData.DiamondVoice_Mode = sgCallbackData.pDiamondVoice_Config(mode, curVolIndex, sgCallbackData.pDldata_nb, sgCallbackData.pUldata_nb, sgCallbackData.pDlOutput_nb_buf, DiamodVoiceDhaParams, VOIF_NARROW_BAND);
            }
        }
    }
}
#endif

#endif /* CONFIG_ENABLE_VOIF */

#ifndef CONFIG_ENABLE_VOIF
void VoIF_SetDelay(int delay)
{
	voifDelay = delay;
}

void VoIF_SetGain(int gain)
{
	voifGain = gain;
}
#endif
