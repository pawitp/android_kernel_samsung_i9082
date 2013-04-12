# Makefile for the Linux Dpram drivers.

# Each configuration option enables a list of files.

MOD_NAME = sec_param


EXTRA_CFLAGS    += -Idrivers/param/inc


# module
obj-$(CONFIG_SAMSUNG_PARAM)			+= $(MOD_NAME).o


$(MOD_NAME)-y := param.o