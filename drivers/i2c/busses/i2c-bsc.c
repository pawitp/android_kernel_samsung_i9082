/*****************************************************************************
* Copyright 2010 - 2011 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/

#include <linux/device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/i2c-kona.h>
#include <linux/slab.h>
#include <linux/err.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#endif /*CONFIG_DEBUG_FS*/
#if defined(CONFIG_CAPRI_BSC3_PAD_CTRL_MODE)|| defined(CONFIG_CAPRI_BSC1_PAD_CTRL_MODE)
#include <mach/chip_pinmux.h>
#include <mach/pinmux.h>
#endif
#include <mach/chipregHw_inline.h>

#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
#include <plat/pwr_mgr.h>
#endif

// #include <linux/broadcom/timer.h>

#include <linux/timer.h>
#include "i2c-bsc.h"

#define DEFAULT_I2C_BUS_SPEED    BSC_BUS_SPEED_50K
#define CMDBUSY_DELAY            100
#define SES_TIMEOUT              (msecs_to_jiffies(100))

/* maximum RX/TX FIFO size in bytes */
#define MAX_RX_FIFO_SIZE         64
#define MAX_TX_FIFO_SIZE         64

/* upper 5 bits of the master code */
#define MASTERCODE               0x08
#define MASTERCODE_MASK          0x07

