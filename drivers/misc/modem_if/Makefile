# Makefile of modem_if

EXTRA_CFLAGS += -Idrivers/misc/modem_if_v2

obj-y += sipc4_modem.o sipc4_io_device.o
# obj-y += sipc5_modem.o sipc5_io_device.o
obj-y += modem_net_flowcontrol_device.o modem_utils.o

obj-$(CONFIG_TDSCDMA_MODEM_SPRD8803) += modem_modemctl_device_sprd8803.o

obj-$(CONFIG_LINK_DEVICE_SPI) += modem_link_device_spi.o
