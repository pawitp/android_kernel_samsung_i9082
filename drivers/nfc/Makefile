#
# Makefile for nfc devices
#

obj-$(CONFIG_PN544_NFC)		+= pn544.o
obj-$(CONFIG_BCM2079X_NFC_I2C)	+= bcm2079x-i2c.o
obj-$(CONFIG_BCM2079X_NFC_SPI)	+= bcm2079x-spi.o
ccflags-$(CONFIG_NFC_DEBUG) := -DDEBUG