#define BSC_DBG(dev, format, args...) \
	do { if (dev->debug) dev_err(dev->device, format, ## args); } while (0)

#define MAX_PROC_BUF_SIZE         256
#define MAX_PROC_NAME_SIZE        15
#define PROC_GLOBAL_PARENT_DIR    "i2c"
#define PROC_ENTRY_DEBUG          "debug"
#define PROC_ENTRY_RESET          "reset"
#define PROC_ENTRY_TX_FIFO        "txFIFO"
#define PROC_ENTRY_RX_FIFO        "rxFIFO"

#define MAX_RETRY_NUMBER        (3-1)

#ifdef CONFIG_DEBUG_FS
static struct dentry *bsc_dentry, *bsc_speed;
static int bsc_clk_open(struct inode *inode, struct file *file);
static ssize_t set_i2c_bus_speed(struct file *file, char const __user *buf,
			size_t size, loff_t *offset);
static const struct file_operations default_file_operations = {
	.open = bsc_clk_open,
	.read = seq_read,
	.write = set_i2c_bus_speed,
};
#endif /*CONFIG_DEBUG_FS*/
/*
 * BSC (I2C) private data structure
 */
struct bsc_i2c_dev {
	struct device *device;

	/* iomapped base virtual address of the registers */
	void __iomem *virt_base;

	/* I2C bus speed */
	enum bsc_bus_speed speed;

	/* Current I2C bus speed configured */
	enum bsc_bus_speed current_speed;

	/* flag to support dynamic bus speed configuration for multiple
	 * slaves */
	int dynamic_speed;

	/* flag for TX/RX FIFO support */
	atomic_t rx_fifo_support;
	atomic_t tx_fifo_support;

	/* the 8-bit master code (0000 1XXX, 0x08) used for high speed mode */
	unsigned char mastercode;

	/* If the transfer is master code */
	bool is_mastercode;

	/* to save the old BSC TIM register value */
	volatile uint32_t tim_val;

	/* flag to indicate whether the I2C bus is in high speed mode */
	unsigned int high_speed_mode;

	/* IRQ line number */
	int irq;

	/* Linux I2C adapter struct */
	struct i2c_adapter adapter;

	/* lock for the I2C device */
	struct semaphore dev_lock;

	/* to signal the command completion */
	struct completion ses_done;

	/*
	 * to signal the BSC controller has finished reading and all RX data has
	 * been stored in the RX FIFO
	 */
	struct completion rx_ready;

	/*
	 * to signal the TX FIFO is empty,
	 * which means all pending TX data has been
	 * sent out and received by the slave
	 */
	struct completion tx_fifo_empty;

	volatile int debug;

	struct clk *bsc_clk;
	struct clk *bsc_apb_clk;

	/* workqueue work for reset the master */
	struct workqueue_struct *reset_wq;
	struct work_struct reset_work;

	/* Bit flag to get the interrupt bit set - Use this in the driver to
	 * check if there was any error when interrupted */
	unsigned int err_flag;
};

static const __devinitconst char gBanner[] =
	KERN_INFO "Broadcom BSC (I2C) Driver\n";

/*
 * Bus speed lookup table
 */
static const unsigned int gBusSpeedTable[BSC_SPD_MAXIMUM] = {
	BSC_SPD_32K,
	BSC_SPD_50K,
	BSC_SPD_100K,
	BSC_SPD_230K,
	BSC_SPD_380K,
	BSC_SPD_400K,
	BSC_SPD_430K,
	BSC_SPD_HS,
	BSC_SPD_HS_1MHZ,
	BSC_SPD_HS_2MHZ,
	BSC_SPD_HS_1625KHZ,
	BSC_SPD_HS_2600KHZ,
	BSC_SPD_100K_FPGA,
	BSC_SPD_400K_FPGA,
	BSC_SPD_HS_FPGA,
};

#ifdef CONFIG_CAPRI_BSC3_PAD_CTRL_MODE
#ifdef CONFIG_MACH_CAPRI_SS_CRATER
static struct pin_config gpin_bsc3_scl_config[2] = {
	{
		.name = PN_VC_CAM2_SCL,
		.func = PF_GPIO_065,
		.reg.val = 0x304,
	},
	{
		.name = PN_VC_CAM2_SCL,
		.func = PF_BSC3_SCL,
		.reg.val = 0x140,
	},
};

static struct pin_config gpin_bsc3_sda_config[2] = {
	{
		.name = PN_VC_CAM2_SDA,
		.func = PF_GPIO_066,
		.reg.val = 0x304,
	},
	{
		.name = PN_VC_CAM2_SDA,
		.func = PF_BSC3_SDA,
		.reg.val = 0x140,
	},
};
#else
static struct pin_config gpin_bsc3_scl_config[2] = {
	{
		.name = PN_VC_CAM3_SCL,
		.func = PF_GPIO_067,
		.reg.val = 0x308,
	},
	{
		.name = PN_VC_CAM3_SCL,
		.func = PF_BSC3_SCL,
		.reg.val = 0x130,
	},
};

static struct pin_config gpin_bsc3_sda_config[2] = {
	{
		.name = PN_VC_CAM3_SDA,
		.func = PF_GPIO_068,
		.reg.val = 0x308,
	},
	{
		.name = PN_VC_CAM3_SDA,
		.func = PF_BSC3_SDA,
		.reg.val = 0x130,
	},
};
#endif
#endif

#ifdef CONFIG_CAPRI_BSC1_PAD_CTRL_MODE
static struct pin_config gpin_bsc1_scl_config[2] = {
	{
		.name = PN_BSC1_SCL,
		.func = PF_SPARE_07,
		.reg.val = 0x308,
	},
	{
		.name = PN_BSC1_SCL,
		.func = PF_BSC1_SCL,
		.reg.val = 0x018,
	},
};

static struct pin_config gpin_bsc1_sda_config[2] = {
	{
		.name = PN_BSC1_SDA,
		.func = PF_SPARE_08,
		.reg.val = 0x308,
	},	
	{
		.name = PN_BSC1_SDA,
		.func = PF_BSC1_SDA,
		.reg.val = 0x018,
	},
};
#endif

static int bsc_enable_clk(struct bsc_i2c_dev *dev);
static void bsc_disable_clk(struct bsc_i2c_dev *dev);

/*
 * BSC ISR routine
 */
static irqreturn_t bsc_isr(int irq, void *devid)
{
	struct bsc_i2c_dev *dev = (struct bsc_i2c_dev *)devid;
	uint32_t status;

	/* get interrupt status */
	status = bsc_read_intr_status((uint32_t)dev->virt_base);

	/* Use the error flag to determine the errors if any outside the
	 * interrupt context */
	dev->err_flag = status;

	/* got nothing, something is wrong */
	if (!status) {
		dev_err(dev->device, "interrupt with zero status register!\n");
		return IRQ_NONE;
	}

	/* ack and clear the interrupts */
	bsc_clear_intr_status((uint32_t)dev->virt_base, status);

	if (status & I2C_MM_HS_ISR_SES_DONE_MASK)
		complete(&dev->ses_done);

	if (status & I2C_MM_HS_ISR_NOACK_MASK) {
		// TODO: check for TX NACK status here
		// should not clear status until figure out what's going on

		/*
		 * For Mastercode, NAK is expected as per HS protocol, it's
		 * not error
		 */
		if (dev->high_speed_mode && dev->is_mastercode) {
			dev->is_mastercode = false;
			/* Since the NACK is expected, reset the error flag */
			/* We can do this safely since we no for sure that the
			 * bus error interrupt is not valid in high speed */
			dev->err_flag &= ~(dev->err_flag);
		}
		else
			dev_err(dev->device, "no ack detected\n");
	}

	if (status & I2C_MM_HS_ISR_TXFIFOEMPTY_MASK)
		complete(&dev->tx_fifo_empty);

	if (status & I2C_MM_HS_ISR_READ_COMPLETE_MASK)
		complete(&dev->rx_ready);

	/*
	 * I2C bus timeout, schedule a workqueue work item to reset the
	 * master
	 */
	if (status & I2C_MM_HS_ISR_ERR_MASK) {
		dev_err(dev->device,
			"bus error interrupt (timeout) - status = %x\n",
			status);

		/* disable interrupts since the master will now reset */
		bsc_disable_intr((uint32_t)dev->virt_base, 0xFF);
		queue_work(dev->reset_wq, &dev->reset_work);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_MFD_BCM_PWRMGR_SW_SEQUENCER
/*
 * PMU BSC ISR routine
 */
static irqreturn_t pmu_bsc_isr(int irq, void *devid)
{
	struct bsc_i2c_dev *dev = (struct bsc_i2c_dev *)devid;
	uint32_t status;

	/* get interrupt status */
	status = bsc_read_intr_status((uint32_t)dev->virt_base);

	/* got nothing, something is wrong */
	if (!status) {
		dev_err(dev->device, "interrupt with zero status register!\n");
		return IRQ_NONE;
	}

	/* ack and clear the interrupts */
	bsc_clear_intr_status((uint32_t)dev->virt_base, status);

	if (status & I2C_MM_HS_ISR_SES_DONE_MASK)
		complete(&dev->ses_done);

	if (status & I2C_MM_HS_ISR_NOACK_MASK) {
		// TODO: check for TX NACK status here
		// should not clear status until figure out what's going on

		/*
		 * For Mastercode, NAK is expected as per HS protocol, it's not error
		 */
		if (dev->high_speed_mode && dev->is_mastercode)
			dev->is_mastercode = false;
		else
			dev_err(dev->device, "no ack detected\n");
	}

	return IRQ_HANDLED;
}
#endif

/*
 * We should not need to do this in software, but this is how the hardware was
 * designed and that leaves us with no choice but SUCK it
 *
 * When the CPU writes to the control, data, or CRC registers the CMDBUSY bit
 * will be set to high. It will be cleared after the writing action has been
 * transferred from APB clock domain to BSC clock domain and then the status
 * has transfered from BSC clock domain back to APB clock domain
 *
 * We need to wait for the CMDBUSY to clear because the hardware does not have
 * CMD pipeline registers. This wait is to avoid a previous written CMD/data
 * to be overwritten by the following writing before the previous written
 * CMD/data was executed/synchronized by the hardware
 *
 * We shouldn't set up an interrupt for this since the context switch overhead
 * is too expensive for this type of action and in fact 99% of time we will
 * experience no wait anyway
 */
static int bsc_wait_cmdbusy(struct bsc_i2c_dev *dev)
{
	unsigned long time = 0, limit;

	/* wait for CMDBUSY is ready  */
	limit = (loops_per_jiffy * msecs_to_jiffies(CMDBUSY_DELAY));
	while ((bsc_read_intr_status((uint32_t)dev->virt_base) &
		I2C_MM_HS_ISR_CMDBUSY_MASK) && (time++ < limit))
		cpu_relax();

	if (time >= limit) {
		dev_err(dev->device, "CMDBUSY timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int bsc_send_cmd(struct bsc_i2c_dev *dev, BSC_CMD_t cmd)
{
	int rc;
	unsigned long time_left;

	/* make sure the hareware is ready */
	rc = bsc_wait_cmdbusy(dev);
	if (rc < 0)
		return rc;

	/* enable the session done (SES) interrupt */
	bsc_enable_intr((uint32_t)dev->virt_base,
			I2C_MM_HS_IER_I2C_INT_EN_MASK);

	/* mark as incomplete before sending the command */
	INIT_COMPLETION(dev->ses_done);

	/* send the command */
	isl_bsc_send_cmd((uint32_t)dev->virt_base, cmd);

	/*
	 * Block waiting for the transaction to finish. When it's finished we'll
	 * be signaled by the interrupt
	 */
	time_left = wait_for_completion_timeout(&dev->ses_done, SES_TIMEOUT);
	bsc_disable_intr((uint32_t)dev->virt_base,
			 I2C_MM_HS_IER_I2C_INT_EN_MASK);
	/* Check if there was a bus error seen */
	if (time_left == 0 || (dev->err_flag & I2C_MM_HS_ISR_ERR_MASK)) {
		dev_err(dev->device, "controller timed out\n");

		/* Reset the error flag */
		dev->err_flag &= ~(dev->err_flag);

		/* clear command */
		isl_bsc_send_cmd((uint32_t)dev->virt_base, BSC_CMD_NOACTION);

		return -ETIMEDOUT;
	}

	/* clear command */
	isl_bsc_send_cmd((uint32_t)dev->virt_base, BSC_CMD_NOACTION);

	return 0;
}

static int bsc_xfer_start(struct i2c_adapter *adapter)
{
	int rc;
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);

	/* now send the start command */
	rc = bsc_send_cmd(dev, BSC_CMD_START);
	if (rc < 0) {
		dev_err(dev->device, "failed to send the start command\n");
		return rc;
	}

	return 0;
}

static int bsc_xfer_repstart(struct i2c_adapter *adapter)
{
	int rc;
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);

	rc = bsc_send_cmd(dev, BSC_CMD_RESTART);
	if (rc < 0) {
		dev_err(dev->device, "failed to send the restart command\n");
		return rc;
	}

	return 0;
}

static int bsc_xfer_stop(struct i2c_adapter *adapter)
{
	int rc;
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);

	rc = bsc_send_cmd(dev, BSC_CMD_STOP);
	if (rc < 0) {
		dev_err(dev->device, "failed to send the stop command\n");
		return rc;
	}

	return 0;
}

static int bsc_xfer_read_byte(struct bsc_i2c_dev *dev, unsigned int no_ack,
			      uint8_t *data)
{
	int rc;
	BSC_CMD_t cmd;

	if (no_ack)
		cmd = BSC_CMD_READ_NAK;
	else
		cmd = BSC_CMD_READ_ACK;

	/* send the read command */
	rc = bsc_send_cmd(dev, cmd);
	if (rc < 0)
		return rc;

	/*
	 * Now read the data from the BSC DATA register. Since BSC does not have
	 * an RX FIFO, we can only read one byte at a time
	 */
	bsc_read_data((uint32_t)dev->virt_base, data, 1);

	return 0;
}

/*
 * Read byte-by-byte through the BSC data register
 */
static unsigned int bsc_xfer_read_data(struct bsc_i2c_dev *dev,
				       unsigned int nak, uint8_t *buf,
				       unsigned int len)
{
	int i, rc;
	uint8_t data;
	unsigned int bytes_read = 0;

	BSC_DBG(dev, "*** read start ***\n");
	for (i = 0; i < len; i++) {
		rc = bsc_xfer_read_byte(dev, (nak || (i == (len - 1))), &data);
		if (rc < 0) {
			dev_err(dev->device, "read error\n");
			break;
		}
		BSC_DBG(dev, "reading %2.2X\n", data);
		buf[i] = data;
		bytes_read++;
	}
	BSC_DBG(dev, "*** read end ***\n");
	return bytes_read;
}

static inline uint8_t bsc_read_from_rx_fifo(uint32_t baseAddr)
{
	return BSC_READ_REG(baseAddr + I2C_MM_HS_RXFIFORDOUT_OFFSET)
				& I2C_MM_HS_RXFIFORDOUT_RXFIFO_RDOUT_MASK;
}

static unsigned int bsc_xfer_read_fifo_single(struct bsc_i2c_dev *dev,
					      uint8_t *buf,
					      unsigned int last_byte_nak,
					      unsigned int len)
{
	int i;
	unsigned long time_left;

	/* enable the read complete interrupt */
	bsc_enable_intr((uint32_t)dev->virt_base,
			I2C_MM_HS_IER_READ_COMPLETE_INT_MASK);

	/* mark as incomplete before starting the RX FIFO */
	INIT_COMPLETION(dev->rx_ready);

	/* start the RX FIFO */
	bsc_start_rx_fifo((uint32_t)dev->virt_base, last_byte_nak, len);
	barrier();

	/*
	 * Block waiting for the transaction to finish. When it's finished
	 * we'll be signaled by the interrupt
	 */
	time_left = wait_for_completion_timeout(&dev->rx_ready, SES_TIMEOUT);
	bsc_disable_intr((uint32_t)dev->virt_base,
			 I2C_MM_HS_IER_READ_COMPLETE_INT_MASK);
	if (time_left == 0) {
		dev_err(dev->device, "RX FIFO time out\n");
		return 0;
	}

	BSC_DBG(dev, "*** read start ***\n");
	for (i = 0; i < len; i++) {
		buf[i] = bsc_read_from_rx_fifo((uint32_t)dev->virt_base);
		BSC_DBG(dev, "reading %2.2X\n", buf[i]);

		/* sanity check */
		if (bsc_rx_fifo_is_empty((uint32_t)dev->virt_base)
		    && i != len - 1)
			dev_err(dev->device, "RX FIFO error\n");
	}
	BSC_DBG(dev, "*** read end ***\n");

	return len;
}

static unsigned int bsc_xfer_read_fifo(struct bsc_i2c_dev *dev, uint8_t * buf,
				       unsigned int len)
{
	unsigned int i, rc, last_byte_nak = 0, bytes_read = 0;

	/* can only read MAX_RX_FIFO_SIZE each time */
	for (i = 0; i < len / MAX_RX_FIFO_SIZE; i++) {
		/* if this is the last FIFO read, need to NAK before stop */
		if (len % MAX_RX_FIFO_SIZE == 0 &&
		    i == (len / MAX_RX_FIFO_SIZE) - 1)
			last_byte_nak = 1;

		rc = bsc_xfer_read_fifo_single(dev, buf, last_byte_nak,
					       MAX_RX_FIFO_SIZE);
		bytes_read += rc;

		/* read less than expected, something is wrong */
		if (rc < MAX_RX_FIFO_SIZE)
			return bytes_read;

		buf += MAX_RX_FIFO_SIZE;
	}

	/* still have some bytes left */
	if (len % MAX_RX_FIFO_SIZE != 0) {
		rc = bsc_xfer_read_fifo_single(dev, buf, 1,
					       len % MAX_RX_FIFO_SIZE);
		bytes_read += rc;
	}

	return bytes_read;
}

static unsigned int bsc_xfer_read(struct i2c_adapter *adapter,
				  struct i2c_msg *msg)
{
	unsigned int nak, bytes_read = 0;
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);

	nak = (msg->flags & I2C_M_NO_RD_ACK) ? 1 : 0;

	/* FIFO mode cannot handle NAK for individual bytes */
	if (atomic_read(&dev->rx_fifo_support) && !nak)
		bytes_read = bsc_xfer_read_fifo(dev, msg->buf, msg->len);
	else
		bytes_read = bsc_xfer_read_data(dev, nak, msg->buf, msg->len);

	return bytes_read;
}

static int bsc_xfer_write_byte(struct bsc_i2c_dev *dev, unsigned int nak_ok,
			       uint8_t *data)
{
	int rc;
	unsigned long time_left;

	/* make sure the hareware is ready */
	rc = bsc_wait_cmdbusy(dev);
	if (rc < 0)
		return rc;

	/* enable the session done (SES) interrupt */
	bsc_enable_intr((uint32_t)dev->virt_base,
			I2C_MM_HS_IER_I2C_INT_EN_MASK);

	/* mark as incomplete before sending the data */
	INIT_COMPLETION(dev->ses_done);

	/* send data */
	bsc_write_data((uint32_t)dev->virt_base, data, 1);

	/*
	 * Block waiting for the transaction to finish. When it's finished we'll
	 * be signaled by the interrupt
	 */
	time_left = wait_for_completion_timeout(&dev->ses_done, SES_TIMEOUT);
	bsc_disable_intr((uint32_t)dev->virt_base,
			 I2C_MM_HS_IER_I2C_INT_EN_MASK);
	if (time_left == 0 || dev->err_flag) {
		/* Check if there was a NACK and it was un-expected */
		if ((dev->err_flag & I2C_MM_HS_ISR_NOACK_MASK) && nak_ok == 0) {
			dev_err(dev->device, "unexpected NAK\n");
			return -EREMOTEIO;
		/* Check if there was a bus error */
		} else if (dev->err_flag & I2C_MM_HS_ISR_ERR_MASK) {
			dev_err(dev->device, "controller timed out\n");
			return -ETIMEDOUT;
		}
		/* Reset the error flag */
		dev->err_flag &= ~(dev->err_flag);
	}

	return 0;
}

/*
 * Write data byte-by-byte into the BSC data register
 */
static int bsc_xfer_write_data(struct bsc_i2c_dev *dev, unsigned int nak_ok,
			       uint8_t *buf, unsigned int len)
{
	int i, rc;
	unsigned int bytes_written = 0;

	for (i = 0; i < len; i++) {
		rc = bsc_xfer_write_byte(dev, nak_ok, &buf[i]);
		if (rc < 0) {
			dev_err(dev->device,
				"problem experienced during data write\n");
			break;
		}
		BSC_DBG(dev, "writing %2.2X\n", *buf);
		bytes_written++;
	}
	return bytes_written;
}

static int bsc_xfer_write_fifo(struct bsc_i2c_dev *dev, unsigned int nak_ok,
			       uint8_t *buf, unsigned int len)
{
	int rc = 0;
	unsigned long time_left, i;
	uint8_t *tmp_buf;

	/* make sure the hareware is ready */
	rc = bsc_wait_cmdbusy(dev);
	if (rc < 0)
		return rc;

	/* enable TX FIFO */
	bsc_set_tx_fifo((uint32_t)dev->virt_base, 1);

	/* mark as incomplete before sending data to the TX FIFO */
	INIT_COMPLETION(dev->tx_fifo_empty);

	/* pointer to the source buffer */
	tmp_buf = buf;

	/* The DVT test code logic :
	 * Write 64 bytes to the FIFO and wait for the TX FIFO full interrupt */
	/* Keep writing 64 bytes of data to the FIFO */
	for (i = 0 ; i < len/MAX_TX_FIFO_SIZE ; i++) {
		/* Write 64 bytes of data */
		bsc_write_data((uint32_t)dev->virt_base, tmp_buf,
				MAX_TX_FIFO_SIZE);

		/* Enable, check and disable the fifo empty interrupt */
		bsc_enable_intr((uint32_t)dev->virt_base,
				I2C_MM_HS_IER_FIFO_INT_EN_MASK |
				I2C_MM_HS_IER_NOACK_EN_MASK);

		time_left = wait_for_completion_timeout(&dev->tx_fifo_empty,
							SES_TIMEOUT);

		bsc_disable_intr((uint32_t)dev->virt_base,
				I2C_MM_HS_IER_FIFO_INT_EN_MASK |
				I2C_MM_HS_IER_NOACK_EN_MASK);

		/* Check if the write timed-out or NACKed */
		if (time_left == 0 || dev->err_flag) {
			/* Check if there was a NACK and it was unexpected */
			if ((dev->err_flag & I2C_MM_HS_ISR_NOACK_MASK) &&
			    nak_ok == 0) {
				dev_err(dev->device, "unexpected NAK\n");
				rc = -EREMOTEIO;
				goto err_fifo;
			/* Check if there was a bus error */
			} else if (dev->err_flag & I2C_MM_HS_ISR_ERR_MASK) {
				dev_err(dev->device, "controller timed out\n");
				rc = -ETIMEDOUT;
				goto err_fifo;
			}
			/* Reset the error flag */
			dev->err_flag &= ~(dev->err_flag);
		}
		/* Reset the error flag */
		dev->err_flag &= ~(dev->err_flag);

		/* Update the buf to point to the next data */
		tmp_buf += MAX_TX_FIFO_SIZE;
	}

	/* Transfer the remainder bytes if the data size is not a multiple of
	 * 64 */
	if (len % MAX_TX_FIFO_SIZE != 0) {

		/* Write the remainder bytes */
		bsc_write_data((uint32_t)dev->virt_base, tmp_buf,
				(len % MAX_TX_FIFO_SIZE));

		/* Enable, check and disable the fifo empty interrupt */
		bsc_enable_intr((uint32_t)dev->virt_base,
				I2C_MM_HS_IER_FIFO_INT_EN_MASK |
				I2C_MM_HS_IER_NOACK_EN_MASK);

		time_left = wait_for_completion_timeout(&dev->tx_fifo_empty,
							SES_TIMEOUT);

		bsc_disable_intr((uint32_t)dev->virt_base,
				I2C_MM_HS_IER_FIFO_INT_EN_MASK |
				I2C_MM_HS_IER_NOACK_EN_MASK);

		/* Check if the write timed-out or NACKed */
		if (time_left == 0 || dev->err_flag) {
			/* Check if there was a NACK and it was unexpected */
			if ((dev->err_flag & I2C_MM_HS_ISR_NOACK_MASK) &&
			    nak_ok == 0) {
				dev_err(dev->device, "unexpected NAK\n");
				rc = -EREMOTEIO;
				goto err_fifo;
			/* Check if there was a bus error */
			} else if (dev->err_flag & I2C_MM_HS_ISR_ERR_MASK) {
				dev_err(dev->device, "controller timed out\n");
				rc = -ETIMEDOUT;
				goto err_fifo;
			}
			/* Reset the error flag */
			dev->err_flag &= ~(dev->err_flag);
		}
	}

	/* make sure writing to be finished before disabling TX FIFO */
	rc = bsc_wait_cmdbusy(dev);
	if (rc < 0)
		goto err_fifo;

	/* The Write was successful. Hence return the length of the data
	 * transferred */

	rc = len;

err_fifo:

	/* disable TX FIFO */
	bsc_set_tx_fifo((uint32_t)dev->virt_base, 0);

	return rc;
}

static int bsc_xfer_write(struct i2c_adapter *adapter, struct i2c_msg *msg)
{
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);
	unsigned int bytes_written = 0;
	unsigned int nak_ok = msg->flags & I2C_M_IGNORE_NAK;

	if (atomic_read(&dev->tx_fifo_support)) {
		bytes_written = bsc_xfer_write_fifo(dev, nak_ok, msg->buf,
						    msg->len);
	} else {
		bytes_written = bsc_xfer_write_data(dev, nak_ok, msg->buf,
						    msg->len);
	}

	return bytes_written;
}

static int bsc_xfer_try_address(struct i2c_adapter *adapter,
				unsigned char addr, unsigned short nak_ok,
				unsigned int retries)
{
	unsigned int i;
	int rc = 0, success = 0;
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);
	struct bsc_adap_cfg *hw_cfg = NULL;

	BSC_DBG(dev, "0x%02x, %d\n", addr, retries);
	hw_cfg = (struct bsc_adap_cfg *)dev->device->platform_data;

	for (i = 0; i <= retries; i++) {
		BSC_DBG(dev, "Retry #%d\n", i);
		rc = bsc_xfer_write_byte(dev, nak_ok, &addr);
		if (rc >= 0) {
			success = 1;
			break;
		}

		/* Send REPSTART in case of PMU, else STOP + START sequence */
		if (dev->high_speed_mode && hw_cfg && hw_cfg->is_pmu_i2c) {
			rc = bsc_xfer_repstart(adapter);
			if (rc < 0)
				break;
		} else {
			/* no luck, let's keep trying */
			rc = bsc_xfer_stop(adapter);
			if (rc < 0 || i >= retries)
				break;

			rc = bsc_xfer_start(adapter);
			if (rc < 0)
				break;
		}
	}

	/* unable to find a slave */
	if (!success) {
		dev_err(dev->device,
			"tried %u times to contact slave device at 0x%02x "
			"but no luck success=%d rc=%d\n", i + 1, addr >> 1,
			success, rc);

		rc = -EREMOTEIO;
	}

	return rc;
}

static int bsc_xfer_do_addr(struct i2c_adapter *adapter, struct i2c_msg *msg)
{
	int rc;
	unsigned int retries;
	unsigned short flags = msg->flags;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
	unsigned char addr;
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);

	retries = nak_ok ? 0 : adapter->retries;

	/* ten bit address */
	if (flags & I2C_M_TEN) {
		/*
		 * First byte is 11110XX0,
		 * where XX is the upper 2 bits of the 10 bits
		 * */
		addr = 0xF0 | ((msg->addr & 0x300) >> 7);
		rc = bsc_xfer_try_address(adapter, addr, nak_ok, retries);
		if (rc < 0)
			return -EREMOTEIO;

		/* then the remaining 8 bits */
		addr = msg->addr & 0xFF;
		rc = bsc_xfer_write_byte(dev, nak_ok, &addr);
		if (rc < 0)
			return -EREMOTEIO;

		/* for read */
		if (flags & I2C_M_RD) {
			rc = bsc_xfer_repstart(adapter);
			if (rc < 0)
				return -EREMOTEIO;

			/* now re-send the first 7 bits with the read bit */
			addr = 0xF0 | ((msg->addr & 0x300) >> 7);
			addr |= 0x01;
			rc = bsc_xfer_try_address(adapter, addr, nak_ok,
						  retries);
			if (rc < 0)
				return -EREMOTEIO;
		}
	} else {		/* normal 7-bit address */

		addr = msg->addr << 1;
		if (flags & I2C_M_RD)
			addr |= 1;
		if (flags & I2C_M_REV_DIR_ADDR)
			addr ^= 1;
		rc = bsc_xfer_try_address(adapter, addr, nak_ok, retries);
		if (rc < 0)
			return -EREMOTEIO;
	}

	return 0;
}

