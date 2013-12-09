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
*  @file  voif_handler.h
*
*  @brief Template voif handler API. Customer can add new API's and functions here.
*
*  @note
*****************************************************************************/
/**
*
* @defgroup Audio    Audio Component
*
* @brief    Updated by customer
*
* @ingroup  Audio Component
*****************************************************************************/

#ifndef	__AUDIO_VOIF_HANDLER_H__
#define	__AUDIO_VOIF_HANDLER_H__

/**
*
* @addtogroup Audio
* @{
*/

enum _VoIF_CallType_t {
	VOIF_NO_CALL = 0x00,
	VOIF_VOICE_CALL_NB = 0x10,
	VOIF_VOICE_CALL_WB = 0x20,
	VOIF_VT_CALL_NB = 0x30,
	VOIF_VT_CALL_WB = 0x40
};

#define VoIF_CallType_t enum _VoIF_CallType_t

enum _VoIF_HeadsetType_t {
	VOIF_OTHER_TYPE,
	VOIF_HEADPHONE_TYPE, // 3-pole
	VOIF_HEADSET_TYPE, // 4-pole
	VOIF_TOTAL_TYPE
};
#define VoIF_HeadsetType_t enum _VoIF_HeadsetType_t

enum _VoIF_Band_t {
	VOIF_NARROW_BAND = 0,
	VOIF_WIDE_BAND,
	VOIF_BAND_MAX
};

#define VoIF_Band_t enum _VoIF_Band_t

/**
* Start the VOIF processing.
*
*	@param	mode: audio mode (equivalently to channel)
*	@param	hstype: headset type (headset type only)
*	@return	void
*	@note
**************************************************************************/
	void VoIF_init(AudioMode_t mode, VoIF_HeadsetType_t hstype, VoIF_CallType_t callType);

/**
* Stop the VOIF processing.
*
*	@param	none
*	@return	void
*	@note
**************************************************************************/
	void VoIF_Deinit(void);

#ifdef CONFIG_ENABLE_VOIF
/**
* Update audio mode to VOIF handler.
*
*	@param	mode: audio mode (equivalently to channel)
*	@param	hstype: headset type (headset type only)
*	@return	void
*	@note
**************************************************************************/
	void VoIF_modeChange(AudioMode_t mode, VoIF_HeadsetType_t hstype);

/**
* Update voice volume index to VOIF handler.
*
*	@param	vol_index: volume index
*	@return	void
*	@note
**************************************************************************/
	void VoIF_volumeSetting(int vol_index);

/**
* Update call type to VOIF handler.
*
*	@param	callType: call type (VT / Voice)
*	@return	void
*	@note
**************************************************************************/
	void VoIF_setCallType(VoIF_CallType_t callType);

/**
* Return call type to VOIF handler.
*
*	@param	void
*	@return	call type (VT / Voice)
*	@note
**************************************************************************/
	VoIF_CallType_t VoIF_getCallType(void);

/**
* Update loopback mode to VOIF handler.
*
*	@param	whether loopback mode or not (0 / 1)
*	@return	void
*	@note
**************************************************************************/
	void VoIF_setLoopbackMode(short isLoopback);


#ifdef ENABLE_DIAMOND_SOLUTION
/**
* Update DHA Voice EQ parameter to VOIF handler.
*
*	@param	mode : audio path
*	@param	arg1~arg4 : parameter
*	@return	void
*	@note
**************************************************************************/
	void VoIF_setDhaVoiceEq(int mode, int arg1, int arg2, int arg3, int arg4);
#endif
	
#endif /* CONFIG_ENABLE_VOIF */

#ifndef CONFIG_ENABLE_VOIF
	void VoIF_SetDelay(int delay);	/* For test purpose only */
	void VoIF_SetGain(int gain);	/* For test purpose only */
#endif

#endif	/* __AUDIO_VOIF_HANDLER_H__ */
