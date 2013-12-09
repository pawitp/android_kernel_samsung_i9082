/*
 * isl29044.c - Intersil ISL29028  ALS & Proximity Driver
 *
 * By Intersil Corp
 * Michael DiGioia
 *
 * Based on isl29028a.c
 *	by Michael DiGioia <mdigioia@intersil.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/hwmon.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/pm_runtime.h>

#include <linux/miscdevice.h>
#include <linux/kfifo.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/poll.h>
#include <linux/errno.h>
#include <linux/fs.h>

#include <linux/isl29044.h>

/* Insmod parameters */
//I2C_CLIENT_INSMOD_1(isl29044);

#define MODULE_NAME	ISL29044_MODULE_NAME

/* registers */
#define ISL29028_REG_VENDOR_REV                 0x06
#define ISL29028_VENDOR                         1
#define ISL29028_VENDOR_MASK                    0x0F
#define ISL29028_REV                            4
#define ISL29028_REV_SHIFT                      4
#define ISL29028_REG_DEVICE                     0x22
#define ISL29028_DEVICE                        	22 

// Table 1: all i2c registers and bits per register
#define REG_CMD_1		0x01 // configure, range is reg 1 bit 1
#define REG_CMD_2		0x02 // interrupt control

#define REG_INT_LOW_PROX	0x03 // 8 bits intr low thresh for prox
#define REG_INT_HIGH_PROX	0x04 // 8 bits intr high thresh for prox
#define REG_INT_LOW_ALS 	0x05 // 8 bits intr low thresh for ALS-IR
#define REG_INT_LOW_HIGH_ALS	0x06 // 8 bits(0-3,4-7) intr high/low thresh for ALS-IR
#define REG_INT_HIGH_ALS	0x07 // 8 bits intr high thresh for ALS-IR

#define REG_DATA_PROX		0x08 // 8 bits of PROX data
#define REG_DATA_LSB_ALS	0x09 // 8 bits of ALS data
#define REG_DATA_MSB_ALS	0x0A // 4 bits of ALS MSB data

#define ISL_TEST1 		0x0E // test write 0x00
#define ISL_TEST2 		0x0F // test write 0x00

#define ISL_MOD_MASK		0xE0
#define ISL_MOD_POWERDOWN	0

#define ISL_MOD_ALS_ONCE	1
#define ISL_MOD_IR_ONCE		2
#define ISL_MOD_PS_ONCE		3
#define ISL_MOD_RESERVED	4
#define ISL_MOD_ALS_CONT	5
#define ISL_MOD_IR_CONT		6
#define ISL_MOD_PS_CONT		7
#define ISL_MOD_DEFAULT		8

#define PROX_EN_MASK          0x80 // prox sense on mask, 1=on, 0=off
#define PROX_CONT_MASK        0x70 // prox sense contimnuous mask
//IR_CURRENT_MASK is now PROX_DR_MASK with just 0 or 1 settings
#define PROX_DR_MASK          0x08 // prox drive pulse 220ma sink mask def=0 110ma
#define ALS_EN_MASK           0x04 // prox sense enabled contimnuous mask
#define ALS_RANGE_HIGH_MASK   0x02 // ALS range high LUX mask
#define ALSIR_MODE_SPECT_MASK 0x01 // prox sense contimnuous mask

#define IR_CURRENT_MASK		0xC0
#define IR_FREQ_MASK		0x30
#define SENSOR_RANGE_MASK	0x03
#define ISL_RES_MASK		0x0C

#define START_TIME_DELAY 50000000

#define ALS_SKIP_DATA  -2
#define ALS_NO_DATA    -1

struct isl29044_dev_data {
	struct miscdevice dev;
	struct i2c_client *client;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
	DECLARE_KFIFO (ebuff, ISL29044_EVENT_NUM * sizeof(struct isl29044_val));
#else
	DECLARE_KFIFO (ebuff, struct isl29044_val, ISL29044_EVENT_NUM);
#endif
	wait_queue_head_t waitq;
	struct workqueue_struct *wq_als;
	struct workqueue_struct *wq_ps;
	struct workqueue_struct *wq_cntl;
	struct work_struct work_als;
	struct work_struct work_ps;
	int enable_wq_als;
	int enable_wq_ps;
	ktime_t poll_delay_als;
	ktime_t poll_delay_ps;
	struct hrtimer timer_als;
	struct hrtimer timer_ps;