static int __bsc_i2c_get_client(struct device *dev, void *addrp)
{
	struct i2c_client *client = i2c_verify_client(dev);
	int addr = *(int *)addrp;

	if (client && client->addr == addr)
		return true;

	return 0;
}

static struct device *bsc_i2c_get_client(struct i2c_adapter *adapter, int addr)
{
	return device_find_child(&adapter->dev, &addr, __bsc_i2c_get_client);
}

static int start_high_speed_mode(struct i2c_adapter *adapter)
{
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);
	int rc = 0;
	struct bsc_adap_cfg *hw_cfg = NULL;

	/*
	 * mastercode (0000 1000 + #id)
	 */
	dev->mastercode = (MASTERCODE | (MASTERCODE_MASK & adapter->nr)) + 1;
	dev->is_mastercode = true;

	hw_cfg = (struct bsc_adap_cfg *)dev->device->platform_data;

	/* send the master code in F/S mode first */
	rc = bsc_xfer_write_byte(dev, 1, &dev->mastercode);
	if (rc < 0) {
		dev_err(dev->device, "high-speed master code failed\n");
		dev->is_mastercode = false;
		return rc;
	}

	/* check to make sure no slave replied to the master code by accident */
	if (bsc_get_ack((uint32_t)dev->virt_base)) {
		dev_err(dev->device,
			"one of the slaves replied to the high-speed "
			"master code unexpectedly\n");
		dev->is_mastercode = false;
		return -EREMOTEIO;
	}

	/*
	 * Now save the BSC_TIM register value as it will be modified before the
	 * master going into high-speed mode. We need to restore the BSC_TIM
	 * value when the device switches back to fast speed
	 */
	dev->tim_val = bsc_get_tim((uint32_t)dev->virt_base);

	/* configure the bsc clock to 26MHz for HS mode */
	if (dev->bsc_clk) {
		clk_disable(dev->bsc_clk);
		/* Use 26MHz source for all HS clients */
		clk_set_rate(dev->bsc_clk, 26000000);
		clk_enable(dev->bsc_clk);
		dev_info(dev->device, "HS mode clock rate is set to %ld\n",
			clk_get_rate(dev->bsc_clk));
	}

	/* Turn-off autosense and Tout interrupt for HS mode */
	bsc_disable_intr((uint32_t)dev->virt_base,
			 I2C_MM_HS_IER_ERR_INT_EN_MASK);
	bsc_set_autosense((uint32_t)dev->virt_base, 0, 0);

	/* configure the bus into high-speed mode */
	bsc_start_highspeed((uint32_t)dev->virt_base);
	dev_err(dev->device, "Adapter is switched to HS mode\n");

	return 0;
}

