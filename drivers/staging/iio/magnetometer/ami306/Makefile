#
# Makefile for Invensense Aichi AMI306 compass IIO driver (inv-ami306-iio).
#

obj-$(CONFIG_INV_AMI306_IIO) += inv-ami306-iio.o

inv-ami306-iio-objs := inv_ami306_core.o
inv-ami306-iio-objs += inv_ami306_ring.o
inv-ami306-iio-objs += inv_ami306_trigger.o

CFLAGS_inv_ami306_core.o    += -Idrivers/staging/iio
CFLAGS_inv_ami306_ring.o    += -Idrivers/staging/iio
CFLAGS_inv_ami306_trigger.o += -Idrivers/staging/iio