	uint8_t ps_max_thr;
	uint8_t ps_reflect_thr;
	uint16_t als_diff;
	int last_als;
};

typedef struct {
  struct work_struct cntl_work;
  struct isl29044_dev_data *pdev;
  int8_t sensor;
  int8_t en;
} cntl_work_t;

static int last_mod;

static DEFINE_MUTEX(mutex);
////////////////////////////////
static void isl29044_work_als_func(struct work_struct *work)
{
	struct isl29044_dev_data *pdev = container_of(work, struct isl29044_dev_data, work_als);
	int tmp, reg2;
	isl29044_val_t res;

	if((tmp=mutex_lock_interruptible(&mutex)))
	{
		dev_err(&pdev->client->adapter->dev, "%s: mutex_lock_interruptible returned %d\n", __func__, tmp);
		return;
	}

	if((reg2 = i2c_smbus_read_byte_data(pdev->client, REG_CMD_2)) < 0)
		goto do_exit;

	res.type = ALS_EVENT;
	if((reg2 & 0x08) == 0)
	{
		if(pdev->last_als < 0)
			goto do_exit;
		res.val = pdev->last_als;
	}
	else
	{
		if(((tmp = i2c_smbus_read_byte_data(pdev->client, REG_DATA_MSB_ALS)) < 0) ||
			((res.val = i2c_smbus_read_byte_data(pdev->client, REG_DATA_LSB_ALS)) < 0))
			goto do_exit;

		res.val |= tmp << 8;

		reg2 &= ~(0x08);
		i2c_smbus_write_byte_data(pdev->client, REG_CMD_2, reg2);

		if(pdev->last_als == ALS_SKIP_DATA) /* skip the first measurement - known issue with the sensor */
			pdev->last_als = ALS_NO_DATA;
		else
		{
			int min_thr, max_thr;
			
			pdev->last_als = res.val;
		
			if((min_thr = res.val - pdev->als_diff) < 0)
				min_thr = 0;
			if((max_thr = res.val + pdev->als_diff) > 0x0fff)
				max_thr = 0x0fff;

			i2c_smbus_write_byte_data(pdev->client, REG_INT_LOW_HIGH_ALS, ((max_thr>>4)&0xf0)|((min_thr>>8)&0x0f));
			i2c_smbus_write_byte_data(pdev->client, REG_INT_HIGH_ALS, (max_thr&0xff));
			i2c_smbus_write_byte_data(pdev->client, REG_INT_LOW_ALS, (min_thr&0xff));
		}
	}
	mutex_unlock(&mutex);

	if(pdev->last_als >= 0)
	{
		kfifo_in(&pdev->ebuff, &res, 1);
		wake_up_interruptible(&pdev->waitq);
//		printk(KERN_ERR "%s: pass(%d) to HAL\n", __func__, res.val);
	}
	return;

do_exit:
	mutex_unlock(&mutex);
}

static void isl29044_work_ps_func(struct work_struct *work)
{
	struct isl29044_dev_data *pdev = container_of(work, struct isl29044_dev_data, work_ps);
	int tmp;
	isl29044_val_t res;

	if((tmp=mutex_lock_interruptible(&mutex)))
	{
		dev_err(&pdev->client->adapter->dev, "%s: mutex_lock_interruptible returned %d\n", __func__, tmp);
		return;
	}
	res.type = PS_EVENT;

	if((tmp = i2c_smbus_read_byte_data(pdev->client, REG_DATA_PROX)) < 0)
		goto do_exit;

	res.val = (tmp > pdev->ps_reflect_thr ? 0:1);
	mutex_unlock(&mutex);

	kfifo_in(&pdev->ebuff, &res, 1);
	wake_up_interruptible(&pdev->waitq);
	printk(KERN_ERR "%s: pass(%d) to HAL\n", __func__, res.val);
	return;

do_exit:
	mutex_unlock(&mutex);
}

static enum hrtimer_restart isl29044_timer_als_func(struct hrtimer *timer)
{
	struct isl29044_dev_data *pdev = container_of(timer, struct isl29044_dev_data, timer_als);

	queue_work(pdev->wq_als, &pdev->work_als);
	hrtimer_forward_now(&pdev->timer_als, pdev->poll_delay_als);
	return HRTIMER_RESTART;
}

static enum hrtimer_restart isl29044_timer_ps_func(struct hrtimer *timer)
{
	struct isl29044_dev_data *pdev = container_of(timer, struct isl29044_dev_data, timer_ps);