static void stop_high_speed_mode(struct i2c_adapter *adapter)
{
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);

	/* Restore TIM register value */
	bsc_set_tim((uint32_t)dev->virt_base, dev->tim_val);


	/* stop hs clock and switch back to F/S clock source */
	if (dev->bsc_clk) {
		clk_disable(dev->bsc_clk);
		clk_set_rate(dev->bsc_clk, 13000000);
		clk_enable(dev->bsc_clk);
	}
	bsc_stop_highspeed((uint32_t)dev->virt_base);
}

/*
 * Set bus speed to what the client wants
 */
static void client_speed_set(struct i2c_adapter *adapter, unsigned short addr)
{
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);
	struct device *d;
	struct i2c_client *client = NULL;
	struct i2c_slave_platform_data *pd = NULL;
	enum bsc_bus_speed set_speed;
	struct bsc_adap_cfg *hw_cfg = NULL;

	/* Get slave speed configuration */
	d = bsc_i2c_get_client(adapter, addr);
	if (d) {

		client = i2c_verify_client(d);
		pd = (struct i2c_slave_platform_data *)client->dev.
		    platform_data;
		if (pd) {
			BSC_DBG(dev, "client addr=0x%x, speed=0x%x\n",
				client->addr, pd->i2c_speed);

			if (i2c_speed_is_valid(pd)
			    && (pd->i2c_speed < BSC_BUS_SPEED_MAX)) {
				set_speed = pd->i2c_speed;
				BSC_DBG(dev,
					"i2c addr=0x%x dynamic slave speed:%d\n",
					client->addr, set_speed);
			} else {
				set_speed = dev->speed;
				BSC_DBG(dev,
					"i2c addr=0x%x using default speed:%d\n",
					client->addr, set_speed);
			}
		} else {
			BSC_DBG(dev,
				"client addr=0x%x no platform data found!\n",
				client->addr);
			set_speed = dev->speed;
		}
	} else {
		BSC_DBG(dev, "no client found with addr=0x%x\n", addr);
		set_speed = dev->speed;
	}

	/* check for high speed */
	if (set_speed >= BSC_BUS_SPEED_HS && set_speed <= BSC_BUS_SPEED_HS_FPGA)
		dev->high_speed_mode = 1;
	else
		dev->high_speed_mode = 0;

	hw_cfg = (struct bsc_adap_cfg *)dev->device->platform_data;

	/* configure the adapter bus speed */
	if (set_speed != dev->current_speed && set_speed < BSC_BUS_SPEED_MAX) {
		/* PMU I2C HSTIM is calculated based on 26MHz source */
		if (hw_cfg->is_pmu_i2c)
			bsc_set_bus_speed((uint32_t)dev->virt_base,
					  gBusSpeedTable[set_speed], true);
		else
			bsc_set_bus_speed((uint32_t)dev->virt_base,
					  gBusSpeedTable[set_speed], false);
		dev->current_speed = set_speed;
	}

	/* high-speed mode */
	if (dev->high_speed_mode) {
		/* Disable Timeout interrupts for HS mode */
		bsc_disable_intr((uint32_t)dev->virt_base,
				 I2C_MM_HS_IER_ERR_INT_EN_MASK);

		/*
		 * Auto-sense allows the slave device to stretch the clock for
		 * a long time. Need to turn off auto-sense for high-speed
		 * mode
		 */
		bsc_set_autosense((uint32_t)dev->virt_base, 0, 0);
	} else {
		/* Enable Timeout interrupts for F/S mode */
		bsc_enable_intr((uint32_t)dev->virt_base,
				I2C_MM_HS_IER_ERR_INT_EN_MASK);

		/* In case of the Keypad controller LM8325, the maximum timeout
		 * set by the BSC controller does not suffice the time for which
		 * it holds the clk line low when busy resulting in bus errors.
		 * To overcome this problem we need to enable autosense with
		 * the timeout disabled
		 */
		if (pd && disable_timeout(pd))
			bsc_set_autosense((uint32_t)dev->virt_base, 1, 0);
		else {
			if (hw_cfg && hw_cfg->autosense_tout_disa)
				bsc_set_autosense(
					(uint32_t)dev->virt_base, 1, 0);
			else
				bsc_set_autosense(
					(uint32_t)dev->virt_base, 1, 1);
		}
	}
}

