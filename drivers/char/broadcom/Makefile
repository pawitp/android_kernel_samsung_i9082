
EXTRA_CFLAGS := -fno-inline -Wno-multichar -O2

obj-y += v3d/
obj-y += vce/
obj-y += fuse_log/
obj-$(CONFIG_BCM_MODEM) += modem/
obj-y += isp/
obj-y += unicam/
obj-$(CONFIG_BCM_FRAME_PROFILER_SUPPORT)   += frame_profiler.o
obj-$(CONFIG_BCM_IDLE_PROFILER)            += idle_profiler/
obj-$(CONFIG_BCM_PERFTEST_SUPPORT)         += perftest/
obj-$(CONFIG_GPIO_USER_MODE_SUPPORT)       += user-gpio.o
obj-$(CONFIG_BCM_HALAUDIO_CORE)            += halaudio/
obj-$(CONFIG_BCM_HALAUDIO)                 += halaudio/ halaudio_drivers/
obj-$(CONFIG_BCM_AMXR)                     += amxr_drv.o
obj-$(CONFIG_BCM_AMXR_CORE)                += amxr_core/
obj-$(CONFIG_BCM_HEADSET_SW)               += headset.o
obj-$(CONFIG_BCM_GIST)                     += gist.o
#obj-$(CONFIG_BCM_GIST)                    += gist_test.o
obj-$(CONFIG_BCM_HDMI_DET)                 += hdmi/

obj-$(CONFIG_BCM_SIGMORPH)                 += sigmorph.o
#obj-$(CONFIG_BCM_SIGMORPH)                += sigmorph-test.o
obj-$(CONFIG_BCM_HDMI_DET)		+= hdmi/

bj-$(CONFIG_BCM_SIGMORPH)              += sigmorph.o
#obj-$(CONFIG_BCM_SIGMORPH)              += sigmorph-test.o

obj-$(CONFIG_BCM_CSX)                      += csx/
obj-$(CONFIG_BCM_RTC_CAL)           += rtc_cal/
obj-$(CONFIG_BCM_WFD)                      += wfd.o

obj-$(CONFIG_BCM_VC_HNFO)	+= vc_hnfo/
obj-$(CONFIG_BCM_VC_DNFO)	+= vc_dnfo/
obj-$(CONFIG_BCM_VC_OMX)	+= vc_omx/
obj-$(CONFIG_BCM_VC_LMK)	+= vc_lmk/
obj-$(CONFIG_BCM_VC_MEM)	+= vc_mem/
obj-$(CONFIG_BCM_VC_CMA)	+= vc_cma/
obj-$(CONFIG_BCM_VC_FB)   	+= vc_fb/
obj-$(CONFIG_BCM_VC_SM)   	+= vc_sm/
obj-$(CONFIG_BCM_VC_CAM)   	+= vc_cam/
obj-$(CONFIG_BCM_VC_SYSMAN_REMOTE) += sysman_remote/
obj-$(CONFIG_BCM_VC_PMU_REQUEST) += vc_pmu_request/
obj-$(CONFIG_BCM_VC_DISPLAY) += vc_display/
obj-$(CONFIG_BCM_VC_WATCHDOG) += vc_watchdog/
obj-$(CONFIG_BCM_FLIGHTMODE_OPT) += flightmode_opt/
obj-$(CONFIG_BCM_GPS_CNTIN) += gps/