	queue_work(pdev->wq_ps, &pdev->work_ps);
	hrtimer_forward_now(&pdev->timer_ps, pdev->poll_delay_ps);
	return HRTIMER_RESTART;
}

static int isl29044_enable_work_als(struct isl29044_dev_data *pdev, int enable)
{
	if(enable != pdev->enable_wq_als)
	{
		if(enable)
			hrtimer_start(&pdev->timer_als, ktime_set(0, START_TIME_DELAY), HRTIMER_MODE_REL);
		else
		{
			hrtimer_cancel(&pdev->timer_als);
			cancel_work_sync(&pdev->work_als);
			pdev->last_als = ALS_SKIP_DATA;
		}
		pdev->enable_wq_als = enable;
	}
	return 0;
}

static int isl29044_enable_work_ps(struct isl29044_dev_data *pdev, int enable)
{
	if(enable != pdev->enable_wq_ps)
	{
		if (enable)
			hrtimer_start(&pdev->timer_ps, ktime_set(0, START_TIME_DELAY), HRTIMER_MODE_REL);			
		else
		{
			hrtimer_cancel(&pdev->timer_ps);			
			cancel_work_sync(&pdev->work_ps);
		}
		pdev->enable_wq_ps = enable;
	}
	return 0;
}

static void isl29044_enable(struct work_struct *work)
{
	cntl_work_t *cntl = (cntl_work_t *)work;

	switch(cntl->sensor) {
	case ALS_EVENT:
		isl29044_enable_work_als(cntl->pdev, cntl->en);
		break;
	case PS_EVENT:
		isl29044_enable_work_ps(cntl->pdev, cntl->en);
		break;
	}
	kfree(work);
}

static int isl29044_open(struct inode *inode, struct file *file)
{
	return 0;
}

