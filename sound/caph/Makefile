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
#the GPL, without Broadcom's express prior written consent.
#*******************************************************************************************/

# Makefile for the audio caph driver
# use $(srctree) for kernel top (see below) and
# use $(src) in a relative path which points to the directory

subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -DMSP -DFUSE_DUAL_PROCESSOR_ARCHITECTURE -DDEVELOPMENT_ONLY -D_RHEA_
#the options below are used by capi headers
#subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -DUNDER_LINUX -DUNDER_LINUX_MODEM -DLINUX_RPC_KERNEL -DCONFIG_ENABLE_VOIF -DENABLE_DIAMOND_SOLUTION -DVOICESOLUTION_DUMP
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -DUNDER_LINUX -DUNDER_LINUX_MODEM -DLINUX_RPC_KERNEL
#the options below are used by audio
ifdef CONFIG_BCM_MODEM
#this feature is required only for voice features which has dependency on CP and DSP. Enabled them only when MODEM is enabled
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -DENABLE_DMA_VOICE
endif
ifdef CONFIG_ARCH_CAPRI
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -DENABLE_BT16
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -DCAPH_48K_MONO_PASSTHRU
#subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -DDYNAMIC_DMA_PLAYBACK
endif

#Linux header files
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./include/linux/
# For pmu audio driver
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./include/linux/broadcom
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/misc/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/modem/public/soc/debug/public
#Audio Controller header files
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./sound/caph -I./sound/caph/include
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./sound/caph/audio_controller
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./sound/caph/audio_driver/

#CSL header files
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./sound/caph/csl/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./sound/caph/dsp/
#CHAL header files
ifdef CONFIG_ARCH_RHEA_B0
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I$(src)/chal/B0/inc/
else
ifdef CONFIG_ARCH_CAPRI
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I$(src)/chal/capri/inc/
else
ifdef CONFIG_ARCH_RHEA
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I$(src)/chal/A0/inc/
endif
endif
endif
#RPC Header files
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/modem/rpc/rpc_CIB/public/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/modem/CAPI2_CIB/soc/debug/public/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/modem/CAPI2_CIB/public/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/fuse_log
#subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/modem/public/

#Platform header files
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/plat-kona/include/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/plat-kona/include/plat/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/plat-kona/include/mach
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/mach-rhea/include/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/mach-rhea/include/mach/
ifdef CONFIG_ARCH_RHEA_B0
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/mach-rhea/include/mach/rdb_Bx/
else
ifdef CONFIG_ARCH_CAPRI
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/mach-capri/include/mach/rdb/
else
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/mach-rhea/include/mach/rdb_A0/
endif
endif
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/plat-kona/include/plat/chal/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/plat-kona/include/plat/csl/
#The below sequence should not be changed on CAPRI to build succesfully
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./arch/arm/plat-kona/include/plat/osabstract/
subdir-ccflags-$(CONFIG_BCM_ALSA_SOUND) += -I./drivers/char/broadcom/modem/public/

obj-$(CONFIG_BCM_ALSA_SOUND) += caph_module.o
obj-$(CONFIG_BCM_ALSA_SOUND) += caph_ctl.o
obj-$(CONFIG_BCM_ALSA_SOUND) += caph_pcm.o
obj-$(CONFIG_BCM_ALSA_SOUND) += caph_hwdep.o
obj-$(CONFIG_BCM_ALSA_SOUND) += caph_ptt_hwdep.o
obj-$(CONFIG_BCM_ALSA_SOUND) += audio_controller/ audio_driver/
ifdef CONFIG_ARCH_RHEA_B0
obj-$(CONFIG_BCM_ALSA_SOUND) += chal/B0/src/
else
ifdef CONFIG_ARCH_CAPRI
obj-$(CONFIG_BCM_ALSA_SOUND) += chal/capri/src/
else
ifdef CONFIG_ARCH_RHEA
obj-$(CONFIG_BCM_ALSA_SOUND) += chal/A0/src/
endif
endif
endif
obj-$(CONFIG_BCM_ALSA_SOUND) += csl/
obj-$(CONFIG_BCM_MODEM)    += dsp/
obj-$(CONFIG_MAP_VOIP)		+= audio_pipe.o