/*
 * Configure the fifo as requested by the client
 */
static void client_fifo_configure(struct i2c_adapter *adapter,
					unsigned short addr, bool enable)
{
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);
	struct device *d;
	struct i2c_client *client = NULL;
	struct i2c_slave_platform_data *pd = NULL;

	/* Get slave fifo configuration */
	d = bsc_i2c_get_client(adapter, addr);
	if (d) {

		client = i2c_verify_client(d);
		pd = (struct i2c_slave_platform_data *)client->
		    dev.platform_data;
		if (pd) {
			if (enable) {
				/* For the RX FIFO */
				if (enable_rx_fifo(pd)) {
					atomic_set(&dev->rx_fifo_support, 1);
					dev_dbg(dev->device,
						"Enabled the RX FIFO\n");
				}

				/* For the TX FIFO */
				if (enable_tx_fifo(pd)) {
					atomic_set(&dev->tx_fifo_support, 1);
					dev_dbg(dev->device,
						"Enabled the TX FIFO\n");
				}
			} else {
				/* For the RX FIFO */
				if (enable_rx_fifo(pd)) {
					atomic_set(&dev->rx_fifo_support, 0);
					dev_dbg(dev->device,
						"Disabled the RX FIFO\n");
				}

				/* For the TX FIFO */
				if (enable_tx_fifo(pd)) {
					atomic_set(&dev->tx_fifo_support, 0);
					dev_dbg(dev->device,
						"Disabled the TX FIFO\n");
				}

			}
		} else {
			dev_dbg(dev->device,
				"No platform data found for client addr 0x%x\n",
				addr);
		}
	} else {
		dev_dbg(dev->device, "no client found with addr 0x%x\n", addr);
	}
}

/*
 * Master transfer function
 */
static int bsc_xfer(struct i2c_adapter *adapter, struct i2c_msg msgs[], int num)
{
	struct bsc_i2c_dev *dev = i2c_get_adapdata(adapter);
	struct i2c_msg *pmsg;
	int rc = 0;
	unsigned short i, nak_ok;
	struct bsc_adap_cfg *hw_cfg = NULL;
#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
	bool rel_hw_sem = false;
#endif

	down(&dev->dev_lock);
#ifdef CONFIG_ARCH_CAPRI
	pause_nohz();
#endif
	bsc_enable_clk(dev);
	bsc_enable_pad_output((uint32_t)dev->virt_base, true);
	hw_cfg = (struct bsc_adap_cfg *)dev->device->platform_data;

#ifdef CONFIG_CAPRI_BSC3_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C2) {

	/* Pull up SDA first so it wont generate STOP condition */
		pinmux_set_pin_config(&gpin_bsc3_sda_config[1]);
		pinmux_set_pin_config(&gpin_bsc3_scl_config[1]);
	}
#endif

#ifdef CONFIG_CAPRI_BSC1_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C0) {

	/* Pull up SDA first so it wont generate STOP condition */
		pinmux_set_pin_config(&gpin_bsc1_sda_config[1]);
		pinmux_set_pin_config(&gpin_bsc1_scl_config[1]);
	}
#endif

#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
	if (hw_cfg && hw_cfg->is_pmu_i2c) {
		rc = pwr_mgr_pm_i2c_sem_lock();
		if (rc) {
			bsc_enable_pad_output((uint32_t)dev->virt_base, false);
			bsc_disable_clk(dev);
#ifdef CONFIG_ARCH_CAPRI
			resume_nohz();
#endif
			up(&dev->dev_lock);
			return rc;
		} else {
			rel_hw_sem = true;
		}
	}
#endif
	/* Enable the fifos based on the client requirement */
	client_fifo_configure(adapter, msgs[0].addr, true);

	/*
	 * Set the bus speed dynamically
	 * if dynamic speed support is turned on
	 */
	if (dev->dynamic_speed)
		client_speed_set(adapter, msgs[0].addr);

	/* send start command, if its not in HS mode */
	if (!(dev->high_speed_mode && hw_cfg)) {
		rc = bsc_xfer_start(adapter);
		if (rc < 0) {
			dev_err(dev->device, "start command failed\n");
			goto err_ret;
		}
	}

	/* high-speed mode handshake, if not PMU adapter */
	if (dev->high_speed_mode && hw_cfg && !hw_cfg->is_pmu_i2c) {
		rc = start_high_speed_mode(adapter);
		if (rc < 0)
			goto err_ret;
	}

	/* send the restart command in high-speed */
	if (dev->high_speed_mode) {
		rc = bsc_xfer_repstart(adapter);
		if (rc < 0) {
			dev_err(dev->device, "restart command failed\n");
			goto hs_ret;
		}
	}

	/* loop through all messages */
	for (i = 0; i < num; i++) {
		pmsg = &msgs[i];
		nak_ok = pmsg->flags & I2C_M_IGNORE_NAK;

		/* need restart + slave address */
		if (!(pmsg->flags & I2C_M_NOSTART)) {
			/* send repeated start only on subsequent messages */
			if (i) {
				rc = bsc_xfer_repstart(adapter);
				if (rc < 0) {
					dev_err(dev->device,
						"restart command failed\n");
					goto hs_ret;
				}
			}

			rc = bsc_xfer_do_addr(adapter, pmsg);
			if (rc < 0) {
				dev_err(dev->device,
					"NAK from device addr %2.2x msg#%d\n",
					pmsg->addr, i);
				goto hs_ret;
			}
		}

		/* read from the slave */
		if (pmsg->flags & I2C_M_RD) {
			rc = bsc_xfer_read(adapter, pmsg);
			BSC_DBG(dev, "read %d bytes msg#%d\n", rc, i);
			if (rc < pmsg->len) {
				dev_err(dev->device,
					"read %d bytes but asked for %d bytes\n",
					rc, pmsg->len);
				rc = (rc < 0) ? rc : -EREMOTEIO;
				goto hs_ret;
			}
		} else {	/* write to the slave */

			/* write bytes from buffer */
			rc = bsc_xfer_write(adapter, pmsg);
			BSC_DBG(dev, "wrote %d bytes msg#%d\n", rc, i);
			if (rc < pmsg->len) {
				dev_err(dev->device,
					"wrote %d bytes but asked for %d bytes\n",
					rc, pmsg->len);
				rc = (rc < 0) ? rc : -EREMOTEIO;
				goto hs_ret;
			}
		}
	}

	/* send stop command, if not PMU in HS mode */
	if (hw_cfg && !hw_cfg->is_pmu_i2c) {
		rc = bsc_xfer_stop(adapter);
		if (rc < 0)
			dev_err(dev->device, "stop command failed\n");
	}

	/* Switch back to F/S mode, if not PMU adapter */
	if (dev->high_speed_mode && hw_cfg && !hw_cfg->is_pmu_i2c)
		stop_high_speed_mode(adapter);

	/* Disable the fifos if previously enabled */
	client_fifo_configure(adapter, msgs[0].addr, false);

	bsc_enable_pad_output((uint32_t)dev->virt_base, false);
#ifdef CONFIG_CAPRI_BSC3_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C2) {
		pinmux_set_pin_config(&gpin_bsc3_scl_config[0]);
		/* add a delay so it wont generate START condition by mistake */
		udelay(1);
		pinmux_set_pin_config(&gpin_bsc3_sda_config[0]);
	}
#endif

#ifdef CONFIG_CAPRI_BSC1_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C0) {
		pinmux_set_pin_config(&gpin_bsc1_scl_config[0]);
		/* add a delay so it wont generate START condition by mistake */
		udelay(1);
		pinmux_set_pin_config(&gpin_bsc1_sda_config[0]);
	}
#endif

	bsc_disable_clk(dev);
#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
	if (rel_hw_sem)
		pwr_mgr_pm_i2c_sem_unlock();
#endif
#ifdef CONFIG_ARCH_CAPRI
	resume_nohz();
#endif
	up(&dev->dev_lock);
	return (rc < 0) ? rc : num;

hs_ret:

	/* Here we should not code such as rc = bsc_xfer_stop(), since it would
	 * change the value of rc, which need to be passed to the caller */
	/* send stop command */
	if (hw_cfg && !hw_cfg->is_pmu_i2c)
		if (bsc_xfer_stop(adapter) < 0)
			dev_err(dev->device, "stop command failed\n");

	/* Switch back to F/S mode, if not PMU adapter */
	if (dev->high_speed_mode && hw_cfg && !hw_cfg->is_pmu_i2c)
		stop_high_speed_mode(adapter);

 err_ret:
	/* Disable the fifos if previously enabled */
	client_fifo_configure(adapter, msgs[0].addr, false);

	bsc_enable_pad_output((uint32_t)dev->virt_base, false);