static long isl29044_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct isl29044_dev_data *pdev = container_of(file->private_data, struct isl29044_dev_data, dev);
	struct i2c_client *client = pdev->client;
	int reg1, reg2, ret = 0;
	int8_t en;
	int64_t poll_delay;
	cntl_work_t *cntl;

	ret = mutex_lock_interruptible(&mutex);
	if(ret)
	{
		dev_err(&client->adapter->dev, "%s: mutex_lock_interruptible returned %d\n", __func__, ret);
		return ret;
	}

	switch (cmd) {
	case ISL29044_ALS_ENABLE:
		if(copy_from_user(&en, (unsigned char __user *)arg, sizeof(int8_t)))
		{
			printk(KERN_ERR "ISL29044_ALS_ENABLE: failed\n");
			ret = -EFAULT;
			break;
		}
		printk(KERN_ERR "%sABLE Ambient Light sensor\n", en?"EN":"DIS");
		if(((reg1 = i2c_smbus_read_byte_data(client, REG_CMD_1)) < 0) || ((reg2 = i2c_smbus_read_byte_data(client, REG_CMD_2)) < 0))
		{
			ret = -EINVAL;
			break;
		}
		if(!(cntl = (cntl_work_t *)kmalloc(sizeof(cntl_work_t), GFP_KERNEL)))
		{
			ret = -ENOMEM;
			break;
		}
		cntl->pdev = pdev;
		cntl->en = en;
		cntl->sensor = ALS_EVENT;
		if(en) /* R1: b.0000.0110 = 0x06 R2: b.0000.0010 = 0x02 */
		{
			reg1 |= 0x06;
			reg2 |= 0x02;
		}
		else
		{
			reg1 &= ~(0x06);
			reg2 &= ~(0x02);
		}
		reg2 &= ~(0x08);
//		printk(KERN_ERR "set REG_CMD_1=0x%x REG_CMD_2=0x%x\n", reg1, reg2);
		if((i2c_smbus_write_byte_data(client, REG_CMD_1, reg1) < 0) ||
			(i2c_smbus_write_byte_data(client, REG_CMD_2, reg2) < 0) ||
			(i2c_smbus_write_byte_data(client, REG_INT_LOW_HIGH_ALS, 0) < 0) ||
			(i2c_smbus_write_byte_data(client, REG_INT_HIGH_ALS, 0) < 0) ||
			(i2c_smbus_write_byte_data(client, REG_INT_LOW_ALS, 0) < 0))
		{
			ret = -EINVAL;
			kfree( (void *)cntl);
			break;
		}

		INIT_WORK((struct work_struct *)cntl, isl29044_enable);
		queue_work(pdev->wq_cntl, (struct work_struct *)cntl);
//		printk(KERN_ERR "%sABLE Ambient Light sensor OK\n", en?"EN":"DIS");
		break;
	case ISL29044_PS_ENABLE:
		if(copy_from_user(&en, (unsigned char __user *)arg, sizeof(int8_t)))
		{
			printk(KERN_ERR "ISL29044_PS_ENABLE: failed\n");
			ret = -EFAULT;
			break;
		}
		printk(KERN_ERR "%sABLE Proximity sensor\n", en?"EN":"DIS");
		if(((reg1 = i2c_smbus_read_byte_data(client, REG_CMD_1)) < 0) || ((reg2 = i2c_smbus_read_byte_data(client, REG_CMD_2)) < 0))
		{
			ret = -EINVAL;
			break;
		}
		if(!(cntl = (cntl_work_t *)kmalloc(sizeof(cntl_work_t), GFP_KERNEL)))
		{
			ret = -ENOMEM;
			break;
		}
		cntl->pdev = pdev;
		cntl->en = en;
		cntl->sensor = PS_EVENT;
		if(en) /* b.1000.0000 = 0x80 */
			reg1 |= 0x80;
		else
			reg1 &= ~(0x80);
		reg2 &= ~(0x80);
//		printk(KERN_ERR "set REG_CMD_1=0x%x REG_CMD_2=0x%x\n", reg1, reg2);
		if((i2c_smbus_write_byte_data(client, REG_CMD_1, reg1) < 0) ||
			(i2c_smbus_write_byte_data(client, REG_CMD_2, reg2) < 0) ||
			(i2c_smbus_write_byte_data(client, REG_INT_LOW_PROX, 0) < 0) ||
			(i2c_smbus_write_byte_data(client, REG_INT_HIGH_PROX, pdev->ps_max_thr) < 0))
		{
			ret = -EINVAL;
			kfree( (void *)cntl);
			break;
		}
		INIT_WORK((struct work_struct *)cntl, isl29044_enable);
		queue_work(pdev->wq_cntl, (struct work_struct *)cntl);
//		printk(KERN_ERR "%sABLE Proximity sensor OK\n", en?"EN":"DIS");
		break;

	case ISL29044_SET_ALS_DELAY:
		if(copy_from_user(&poll_delay, (unsigned char __user *)arg, sizeof(int64_t))) {
			printk(KERN_ERR "ISL29044_SET_ALS_DELAY: failed\n");
			ret = -EFAULT;
		}
		else
			pdev->poll_delay_als = ns_to_ktime(poll_delay);
		break;

	case ISL29044_SET_PS_DELAY:
		if(copy_from_user(&poll_delay, (unsigned char __user *)arg, sizeof(int64_t))) {
			printk(KERN_ERR "ISL29044_SET_PS_DELAY: failed\n");
			ret = -EFAULT;
		}
		else
			pdev->poll_delay_ps = ns_to_ktime(poll_delay);
		break;

	default:
		dev_err(&client->adapter->dev, "%s: Unknown cmd %x, arg %lu\n", __func__, cmd, arg);
		ret = -EINVAL;
	}

	mutex_unlock(&mutex);
	dev_dbg(&client->adapter->dev, "%s: %08x, %08lx, %d\n", __func__, cmd, arg, ret);

	if (ret > 0)
		ret = -ret;

	return ret;
}

/* read function called when from /dev/isl29044 is read.  Read from the FIFO */
static ssize_t isl29044_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	struct isl29044_dev_data *pdev = container_of(file->private_data, struct isl29044_dev_data, dev);
	u32 iRead = 0;
	int res;
	
	wait_event_interruptible(pdev->waitq, (kfifo_len(&(pdev->ebuff))> 0));

	/* copy to user */
	res = kfifo_to_user(&pdev->ebuff, buf, count, &iRead);
	if (res < 0) {
		printk("isl29044_read failed to copy\n");
		return -EFAULT;
	}
	return iRead;
}

static unsigned int isl29044_poll(struct file *file, struct poll_table_struct *poll)
{
	struct isl29044_dev_data *pdev = container_of(file->private_data, struct isl29044_dev_data, dev);
	int mask = 0;

	poll_wait(file, &pdev->waitq, poll);
	if (kfifo_len(&pdev->ebuff) > 0) {
		mask = POLLIN | POLLRDNORM;
	}
	return mask;
}

