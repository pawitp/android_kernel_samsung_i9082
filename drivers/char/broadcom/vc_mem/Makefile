EXTRA_CFLAGS  += -Wall -Wstrict-prototypes -Wno-trigraphs
EXTRA_CFLAGS  += -Werror
EXTRA_CFLAGS  += -I"include/linux/broadcom"

obj-$(CONFIG_BCM_VC_MEM) += vc-mem.o
obj-$(CONFIG_BCM_VC_RESERVE_LOWMEM) += vc-reserve.o

vc-mem-objs := vc_mem.o
vc-reserve-objs := vc_reserve.o