#ifdef CONFIG_CAPRI_BSC3_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C2) {
		pinmux_set_pin_config(&gpin_bsc3_scl_config[0]);
		pinmux_set_pin_config(&gpin_bsc3_sda_config[0]);
	}
#endif

#ifdef CONFIG_CAPRI_BSC1_PAD_CTRL_MODE 
	if (dev->irq == BCM_INT_ID_I2C0) {
		pinmux_set_pin_config(&gpin_bsc1_scl_config[0]);
		pinmux_set_pin_config(&gpin_bsc1_sda_config[0]);
	}
#endif

	bsc_disable_clk(dev);
#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
	if (rel_hw_sem)
		pwr_mgr_pm_i2c_sem_unlock();
#endif
#ifdef CONFIG_ARCH_CAPRI
	resume_nohz();
#endif
	up(&dev->dev_lock);
	return rc;
}

static u32 bsc_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL &
			       ~(I2C_FUNC_SMBUS_WRITE_BLOCK_DATA |
				 I2C_FUNC_SMBUS_READ_BLOCK_DATA)) |
	    I2C_FUNC_10BIT_ADDR | I2C_FUNC_PROTOCOL_MANGLING;
}

static struct i2c_algorithm bsc_algo = {
	.master_xfer = bsc_xfer,
	.functionality = bsc_functionality,
};

static int bsc_enable_clk(struct bsc_i2c_dev *dev)
{
#ifdef CONFIG_MACH_CAPRI_FPGA
	return 0;
#endif
	int ret = 0;
	if (dev->bsc_apb_clk)
		ret |= clk_enable(dev->bsc_apb_clk);
	if (dev->bsc_clk)
		ret |= clk_enable(dev->bsc_clk);
	return ret;
}

static void bsc_disable_clk(struct bsc_i2c_dev *dev)
{
#ifdef CONFIG_MACH_CAPRI_FPGA
	return;
#endif
	if (dev->bsc_clk)
		clk_disable(dev->bsc_clk);
	if (dev->bsc_apb_clk)
		clk_disable(dev->bsc_apb_clk);
}

static void i2c_master_reset(struct work_struct *work)
{
	int rc;
	struct bsc_i2c_dev *dev = container_of(work, struct bsc_i2c_dev,
					       reset_work);
	struct i2c_adapter *adap = &dev->adapter;

	down(&dev->dev_lock);
	bsc_enable_clk(dev);

	dev_info(dev->device, "resetting i2c bus...\n");

	rc = bsc_xfer_stop(adap);
	if (rc < 0) {
		dev_err(dev->device, "failed to send stop command\n");
		/* still go ahead to reset the master */
	}

	/* reset BSC controller */
	bsc_reset((uint32_t)dev->virt_base);

	/* clear all interrupts */
	bsc_clear_intr_status((uint32_t)dev->virt_base, 0xFF);

	/* re-enable bus error (timeout) interrupt */
	if (!dev->high_speed_mode)
		bsc_enable_intr((uint32_t)dev->virt_base,
				I2C_MM_HS_IER_ERR_INT_EN_MASK);

	bsc_disable_clk(dev);
	up(&dev->dev_lock);
}

#ifdef CONFIG_MFD_BCM_PWRMGR_SW_SEQUENCER
void *pwmgr_pmu_bsc_init(unsigned long reg_start, unsigned long reg_end,
			 unsigned int irq, int *pstatus)
{
	int rc = 0;
	struct bsc_i2c_dev *dev;
	struct resource iomem, *ioarea;

	if (!pstatus) {
		rc = -EINVAL;
		return 0;
	}

	/* get register memory resource */
	iomem.start = reg_start,
	    iomem.end = reg_end, iomem.flags = IORESOURCE_MEM,
	    /* get the interrupt number */
	    irq = BCM_INT_ID_PM_I2C;

	/* mark the memory region as used */
	ioarea =
	    request_mem_region(iomem.start, resource_size(&iomem), "bsc-i2c");
	if (!ioarea) {
		dev_err(NULL, "I2C region already claimed\n");
		*pstatus = -EBUSY;
		return 0;
	}

	/* allocate memory for our private data structure */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(NULL, "unable to allocate mem for private data\n");
		rc = -ENOMEM;
		goto err_release_mem_region;
	}

	/* use default speed */
	dev->speed = BSC_BUS_SPEED_HS;
	dev->bsc_clk = NULL;
	dev->bsc_apb_clk = NULL;

	sema_init(&dev->dev_lock, 1);
	init_completion(&dev->ses_done);
	dev->debug = 0;
	dev->irq = irq;
	dev->virt_base = ioremap(iomem.start, resource_size(&iomem));
	if (!dev->virt_base) {
		dev_err(NULL, "ioremap of register space failed\n");
		rc = ENOMEM;
		goto err_free_dev_mem;
	}

	/*
	 * Configure BSC timing registers
	 * If PMU I2C - hs timing is calculated based on 26MHz source, else 104MHz
	 */
	bsc_set_bus_speed((uint32_t)dev->virt_base,
			  gBusSpeedTable[dev->speed], true);

	/* curent speed configured */
	dev->current_speed = dev->speed;

	/* init I2C controller */
	isl_bsc_init((uint32_t)dev->virt_base);

	/* disable and clear interrupts */
	bsc_disable_intr((uint32_t)dev->virt_base, 0xFF);
	bsc_clear_intr_status((uint32_t)dev->virt_base, 0xFF);

	/* register the ISR handler */
	rc = request_irq(dev->irq, pmu_bsc_isr, IRQF_SHARED, "bsc-i2c", dev);
	if (rc) {
		dev_err(NULL, "failed to request irq %i\n", dev->irq);
		goto err_free_iomap;
	}

	bsc_enable_intr((uint32_t)dev->virt_base,
			I2C_MM_HS_IER_ERR_INT_EN_MASK);

	/* PMU I2C: Switch to HS mode once. This is a workaround needed for
	 * Power manager sequencer to function properly.
	 *
	 * PMU adapter will always be in HS, dont switch back F/S from now onwards.
	 *
	 */
	/* send start command */
	rc = bsc_send_cmd(dev, BSC_CMD_START);
	if (rc < 0) {
		dev_err(dev->device, "start command failed\n");
		goto err_free_irq;
	}

	/*
	 * mastercode (0000 1000 + #id)
	 */
	dev->mastercode = (MASTERCODE | (MASTERCODE_MASK & 2)) + 1;
	dev->is_mastercode = true;
	/* send the master code in F/S mode first */
	rc = bsc_xfer_write_byte(dev, 1, &dev->mastercode);
	if (rc < 0) {
		dev_err(dev->device, "high-speed master code failed\n");
		dev->is_mastercode = false;
		goto err_free_irq;
	}

	/* check to make sure no slave replied to the master code by accident */
	if (bsc_get_ack((uint32_t)dev->virt_base)) {
		dev_err(dev->device,
			"one of the slaves replied to the high-speed "
			"master code unexpectedly\n");
		dev->is_mastercode = false;
		rc = -EREMOTEIO;
		goto err_free_irq;
	}

	/*
	 * Now save the BSC_TIM register value as it will be modified before the
	 * master going into high-speed mode. We need to restore the BSC_TIM
	 * value when the device switches back to fast speed
	 */
	dev->tim_val = bsc_get_tim((uint32_t)dev->virt_base);
	/* Turn-off autosense and Tout interrupt for HS mode */
	bsc_disable_intr((uint32_t)dev->virt_base,
			 I2C_MM_HS_IER_ERR_INT_EN_MASK);
	bsc_set_autosense((uint32_t)dev->virt_base, 0, 0);

	/* configure the bus into high-speed mode */
	bsc_start_highspeed((uint32_t)dev->virt_base);
	dev_err(dev->device, "Adapter is switched to HS mode\n");
	bsc_set_tx_fifo((uint32_t)dev->virt_base, 1);
	return (void *)dev;

err_free_irq:
	free_irq(dev->irq, dev);
	bsc_deinit((uint32_t)dev->virt_base);
err_free_iomap:
	iounmap(dev->virt_base);
err_free_dev_mem:
	kfree(dev);
err_release_mem_region:
	release_mem_region(iomem.start, resource_size(&iomem));
	*pstatus = rc;
	return 0;
}

int pwmgr_pmu_bsc_deinit(void *handle, unsigned long reg_start,
			 unsigned long reg_end)
{
	struct bsc_i2c_dev *dev;
	struct resource iomem;

	if (!handle)
		return -ENODEV;
	dev = (struct bsc_i2c_dev *)handle;

	bsc_deinit((uint32_t)dev->virt_base);
	iounmap(dev->virt_base);
	free_irq(dev->irq, dev);
	iomem.start = reg_start,
	    iomem.end = reg_end,
	    iomem.flags = IORESOURCE_MEM,
	    release_mem_region(iomem.start, resource_size(&iomem));

	return 0;
}
#endif