/* define which file operations are supported */
static const struct file_operations isl29044_fops = {
	.owner = THIS_MODULE,
	.read = isl29044_read,
	.poll = isl29044_poll,
	.unlocked_ioctl = isl29044_ioctl,
	.open = isl29044_open,
};
////////////////////////////////
static int isl_set_range(struct i2c_client *client, int range)
{
	int ret_val;

	ret_val = i2c_smbus_read_byte_data(client, REG_CMD_1);
	if (ret_val < 0)
		return -EINVAL;
	ret_val &= ~ALS_RANGE_HIGH_MASK;  /*reset the bit */
	ret_val |= range;
	ret_val = i2c_smbus_write_byte_data(client, REG_CMD_1, ret_val);

	printk(KERN_INFO MODULE_NAME ": %s isl29044 set_range call, \n", __func__);
	if (ret_val < 0)
		return ret_val;
	return range;
}

//Set Mode of operation
static int isl_set_mod(struct i2c_client *client, int mod)
{
	int ret, val, freq;

	// check current mod
	switch (mod) {
	case ISL_MOD_POWERDOWN:
	case ISL_MOD_RESERVED:
		break;
	case ISL_MOD_ALS_ONCE:
	case ISL_MOD_ALS_CONT:
		freq = 0;
		break;
	case ISL_MOD_IR_ONCE:
	case ISL_MOD_IR_CONT:
	case ISL_MOD_PS_ONCE:
	case ISL_MOD_PS_CONT:
		freq = 1;
		break;
	default:
		return ISL_MOD_DEFAULT; // default is prox off and als off
	}

	/* set operation mod */
	val = i2c_smbus_read_byte_data(client, REG_CMD_1);
	if (val < 0)
		return -EINVAL;
	if (mod == ISL_MOD_PS_CONT) val |= PROX_EN_MASK;
	else if (mod == ISL_MOD_ALS_CONT) val |= ALS_EN_MASK;	
	else val |= ISL_MOD_RESERVED;

	ret = i2c_smbus_write_byte_data(client, REG_CMD_1, val);
	if (ret < 0)
		return -EINVAL;

	if (mod != ISL_MOD_POWERDOWN)
		last_mod = mod;

	return mod;
}
#if 0
static int isl_get_res(struct i2c_client *client)
{
	int val;

 printk(KERN_INFO MODULE_NAME ": %s isl29044 get_res call, \n", __func__);
	val = i2c_smbus_read_byte_data(client, REG_CMD_2);

	if (val < 0)
		return -EINVAL;

	val &= ISL_RES_MASK;
	val >>= 2;

	switch (val) {
	case 0:
	case 1:
	case 2:
	case 3:
		return 4096;
	default:
		return -EINVAL;
	}
}
#endif
static int isl_get_mod(struct i2c_client *client)
{
	int val;

	val = i2c_smbus_read_byte_data(client, REG_CMD_1);
	if (val < 0)
		return -EINVAL;
	if (val &= PROX_EN_MASK) val = ISL_MOD_PS_CONT;
	else if (val &= ALS_EN_MASK) val = ISL_MOD_ALS_CONT;
	else val |= ISL_MOD_RESERVED;

	return val;
}

static ssize_t
isl_sensing_range_show(struct device *dev,
		       struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int val;

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);
	val = i2c_smbus_read_byte_data(client, REG_CMD_1);
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);
	val &= ALS_RANGE_HIGH_MASK;
	dev_dbg(dev, "%s: range: 0x%.2x\n", __func__, val);

	if (val < 0)
		return -EINVAL;
	return sprintf(buf, "%d00\n", val ); // 0 or 1 now not old num
}

static ssize_t
ir_current_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int val;

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);
	val = i2c_smbus_read_byte_data(client, REG_CMD_1);
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);

	val &= PROX_DR_MASK;
	dev_dbg(dev, "%s: IR current: 0x%.2x\n", __func__, val);

	if (val < 0)
		return -EINVAL;

	switch (val) {
	case 0:
		val = 110;
		break;
	case 1:
		val = 220;
		break;
	default:
		return -EINVAL;
	}

	if (val)
		val = sprintf(buf, "%d\n", val);
	else
		val = sprintf(buf, "%s\n", "110");
	return val;
}

