#/*******************************************************************************************
#Copyright 2010 Broadcom Corporation.  All rights reserved.                                */
#
#Unless you and Broadcom execute a separate written software license agreement
#governing use of this software, this software is licensed to you under the
#terms of the GNU General Public License version 2, available at
#http://www.gnu.org/copyleft/gpl.html (the "GPL").
#
#Notwithstanding the above, under no circumstances may you combine this software
#in any way with any other Broadcom software provided under a license other than
#the GPL, without Broadcom's express prior written consent
#*******************************************************************************************/

# Makefile for the audio caph driver
# use $(srctree) for kernel top (see below) and
# use $(src) in a relative path which points to the directory

#enable it once all the commits are chery-picked from Rhea and clean up done
#KBUILD_CFLAGS += -Werror

#audio driver Source Files
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= at_aud_ctl.o
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= bcm_auddrv_test.o
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= audio_ddriver.o
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= audio_vdriver.o
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= audio_rpc_ap.o
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= audio_vdriver_audlog.o
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= voif_handler.o
obj-$(CONFIG_BCM_ALSA_SOUND) 	+= audio_mqueue.o

#EXTRA_CFLAGS += -DEXPORT_SYMTAB
#EXTRA_CFLAGS += -static -nostdlib -fno-pic -fno-stack-protector 
#EXTRA_CFLAGS += -I../sound/caph/audio_driver

#LVVE_Solution-objs += Engine.o
#LVVE_Solution-objs += REL_LVVE_HPF_NF_EQ_VOL_HF_FENS_VC_WM_WB_DRC_CNG_NG_AND_ARMA8_GCC_T4D4D0_demo.a

#obj-m := LVVE_Solution.o

#EXTRA_CFLAGS += -DEXPORT_SYMTAB
#EXTRA_CFLAGS += -static -nostdlib -fno-pic 
#VoiceSolution-objs += voicesolution.o
#VoiceSolution-objs += BCM_DiamondVoice_LIB_20130318.a

#obj-m := VoiceSolution.o