static int __devinit bsc_probe(struct platform_device *pdev)
{
	int rc = 0, irq;
	struct bsc_adap_cfg *hw_cfg = 0;
	struct bsc_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *iomem, *ioarea;

	printk(gBanner);

#ifdef CONFIG_MFD_BCM_PWRMGR_SW_SEQUENCER
	if (pdev->dev.platform_data) {
		hw_cfg = (struct bsc_adap_cfg *)pdev->dev.platform_data;
		if (hw_cfg && hw_cfg->is_pmu_i2c) {
			/* allocate memory for our private data structure */
			dev = kzalloc(sizeof(*dev), GFP_KERNEL);
			if (!dev) {
				dev_err(&pdev->dev,
					"unable to allocate mem for private data\n");
				return -ENOMEM;
			}
			adap = &dev->adapter;
			i2c_set_adapdata(adap, dev);
			adap->owner = THIS_MODULE;
			/* can be used by any I2C device */
			adap->class = UINT_MAX;
			snprintf(adap->name, sizeof(adap->name), "bsc-i2c%d",
				 pdev->id);
			adap->algo = &bsc_algo;
			adap->dev.parent = &pdev->dev;
			adap->nr = pdev->id;
			adap->retries = hw_cfg->retries;
			rc = i2c_add_numbered_adapter(adap);
			if (rc) {
				dev_err(dev->device, "failed to add adapter\n");
				kfree(dev);
				return rc;
			}
			return 0;
		}
	}
#endif

	/* get register memory resource */
	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem) {
		dev_err(&pdev->dev, "no mem resource\n");
		kfree(dev);
		return -ENODEV;
	}

	/* get the interrupt number */
	irq = platform_get_irq(pdev, 0);
	if (irq == -ENXIO) {
		dev_err(&pdev->dev, "no irq resource\n");
		kfree(dev);		
		return -ENODEV;
	}

	/* mark the memory region as used */
	ioarea = request_mem_region(iomem->start, resource_size(iomem),
				    pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "I2C region already claimed\n");
		kfree(dev);		
		return -EBUSY;
	}

	/* allocate memory for our private data structure */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&pdev->dev,
			"unable to allocate mem for private data\n");
		rc = -ENOMEM;
		goto err_release_mem_region;
	}

	/* init clocks */
	dev->bsc_apb_clk = clk_get(&pdev->dev, "bus_clk");
	if (IS_ERR(dev->bsc_apb_clk))
		goto err_free_dev_mem;

	dev->bsc_clk = clk_get(&pdev->dev, "peri_clk");
	if (IS_ERR(dev->bsc_clk))
		goto err_free_bus_clk;

	rc = bsc_enable_clk(dev);
	if (rc)
		goto err_free_peri_clk;

	if (pdev->dev.platform_data) {
		hw_cfg = (struct bsc_adap_cfg *)pdev->dev.platform_data;
		dev->speed = hw_cfg->speed;
		dev->dynamic_speed = hw_cfg->dynamic_speed;
	} else {
		/* use default speed */
		dev->speed = DEFAULT_I2C_BUS_SPEED;
		dev->bsc_clk = NULL;
		dev->bsc_apb_clk = NULL;
	}

	/* Initialize the error flag */
	dev->err_flag = 0;

	/* validate the speed parameter */
	if (dev->speed >= BSC_BUS_SPEED_MAX) {
		dev_err(&pdev->dev, "invalid bus speed parameter\n");
		rc = -EFAULT;
		goto err_disable_clk;
	}

	/* high speed - For any speed defined between BSC_BUS_SPEED_HS
     * & BSC_BUS_SPEED_HS_FPGA */
	if (dev->speed >= BSC_BUS_SPEED_HS
	    && dev->speed <= BSC_BUS_SPEED_HS_FPGA)
		dev->high_speed_mode = 1;
	else
		dev->high_speed_mode = 0;

	dev->device = &pdev->dev;
	sema_init(&dev->dev_lock, 1);
	init_completion(&dev->ses_done);
	init_completion(&dev->rx_ready);
	init_completion(&dev->tx_fifo_empty);
	atomic_set(&dev->tx_fifo_support, 0);
	atomic_set(&dev->rx_fifo_support, 0);
	dev->debug = 0;
	dev->irq = irq;
	dev->virt_base = ioremap(iomem->start, resource_size(iomem));
	if (!dev->virt_base) {
		dev_err(&pdev->dev, "ioremap of register space failed\n");
		rc = -ENOMEM;
		goto err_disable_clk;
	}

	platform_set_drvdata(pdev, dev);

#ifdef CONFIG_CAPRI_BSC3_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C2) {
		pinmux_set_pin_config(&gpin_bsc3_scl_config[1]);
		pinmux_set_pin_config(&gpin_bsc3_sda_config[1]);
	}
#endif

#ifdef CONFIG_CAPRI_BSC1_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C0) {
		pinmux_set_pin_config(&gpin_bsc1_scl_config[1]);
		pinmux_set_pin_config(&gpin_bsc1_sda_config[1]);
	}
#endif

	/*
	 * Configure BSC timing registers
	 * HS timing is calculated based on 26MHz source
	 */
	if (hw_cfg && hw_cfg->is_pmu_i2c)
		bsc_set_bus_speed((uint32_t)dev->virt_base,
				  gBusSpeedTable[dev->speed], true);
	else
		bsc_set_bus_speed((uint32_t)dev->virt_base,
				  gBusSpeedTable[dev->speed], false);

	/* curent speed configured */
	dev->current_speed = dev->speed;

	/* init I2C controller */
	isl_bsc_init((uint32_t)dev->virt_base);

	/* disable and clear interrupts */
	bsc_disable_intr((uint32_t)dev->virt_base, 0xFF);
	bsc_clear_intr_status((uint32_t)dev->virt_base, 0xFF);

	/* Enable the Thigh control */
	/* T-high in SS/FS mode varies when the clock gets stretched in the A1
	 * Variant. . */
	if (hw_cfg && !hw_cfg->is_pmu_i2c && (get_chip_version() >= CAPRI_A1))
		bsc_enable_thigh_ctrl((uint32_t)dev->virt_base, true);
	else
		bsc_enable_thigh_ctrl((uint32_t)dev->virt_base, false);

	INIT_WORK(&dev->reset_work, i2c_master_reset);
	dev->reset_wq = create_workqueue("i2c_master_reset");
	if (dev->reset_wq == NULL) {
		dev_err(dev->device, "unable to create bus reset workqueue\n");
		rc = -ENOMEM;
		goto err_bsc_deinit;
	}

	/* register the ISR handler */
	rc = request_irq(dev->irq, bsc_isr, IRQF_SHARED, pdev->name, dev);
	if (rc) {
		dev_err(&pdev->dev, "failed to request irq %i\n", dev->irq);
		goto err_destroy_wq;
	}

	dev_info(dev->device, "bus %d at speed %d\n", pdev->id, dev->speed);

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = UINT_MAX;	/* can be used by any I2C device */
	snprintf(adap->name, sizeof(adap->name), "bsc-i2c%d", pdev->id);
	adap->algo = &bsc_algo;
	adap->dev.parent = &pdev->dev;
	adap->nr = pdev->id;
	if(hw_cfg)
		adap->retries = hw_cfg->retries;
	else
		adap->retries = 0;

	/*
	 * I2C device drivers may be active on return from
	 * i2c_add_numbered_adapter()
	 */
	rc = i2c_add_numbered_adapter(adap);
	if (rc) {
		dev_err(dev->device, "failed to add adapter\n");
		goto err_free_irq;
	}

	bsc_enable_intr((uint32_t)dev->virt_base,
			I2C_MM_HS_IER_ERR_INT_EN_MASK);

	if (hw_cfg && hw_cfg->autosense_tout_disa)
		bsc_set_autosense((uint32_t)dev->virt_base, 1, 0);
	else
		bsc_set_autosense((uint32_t)dev->virt_base, 1, 1);

	/* configure the bsc clock to 26MHz for PMU-I2C, 13MHz for BSC */
	if (dev->bsc_clk) {
		clk_disable(dev->bsc_clk);
		if (hw_cfg && hw_cfg->is_pmu_i2c)
			clk_set_rate(dev->bsc_clk, 26000000);
		else
			clk_set_rate(dev->bsc_clk, 13000000);

		clk_enable(dev->bsc_clk);
	}

	/* PMU I2C: Switch to HS mode once. This is a workaround needed for
	 * Power manager sequencer to function properly.
	 *
	 * PMU adapter will always be in HS, dont switch back F/S from now onwards.
	 *
	 */
	if (dev->high_speed_mode && hw_cfg && hw_cfg->is_pmu_i2c) {
#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
		rc = pwr_mgr_pm_i2c_sem_lock();
		if (rc) {
			dev_err(dev->device,
				"Failed to acquire PMU HW semaphore!!!\n");
			goto err_free_irq;
		}
#endif
		/* send start command */
		if (bsc_xfer_start(adap) < 0) {
			dev_err(dev->device, "start command failed\n");
			goto err_hw_sem;
		}
		/* HS mode handshake for PMU adapter */
		if (start_high_speed_mode(adap) < 0) {
			dev_err(dev->device, "failed to switch HS mode\n");
			goto err_hw_sem;
		}
#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
		pwr_mgr_pm_i2c_sem_unlock();
#endif
	}

	bsc_enable_pad_output((uint32_t)dev->virt_base, false);