static ssize_t
isl_sensing_mod_show(struct device *dev,
		     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int val;

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);
	val = isl_get_mod(client);
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);

	dev_dbg(dev, "%s: mod: 0x%.2x\n", __func__, val);

	if (val < 0)
		return val;

	switch (val) {
	case ISL_MOD_POWERDOWN:
		return sprintf(buf, "%s\n", "0-Power-down");
	case ISL_MOD_RESERVED:
		return sprintf(buf, "%s\n", "4-Reserved");
	case ISL_MOD_ALS_CONT:
		return sprintf(buf, "%s\n", "5-ALS continuous");
	case ISL_MOD_PS_CONT:
		return sprintf(buf, "%s\n", "7-Proximity continuous");
	default:
		return -EINVAL;
	}
}

static ssize_t
isl_output_data_show(struct device *dev,
		     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret_val, val, mod;
	unsigned long int output = 0;
	int temp;

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);

	temp = i2c_smbus_read_byte_data(client, REG_DATA_MSB_ALS);
	if (temp < 0)
		goto err_exit;
	ret_val = i2c_smbus_read_byte_data(client, REG_DATA_LSB_ALS);
	if (ret_val < 0)
		goto err_exit;
	ret_val |= temp << 8;

	dev_dbg(dev, "%s: Data: %04x\n", __func__, ret_val);

	mod = isl_get_mod(client);
	switch (mod) {
	case ISL_MOD_ALS_CONT:
	case ISL_MOD_ALS_ONCE:
	case ISL_MOD_IR_ONCE:
	case ISL_MOD_IR_CONT:
		output = ret_val;
		break;
	case ISL_MOD_PS_CONT:
	case ISL_MOD_PS_ONCE:
		val = i2c_smbus_read_byte_data(client, REG_DATA_PROX);
		if (val < 0)
			goto err_exit;
		output = val;
		break;
	default:
		goto err_exit;
	}
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);
	return sprintf(buf, "%ld\n", output);

err_exit:
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);
	return -EINVAL;
}

static ssize_t
isl_sensing_range_store(struct device *dev,
                        struct device_attribute *attr,
                        const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned int ret_val;
	unsigned long val;
	int range;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	switch (val) {
	case 0:
		range = 0;
		break;
	case 1:
		range = ALS_RANGE_HIGH_MASK;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);
	ret_val = isl_set_range(client, range);
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);

	if (ret_val < 0)
		return ret_val;
	return count;
}

static ssize_t
ir_current_store(struct device *dev,
		 struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned int ret_val;
	unsigned long val;

	if (!strncmp(buf, "110", 3))
		val = 0;
        if (!strncmp(buf, "220", 3))
                val = 1;

	else {
		if (strict_strtoul(buf, 10, &val))
			return -EINVAL;
		switch (val) {
		case 110:
			val = 0;
			break;
		case 220:
			val = 1;
			break;
		default:
			return -EINVAL;
		}
	}

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);

	ret_val = i2c_smbus_read_byte_data(client, REG_CMD_1);
	if (ret_val < 0)
		goto err_exit;

	ret_val &= ~PROX_DR_MASK;	/*reset the bit before setting them */
	ret_val |= (val << 3);

	ret_val = i2c_smbus_write_byte_data(client, REG_CMD_1, ret_val);
	if (ret_val < 0)
		goto err_exit;

	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);

	return count;

err_exit:
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);
	return -EINVAL;
}

static ssize_t
isl_sensing_mod_store(struct device *dev,
		      struct device_attribute *attr,
		      const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret_val;
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	if (val > 8)
		return -EINVAL;

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);
	ret_val = isl_set_mod(client, val);
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);

	if (ret_val < 0)
		return ret_val;
	return count;
}

static DEVICE_ATTR(range, S_IRUGO | S_IWUSR,
		   isl_sensing_range_show, isl_sensing_range_store);
static DEVICE_ATTR(mod, S_IRUGO | S_IWUSR,
		   isl_sensing_mod_show, isl_sensing_mod_store);
static DEVICE_ATTR(ir_current, S_IRUGO | S_IWUSR,
		   ir_current_show, ir_current_store);
static DEVICE_ATTR(output, S_IRUGO, isl_output_data_show, NULL);

static struct attribute *mid_att_isl[] = {
	&dev_attr_range.attr,
	&dev_attr_mod.attr,
	&dev_attr_ir_current.attr,
	&dev_attr_output.attr,
	NULL
};

//Register sysfs files
/*
device_create_file (&new_client->dev, &dev_attr_range);
device_create_file (&new_client->dev, &dev_attr_mod);
device_create_file (&new_client->dev, &dev_attr_ir_current);
device_create_file (&new_client->dev, &dev_attr_output);
*/

static struct attribute_group m_isl_gr = {
	.name = "isl29044",
	.attrs = mid_att_isl
};

static int isl_set_default_config(struct i2c_client *client)
{
	int ret=0;
	ret = i2c_smbus_write_byte_data(client, REG_CMD_1, 0x80); // Prox ena
	if (ret < 0)
		return -EINVAL;
 printk(KERN_INFO MODULE_NAME ": %s isl29044 set_default_config call, \n", __func__);

	return 0;
}

/* Return 0 if detection is successful, -ENODEV otherwise */
static int isl29044_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

 printk(KERN_INFO MODULE_NAME ": %s isl29044 detact call, type:%s addr:%x \n", __func__, info->type, info->addr);

//	if (kind <= 0)
	{
		int vendor, device, revision;

		vendor = i2c_smbus_read_word_data(client, ISL29028_REG_VENDOR_REV);
		vendor >>= 8;
		revision = vendor >> ISL29028_REV_SHIFT;
		vendor &= ISL29028_VENDOR_MASK;
		if (vendor != ISL29028_VENDOR)
			return -ENODEV;

		device = i2c_smbus_read_word_data(client, ISL29028_REG_DEVICE);
		device >>= 8;
		if (device != ISL29028_DEVICE)
			return -ENODEV;

		if (revision != ISL29028_REV)
			dev_info(&adapter->dev, "Unknown revision %d\n", revision);
	}
//	else
//		dev_dbg(&adapter->dev, "detection forced\n");

	strlcpy(info->type, "isl29044", I2C_NAME_SIZE);

	return 0;
}

static int
isl29044_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int res=0;
	struct isl29044_dev_data *pdev;
	struct isl29044_platform_data *pdata;

	dev_info(&client->dev, "%s: ISL 028 chip found\n", client->name);

	printk(KERN_INFO MODULE_NAME ": %s isl29044 probe call, ID= %s\n", __func__, id->name);
	res = isl_set_default_config(client);
	//msleep(100);
	if (res < 0) {
		//pr_warn("isl29044: set default config failed!!\n");
		printk(KERN_INFO MODULE_NAME ": %s isl29044 set default config failed\n", __func__);
		return -EINVAL;
	}
	
	res = sysfs_create_group(&client->dev.kobj, &m_isl_gr);
	if (res) {
		//pr_warn("isl29044: device create file failed!!\n");
		printk(KERN_INFO MODULE_NAME ": %s isl29044 device create file failed\n", __func__);
		return -EINVAL;
	}

	pdev = kzalloc(sizeof(struct isl29044_dev_data), GFP_KERNEL);
	if (!pdev) {
		return -ENOMEM;
	}
	i2c_set_clientdata(client, pdev);
	pdev->client = client;

	pdata = (struct isl29044_platform_data *) client->dev.platform_data;
	if(!pdata) {
		printk(KERN_INFO MODULE_NAME "%s: missing platform data info\n", __func__);
		pdev->ps_max_thr = 100;
		pdev->ps_reflect_thr = 200;
		pdev->als_diff = 10;
	}
	else {
		pdev->ps_max_thr = pdata->ps_max_thr;
		pdev->ps_reflect_thr =pdata->ps_reflect_thr;
		pdev->als_diff = pdata->als_diff;
	}
	printk(KERN_INFO MODULE_NAME "%s: als_diff=%d ps_max_thr=%d ps_reflect_thr=%d\n", __func__, pdev->als_diff, pdev->ps_max_thr, pdev->ps_reflect_thr);
	pdev->dev.minor = MISC_DYNAMIC_MINOR;
	pdev->dev.name = MODULE_NAME;
	pdev->dev.fops = &isl29044_fops;
	res = misc_register(&pdev->dev);
	if (res < 0) {
		dev_err(&client->adapter->dev, "ERROR: misc_register returned %d\n", res);
		goto out_free_pdev;
	}
	
	/* initialize the waitq */
	init_waitqueue_head(&pdev->waitq);

	/* initialize the kfifo */
	INIT_KFIFO(pdev->ebuff);

	// create workqueue
	pdev->wq_als = create_singlethread_workqueue("isl29044_wq_als");
	if (!pdev->wq_als) {
		pr_err("isl29044: Failed to create workqueue\n");
		res = -ENOMEM;
		goto out_misc_deregister;
	}
	pdev->wq_ps = create_singlethread_workqueue("isl29044_wq_ps");
	if (!pdev->wq_ps) {
		pr_err("isl29044: Failed to create workqueue\n");
		res = -ENOMEM;
		goto out_destroy_wq_als;
	}
	pdev->wq_cntl = create_singlethread_workqueue("isl29044_wq_cntl");
	if (!pdev->wq_cntl) {
		pr_err("isl29044: Failed to create workqueue\n");
		res = -ENOMEM;
		goto out_destroy_wq_ps;
	}

	// initialize the work struct
	INIT_WORK(&pdev->work_als, isl29044_work_als_func);
	INIT_WORK(&pdev->work_ps, isl29044_work_ps_func);
	pdev->enable_wq_als = pdev->enable_wq_ps = 0;

	// create hrtimer, we don't just do msleep_interruptible as android
	// would pass something in unit of nanoseconds
	hrtimer_init(&pdev->timer_als, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_init(&pdev->timer_ps, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pdev->poll_delay_als = ns_to_ktime(500 * NSEC_PER_MSEC);
	pdev->poll_delay_ps = ns_to_ktime(500 * NSEC_PER_MSEC);
	pdev->timer_als.function = isl29044_timer_als_func;
	pdev->timer_ps.function = isl29044_timer_ps_func;
	pdev->last_als = ALS_SKIP_DATA;

	last_mod = 0;
	isl_set_mod(client, ISL_MOD_POWERDOWN);
	pm_runtime_enable(&client->dev);

	dev_dbg(&client->dev, "isl29044 probe succeed!\n");
	return res;

out_destroy_wq_ps:
	destroy_workqueue(pdev->wq_ps);
out_destroy_wq_als:
	destroy_workqueue(pdev->wq_als);
out_misc_deregister:
	misc_deregister(&pdev->dev);
out_free_pdev:
	kfree(pdev);

	dev_err(&client->dev, "isl29044 probe FAILED with err=%d\n", res);
	return res;
}

static int isl29044_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &m_isl_gr);
	__pm_runtime_disable(&client->dev, false);
 printk(KERN_INFO MODULE_NAME ": %s isl29044 remove call, \n", __func__);
	return 0;
}