#ifdef CONFIG_CAPRI_BSC3_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C2) {
		pinmux_set_pin_config(&gpin_bsc3_scl_config[0]);
		pinmux_set_pin_config(&gpin_bsc3_sda_config[0]);
	}
#endif

#ifdef CONFIG_CAPRI_BSC1_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C0) {
		pinmux_set_pin_config(&gpin_bsc1_scl_config[0]);
		pinmux_set_pin_config(&gpin_bsc1_sda_config[0]);
	}
#endif

	bsc_disable_clk(dev);

#ifdef CONFIG_DEBUG_FS
	/*create debugfs interface for the device*/
	bsc_dentry = debugfs_create_dir(adap->name, NULL);
	if (!bsc_dentry && IS_ERR(bsc_dentry)) {
		printk(KERN_ERR "Failed to create debugfs directory\n");
	return PTR_ERR(bsc_dentry);
	}
	bsc_speed = debugfs_create_file("speed", 0444, bsc_dentry, adap,
			&default_file_operations);
	if (!bsc_speed && IS_ERR(bsc_speed)) {
		printk(KERN_ERR "Failed to create debugfs file\n");
		return PTR_ERR(bsc_speed);
	}
#endif /*CONFIG_DEBUG_FS*/
	return 0;

err_hw_sem:
#ifdef CONFIG_KONA_PMU_BSC_USE_PMGR_HW_SEM
	pwr_mgr_pm_i2c_sem_unlock();
#endif

err_free_irq:
	free_irq(dev->irq, dev);

err_destroy_wq:
	if (dev->reset_wq)
		destroy_workqueue(dev->reset_wq);

err_bsc_deinit:
	bsc_set_autosense((uint32_t)dev->virt_base, 0, 0);
	bsc_deinit((uint32_t)dev->virt_base);

	iounmap(dev->virt_base);

	platform_set_drvdata(pdev, NULL);

#ifdef CONFIG_CAPRI_BSC3_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C2) {
		pinmux_set_pin_config(&gpin_bsc3_scl_config[0]);
		pinmux_set_pin_config(&gpin_bsc3_sda_config[0]);
	}
#endif

#ifdef CONFIG_CAPRI_BSC1_PAD_CTRL_MODE
	if (dev->irq == BCM_INT_ID_I2C0) {
		pinmux_set_pin_config(&gpin_bsc1_scl_config[0]);
		pinmux_set_pin_config(&gpin_bsc1_sda_config[0]);
	}
#endif

err_disable_clk:
	bsc_disable_clk(dev);

err_free_peri_clk:
	clk_put(dev->bsc_clk);

err_free_bus_clk:
	clk_put(dev->bsc_apb_clk);

err_free_dev_mem:
	dev_err(dev->device, "bus %d probe failed\n", pdev->id);
	kfree(dev);

err_release_mem_region:
	release_mem_region(iomem->start, resource_size(iomem));

	return rc;
}

static int bsc_remove(struct platform_device *pdev)
{
	struct bsc_i2c_dev *dev = platform_get_drvdata(pdev);
	struct resource *iomem;

	i2c_del_adapter(&dev->adapter);

	platform_set_drvdata(pdev, NULL);
	free_irq(dev->irq, dev);

	if (dev->reset_wq)
		destroy_workqueue(dev->reset_wq);

	bsc_set_autosense((uint32_t)dev->virt_base, 0, 0);
	bsc_deinit((uint32_t)dev->virt_base);

	iounmap(dev->virt_base);

	bsc_disable_clk(dev);

	if (dev->bsc_clk) {
		clk_put(dev->bsc_clk);
		dev->bsc_clk = NULL;
	}
	if (dev->bsc_apb_clk) {
		clk_put(dev->bsc_apb_clk);
		dev->bsc_apb_clk = NULL;
	}

	kfree(dev);

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(iomem->start, resource_size(iomem));

#ifdef CONFIG_DEBUG_FS
	debugfs_remove(bsc_speed);
	debugfs_remove(bsc_dentry);
#endif /*CONFIG_DEBUG_FS*/
	return 0;
}

#ifdef CONFIG_PM
static int bsc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct bsc_i2c_dev *dev = platform_get_drvdata(pdev);
#ifdef CONFIG_MFD_BCM_PWRMGR_SW_SEQUENCER
	if (pdev && pdev->dev.platform_data)
		if (((struct bsc_adap_cfg *)
		     pdev->dev.platform_data)->is_pmu_i2c)
			return 0;
#endif
	/* flush the workqueue to make sure all outstanding work items are done */
	flush_workqueue(dev->reset_wq);

	/* grab lock to prevent further I2C transactions */
	down(&dev->dev_lock);

	/*
	 * Don't need to disable BSC clocks here since they are now only
	 * turned on for each transaction
	 */
	return 0;
}

static int bsc_resume(struct platform_device *pdev)
{
	struct bsc_i2c_dev *dev = platform_get_drvdata(pdev);
#ifdef CONFIG_MFD_BCM_PWRMGR_SW_SEQUENCER
	if (pdev && pdev->dev.platform_data)
		if (((struct bsc_adap_cfg *)
		     pdev->dev.platform_data)->is_pmu_i2c)
			return 0;
#endif
	up(&dev->dev_lock);
	return 0;
}
#else
#define bsc_suspend    NULL
#define bsc_resume     NULL
#endif

static struct platform_driver bsc_driver = {
	.driver = {
		   .name = "bsc-i2c",
		   .owner = THIS_MODULE,
		   },
	.probe = bsc_probe,
	.remove = __devexit_p(bsc_remove),
	.suspend = bsc_suspend,
	.resume = bsc_resume,
};

static int __init bsc_init(void)
{
	int rc;

	rc = platform_driver_register(&bsc_driver);
	if (rc < 0) {
		printk(KERN_ERR "I2C driver init failed\n");
		return rc;
	}

	return 0;
}

static void __exit bsc_exit(void)
{
	platform_driver_unregister(&bsc_driver);
}

arch_initcall(bsc_init);
module_exit(bsc_exit);

#ifdef CONFIG_DEBUG_FS
static int show_i2c_bus_speed(struct seq_file *m, void *v)
{
	struct bsc_i2c_dev *dev;
	struct i2c_adapter *adap;

	adap = (struct i2c_adapter *)m->private;
	if (!adap)
		return -ENODEV;

	dev = i2c_get_adapdata(adap);
	switch (dev->speed) {
	case BSC_SPD_400K:
		seq_printf(m, "400K\n");
		break;
	case BSC_SPD_100K:
		seq_printf(m, "100K\n");
		break;
	case BSC_SPD_50K:
		seq_printf(m, "50K\n");
		break;
	default:
		seq_printf(m, "%d\n", dev->speed);
		break;
	}
	return 0;
}

/*sysfs interface to change bsc speed - used by hwtest*/
static ssize_t set_i2c_bus_speed(struct file *file,
				char const __user *buf, size_t size,
					loff_t *offset)
{
	struct bsc_i2c_dev *dev;
	struct device *d;
	struct i2c_adapter *adap;
	struct bsc_adap_cfg *hw_cfg = NULL;
	int speed = 0, buf_size, bus;
	unsigned int addr;
	char data[64];

	buf_size = min((int)size, 64);
	if (strncpy_from_user(data, buf, buf_size) < 0)
		return -EFAULT;
	data[buf_size] = 0;

	adap = (struct i2c_adapter *)
		((struct seq_file *)file->private_data)->private;
	if (!adap)
		return -ENODEV;

	dev = i2c_get_adapdata(adap);

	sscanf(data, "%d 0x%x", &speed, &addr);
	if (speed == 400)
		speed = BSC_SPD_400K;
	else if (speed == 100)
		speed = BSC_SPD_100K;
	else
		speed = BSC_SPD_50K;	/* Default speed */

	dev->speed = speed;
	if (!dev) {
		printk(KERN_ERR "Cannot find i2c device\n");
		return -ENODEV;
	}

	d = bsc_i2c_get_client(adap, addr);
	if (d) {
		struct i2c_client *client = NULL;
		struct i2c_slave_platform_data *pd = NULL;
		client = i2c_verify_client(d);
		pd = (struct i2c_slave_platform_data *)client->dev.
		    platform_data;
		if (pd)
			pd->i2c_speed = speed;
	} else
		/*Not an error- as client address is optional parameter */
		printk(KERN_ERR "Cannot find client at 0x%x\n", addr);

	hw_cfg = (struct bsc_adap_cfg *)dev->device->platform_data;
	if (hw_cfg->is_pmu_i2c)
		bsc_set_bus_speed((uint32_t)dev->virt_base,
					gBusSpeedTable[speed], true);
	else
		bsc_set_bus_speed((uint32_t)dev->virt_base,
					gBusSpeedTable[speed], false);
	dev->current_speed = speed;

	return size;
}

static int bsc_clk_open(struct inode *inode, struct file *file)
{
	return single_open(file, show_i2c_bus_speed, inode->i_private);
}
#endif/*CONFIG_DEBUG_FS*/

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Broadcom I2C Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