static struct i2c_device_id isl29044_id[] = {
	{"isl29044", 0},
	{}
};

static int isl29044_runtime_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	dev_dbg(dev, "suspend\n");
	isl_set_mod(client, ISL_MOD_POWERDOWN);
 printk(KERN_INFO MODULE_NAME ": %s isl29044 suspend call, \n", __func__);
	return 0;
}

static int isl29044_runtime_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	dev_dbg(dev, "resume\n");
	isl_set_mod(client, last_mod);
	msleep(100);
 printk(KERN_INFO MODULE_NAME ": %s isl29044 resume call, \n", __func__);
	return 0;
}

MODULE_DEVICE_TABLE(i2c, isl29044_id);

static const struct dev_pm_ops isl29044_pm_ops = {
	.runtime_suspend = isl29044_runtime_suspend,
	.runtime_resume = isl29044_runtime_resume,
};

static struct i2c_driver isl29044_driver = {
	.driver = {
		   .name = "isl29044",
		   .pm = &isl29044_pm_ops,
		   },
	.probe = isl29044_probe,
	.remove = isl29044_remove,
	.id_table = isl29044_id,
	.detect = isl29044_detect,
	//.address_data   = &addr_data,
};

static int __init sensor_isl29044_init(void)
{
 printk(KERN_INFO MODULE_NAME ": %s isl29044 init call, \n", __func__);
	return i2c_add_driver(&isl29044_driver);
}

static void __exit sensor_isl29044_exit(void)
{
 printk(KERN_INFO MODULE_NAME ": %s isl29044 exit call \n", __func__);
	i2c_del_driver(&isl29044_driver);
}

module_init(sensor_isl29044_init);
module_exit(sensor_isl29044_exit);

MODULE_AUTHOR("mdigioia");
MODULE_ALIAS("isl29044 ALS/PS");
MODULE_DESCRIPTION("Intersil isl29044 ALS/PS Driver");
MODULE_LICENSE("GPL v2");
