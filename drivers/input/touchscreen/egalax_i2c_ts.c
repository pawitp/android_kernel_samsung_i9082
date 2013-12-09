/*****************************************************************************
 * Copyright 2001 - 2013 Broadcom Corporation.  All rights reserved.
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/kfifo.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/i2c/egalax_i2c_ts.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#define MAX_I2C_LEN          10
#define MAX_SUPPORT_POINT    4
#define REPORTID_MOUSE       0x01
#define REPORTID_VENDOR      0x03
#define REPORTID_MTOUCH      0x04
#define MIN_LONG_RESET_TIME  200 /* milliseconds */

#define MAX_X_RES 2047
#define MAX_Y_RES 2047

#ifdef CONFIG_TOUCHSCREEN_SCALE

/*
 * Different boards have different screen resolutions,
 * so if we expect the scale factors in pixel units,
 * we need to know the full screen resolution to
 * compute the scaling. For historical reasons,
 * assume a screen of 1366x768, if it is not
 * specified through the x-full-screen, y-full-screen
 * parameters.
 */
#define X_ORIG 1366
#define Y_ORIG 768
static  int g_x_full = X_ORIG;
static  int g_y_full = Y_ORIG;
static  int g_x_new = X_ORIG;
static  int g_y_new = Y_ORIG;

#define X_ORIG_OFF(x_new)  ((g_x_full - (x_new)) / 2)
#define Y_ORIG_OFF(y_new)  ((g_y_full - (y_new)) / 2)

#define X_OFF(x_new)  ((X_ORIG_OFF(x_new) * 2048) / g_x_full)
#define Y_OFF(y_new)  ((Y_ORIG_OFF(y_new) * 2048) / g_y_full)

#define X_SCALED(x, x_new)						\
	((x_new) ? ((((x) - X_OFF(x_new)) * g_x_full) / (x_new)) : (x))
#define Y_SCALED(y, y_new)						\
	((y_new) ? ((((y) - Y_OFF(y_new)) * g_y_full) / (y_new)) : (y))

#define X_MIN(x_new)  X_OFF(x_new)
#define X_MAX(x_new)  (((X_ORIG_OFF(x_new) + (x_new)) * 2048) / g_x_full)

#define Y_MIN(y_new)  Y_OFF(y_new)
#define Y_MAX(y_new)  (((Y_ORIG_OFF(y_new) + (y_new)) * 2048) / g_y_full)

#endif /* CONFIG_TOUCHSCREEN_SCALE */

#ifdef CONFIG_CP_TS_SOFTKEY
#define BTN_HOME         2
#define BTN_BACKK        1
#define BTN_MENU         3
#define BTN_SEARCH       0
#define MAX_BTN          4
#define BTN_REPORT_MOD   5

static unsigned short BtnState[MAX_BTN];
static struct input_dev *softkey_input_dev;
#endif /* CONFIG_CP_TS_SOFTKEY */

#define MAX_PROC_BUF_SIZE         256
#define PROC_PARENT_DIR           "touchscreen"
#define PROC_ENTRY_DEBUG          "debug"
#define PROC_ENTRY_ROTATE         "rotate"
#define PROC_ENTRY_CMD            "cmd"
#define PROC_ENTRY_WAKEUP         "wakeup"
#define PROC_ENTRY_IDLE_SCAN      "idle_scan"
#define PROC_ENTRY_IDLE_DETECT    "idle_detect"
#define PROC_ENTRY_X_SCALE        "x-scale"
#define PROC_ENTRY_Y_SCALE        "y-scale"
#define PROC_ENTRY_X_FULL_SCREEN  "x-full-screen"
#define PROC_ENTRY_Y_FULL_SCREEN  "y-full-screen"

static const u8 cmd_str_query_firmware[MAX_I2C_LEN] = {
	0x03, 0x03, 0x0A, 0x01, 0x44, 0, 0, 0, 0, 0};
static const u8 cmd_str_query_controller[MAX_I2C_LEN] = {
	0x03, 0x03, 0x0A, 0x01, 0x45, 0, 0, 0, 0, 0};
static const u8 cmd_str_idle[MAX_I2C_LEN] = {
	0x03, 0x06, 0x0A, 0x04, 0x36, 0x3F, 0x01, 0x05, 0, 0};
static const u8 cmd_str_sleep[MAX_I2C_LEN] = {
	0x03, 0x05, 0x0A, 0x03, 0x36, 0x3F, 0x02, 0, 0, 0};
static const u8 wakeup_response[MAX_I2C_LEN] = {
	0x03, 0x05, 0x0A, 0x03, 0x36, 0x3F, 0x01, 0, 0, 0};

/* debug mask to turn on debug prints */
static  int gDbg;

#define PRINT_DEBUG (1U << 0)
#define PRINT_FSM (1U << 1)

#define TS_DEBUG(format, args...)					\
	do {if (gDbg & PRINT_DEBUG) printk(				\
			KERN_WARNING "[egalax_i2c]: " format, ## args); } \
	while (0)

#define TS_FSM(format, args...)						\
	do {if (gDbg & PRINT_FSM) printk(				\
			KERN_WARNING "[egalax_i2c]: " format, ## args); } \
	while (0)

#define DBG()								\
	do {if (gDbg & PRINT_DEBUG) printk(				\
			KERN_WARNING "[%s]:%d =>\n", __func__, __LINE__); } \
	while (0)

#define TS_ERR(fmt, args...)   printk(KERN_ERR "[egalax_i2c]: " fmt, ## args)
#define TS_INFO(fmt, args...)  printk(KERN_INFO "[egalax_i2c]: " fmt, ## args)

/* This is just to share code between TS_DEBUG_PKT() and TS_INFO_PKT(), below */
#define TS_PRINT_PKT(lvl, format, buf, args...)				\
	lvl(format ": 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "	\
		"0x%02x 0x%02x 0x%02x 0x%02x\n",			\
		## args, buf[0], buf[1], buf[2], buf[3], buf[4],	\
		buf[5], buf[6], buf[7], buf[8], buf[9]);

#define TS_DEBUG_PKT(fmt, buf, args...)			\
	TS_PRINT_PKT(TS_DEBUG, fmt, buf, ## args);
#define TS_INFO_PKT(fmt, buf, args...) TS_PRINT_PKT(TS_INFO, fmt, buf, ## args);

struct proc_dir {
	struct proc_dir_entry *parent;
};

struct point_data {
	short Status;
	short X;
	short Y;
};

enum rotation_angle {
	ROTATE_NONE,
	ROTATE_CLOCKWISE_90,
	ROTATE_CLOCKWISE_180,
	ROTATE_CLOCKWISE_270,
	ROTATE_MAX,
};

static unsigned int rotate_string[ROTATE_MAX] = {
	0, 90, 180, 270
};

struct rotation_ctrl {
	enum rotation_angle rotate;
	int flip_x;
	int flip_y;
	int swap_xy;
};

struct _egalax_i2c {
	struct workqueue_struct *ktouch_wq;
	struct work_struct work;
	struct mutex mutex_wq;
	struct i2c_client *client;
	struct egalax_i2c_ts_cfg hw_cfg;
	struct rotation_ctrl rotate_ctrl;
	struct proc_dir proc;
	struct regulator *ts_regulator;
};

static void egalax_i2c_reset_wq_func(struct work_struct *work);
static void egalax_idle_detect_wq_func(struct work_struct *work);
static void wakeup_controller(void);

static struct input_dev *input_dev;
static struct _egalax_i2c *p_egalax_i2c_dev;
static struct point_data PointBuf[MAX_SUPPORT_POINT];

static DECLARE_WORK(shared_work, egalax_i2c_reset_wq_func);

/* default value for idle detect period in ms */
#define IDLE_DETECT_PERIOD 500

/* default value for idle scan interval - T. Time in msec is (T+1)*50ms */
#define IDLE_SCAN_INTERVAL 0

static int g_idle_scan_interval = IDLE_SCAN_INTERVAL;
static int g_idle_detect_period = IDLE_DETECT_PERIOD;

/* TS controller states */
enum {
	ACTIVE_STATE,
	IDLE_STATE,
	READY_TO_SLEEP_STATE,
	SUSPEND_WAIT_FOR_WAKEUP_STATE,
	RESUME_WAIT_FOR_WAKEUP_STATE,
	SUSPENDED_STATE,
	WAKEUP_PROBLEM_STATE,
};

/* FSM events */
enum {
	IDLE_DETECT_EVENT,
	WAKEUP_RESPONSE_EVENT,
	RESUME_WAKEUP_FAILED_EVENT,
	RESUME_EVENT,
	SUSPEND_EVENT,
	SLEEP_CMD_FAILED_EVENT,
	SLEEP_CMD_SUCCESS_EVENT,
};

/* TS controller state */
static int g_state;

/* flag indicating that TS was touched */
static int g_touched;

/* idle detect wq function */
static DECLARE_DELAYED_WORK(idle_detect_work, egalax_idle_detect_wq_func);

/*
 * A long reset time means the probe has to finish before it is known
 * whether or not there is a egalax touch screen controller.
 */
static int g_is_hw_detected;

static int egalax_probe_part2(struct _egalax_i2c *p_egalax_dev);
static int egalax_i2c_measure(struct i2c_client *client);

/* send idle command to TS controller */
static int send_idle(void)
{
	struct i2c_client *client = p_egalax_i2c_dev->client;
	int rc;
	u8 cmdbuf[MAX_I2C_LEN];

	/* set idle scan interval in the idle command */
	memcpy(cmdbuf, cmd_str_idle, MAX_I2C_LEN);
	cmdbuf[7] = g_idle_scan_interval;

	/* send i2c command to put controller into idle */
	rc = i2c_master_send(client, cmdbuf, MAX_I2C_LEN);
	if (rc == MAX_I2C_LEN)
		return true;
	else
		return false;
}

/* send sleep command to TS controller */
static int send_sleep(void)
{
	struct i2c_client *client = p_egalax_i2c_dev->client;
	int rc;

	rc = i2c_master_send(client, cmd_str_sleep, MAX_I2C_LEN);
	if (rc == MAX_I2C_LEN)
		return true;
	else
		return false;
}

static char *state_name(int state)
{
	switch (state) {
	case ACTIVE_STATE:
		return "ACTIVE";
	case IDLE_STATE:
		return "IDLE";
	case READY_TO_SLEEP_STATE:
		return "READY_TO_SLEEP";
	case SUSPEND_WAIT_FOR_WAKEUP_STATE:
		return "SUSPEND_WAIT_FOR_WAKEUP";
	case RESUME_WAIT_FOR_WAKEUP_STATE:
		return "RESUME_WAIT_FOR_WAKEUP";
	case SUSPENDED_STATE:
		return "SUSPENDED";
	case WAKEUP_PROBLEM_STATE:
		return "WAKEUP_PROBLEM";
	default:
		return "NULL";
	}
}

static char *event_name(int event)
{
	switch (event) {
	case IDLE_DETECT_EVENT:
		return "IDLE_DETECT";
	case WAKEUP_RESPONSE_EVENT:
		return "WAKEUP_RESPONSE";
	case RESUME_WAKEUP_FAILED_EVENT:
		return "RESUME_WAKEUP_FAILED";
	case RESUME_EVENT:
		return "RESUME";
	case SUSPEND_EVENT:
		return "SUSPEND";
	case SLEEP_CMD_FAILED_EVENT:
		return "SLEEP_CMD_FAILED";
	case SLEEP_CMD_SUCCESS_EVENT:
		return "SLEEP_CMD_SUCCESS";
	default:
		return "NULL";
	}
}

/* state functions. executed when entering the state */

/* When executed:
 * - initially
 * - after being in sleep state upon wakeup response (IRQ thread)
 * - after being in idle state upon wakeup response (IRQ thread)
 */
static void active(void)
{
	/* if idle detect enabled */
	if (g_idle_detect_period > 0)
		schedule_delayed_work(&idle_detect_work,
				msecs_to_jiffies(g_idle_detect_period));
	g_state = ACTIVE_STATE;
}

/* When executed:
 * - in resume thread if no wake up or bogus response received (resume thread)
 * - idle command failed (idle detection thread)
 */
static void wakeup_problem(void)
{
	/* if idle detect enabled */
	if (g_idle_detect_period > 0)
		schedule_delayed_work(&idle_detect_work,
				msecs_to_jiffies(g_idle_detect_period));
	g_state = WAKEUP_PROBLEM_STATE;
}

/* When executed:
 * - idle detected (idle detect thread)
 */
static void idle(void)
{
	if (g_touched) {
		g_touched = 0;
		/* schedule next idle detect work */
		schedule_delayed_work(&idle_detect_work,
				msecs_to_jiffies(g_idle_detect_period));
	} else {
		if (send_idle()) {
			/* no idle detection while we are idle, so we will not
			 * schedule idle_detect_work */
			g_state = IDLE_STATE;
		} else {
			TS_ERR("%s: failed to send idle command\n", __func__);

			/* observing failures after resuming. Could be that
			   wakeup_controller did not do its job, so repeat it
			   here */
			wakeup_controller();

			/* schedule idle detect. Hoping that next time idle
			 * command will get through */
			schedule_delayed_work(&idle_detect_work,
					msecs_to_jiffies(
						g_idle_detect_period));

			g_state = WAKEUP_PROBLEM_STATE;
		}
	}
}

/* When executed:
 * - suspend while active (suspend thread)
 * - received wakeup response if idle when suspend was requested (IRQ thread)
 */
static void ready_to_sleep(void)
{
	/* signal to suspend thread that wake up response was received */
	g_state = READY_TO_SLEEP_STATE;
}

/* When executed:
 * - sleep command was successful (suspend thread)
 */
static void suspended(void)
{
	g_state = SUSPENDED_STATE;
}

/* When executed:
 * - were in idle state when suspend was requested (suspend thread)
 */
static void suspend_wait_for_wakeup(void)
{
	wakeup_controller();
	g_state = SUSPEND_WAIT_FOR_WAKEUP_STATE;
}

/* When executed:
 * - returning from sleep state (resume thread)
 */
static void resume_wait_for_wakeup(void)
{
	enable_irq(p_egalax_i2c_dev->client->irq);
	wakeup_controller();
	g_state = RESUME_WAIT_FOR_WAKEUP_STATE;
}

/* When executed:
 * - resuming after suspend, when sleep command failed in suspend path (resume
 * thread)
 */
static void resume_after_sleep_failure(void)
{
	enable_irq(p_egalax_i2c_dev->client->irq);
	wakeup_controller();
	active();
}

/* FSM transition: takes event and returns new state. Note that this function
 * is called from different threads:
 - IRQ thread
 - Suspend thread
 - Resume thread
 - Idle detect thread
 and must be protected by the mutex */
static int state_transition(int event)
{
	TS_FSM("%s: state - %s event - %s\n",
		__func__, state_name(g_state), event_name(event));

	mutex_lock(&p_egalax_i2c_dev->mutex_wq);

	switch (g_state) {

		/* TS controller is active */
	case ACTIVE_STATE:
		switch (event) {
		case IDLE_DETECT_EVENT:
			idle();
			break;

		case SUSPEND_EVENT:
			ready_to_sleep();
			break;

		case WAKEUP_RESPONSE_EVENT:
		case RESUME_EVENT:
		case SLEEP_CMD_FAILED_EVENT:
		case SLEEP_CMD_SUCCESS_EVENT:
		case RESUME_WAKEUP_FAILED_EVENT:
			TS_ERR("%s: should not happen. event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
			break;

		default:
			TS_ERR("%s: unknown event - %s in state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
		}
		break;

		/* TS controller is idle - intermediary power saving state */
	case IDLE_STATE:
		switch (event) {

		case SUSPEND_EVENT:
			suspend_wait_for_wakeup();
			break;

		case WAKEUP_RESPONSE_EVENT:
			active();
			break;

		case RESUME_EVENT:
		case IDLE_DETECT_EVENT:
		case SLEEP_CMD_FAILED_EVENT:
		case SLEEP_CMD_SUCCESS_EVENT:
		case RESUME_WAKEUP_FAILED_EVENT:
			TS_ERR("%s: should not happen. event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
			break;

		default:
			TS_ERR("%s: unknown event - %s in state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
		}
		break;

		/* waiting for "after idle wakeup response" after suspend was
		   requested */
	case SUSPEND_WAIT_FOR_WAKEUP_STATE:
		switch (event) {

		case WAKEUP_RESPONSE_EVENT:
			ready_to_sleep();
			break;


		case RESUME_EVENT:
			/* if we received resume in this state, we never
			   received wakeup response after "suspending while
			   idle". We do not know exactly what state TS
			   controller is and can not expect wakeup response */
			active();
			break;

		case IDLE_DETECT_EVENT:
		case SUSPEND_EVENT:
		case SLEEP_CMD_FAILED_EVENT:
		case SLEEP_CMD_SUCCESS_EVENT:
		case RESUME_WAKEUP_FAILED_EVENT:
			TS_ERR("%s: should not happen. event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
			break;

		default:
			TS_ERR("%s: unknown event - %s in state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
		}
		break;

		/* TS controller is ready to issue sleep command */
	case READY_TO_SLEEP_STATE:
		switch (event) {

		case SLEEP_CMD_FAILED_EVENT:
			/* staying in the same state */
			break;

		case SLEEP_CMD_SUCCESS_EVENT:
			suspended();
			break;

		case RESUME_EVENT:
			/* if we received resume in this state, sleep command
			   failed in suspension processing. We do not know
			   exactly what state TS controller is and can not
			   expect wakeup response */
			resume_after_sleep_failure();
			break;

		case IDLE_DETECT_EVENT:
		case WAKEUP_RESPONSE_EVENT:
		case SUSPEND_EVENT:
		case RESUME_WAKEUP_FAILED_EVENT:
			TS_ERR("%s: should not happen. event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
			break;

		default:
			TS_ERR("%s: unknown event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
		}
		break;

		/* TS controller is sleeping - lowest power saving state */
	case SUSPENDED_STATE:
		switch (event) {

		case RESUME_EVENT:
			resume_wait_for_wakeup();
			break;

		case IDLE_DETECT_EVENT:
		case WAKEUP_RESPONSE_EVENT:
		case SUSPEND_EVENT:
		case SLEEP_CMD_FAILED_EVENT:
		case SLEEP_CMD_SUCCESS_EVENT:
		case RESUME_WAKEUP_FAILED_EVENT:
			TS_ERR("%s: should not happen. event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
			break;

		default:
			TS_ERR("%s: unknown event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
		}
		break;


		/* waiting for "after sleep wakeup response" after resume was
		   requested */
	case RESUME_WAIT_FOR_WAKEUP_STATE:
		switch (event) {

		case WAKEUP_RESPONSE_EVENT:
			/* we are woken up after sleep */
			active();
			break;

		case RESUME_WAKEUP_FAILED_EVENT:
			/* we did not receive wake up response or it was a
			 * bogus one */
			wakeup_problem();
			break;

		case IDLE_DETECT_EVENT:
		case RESUME_EVENT:
		case SUSPEND_EVENT:
		case SLEEP_CMD_FAILED_EVENT:
		case SLEEP_CMD_SUCCESS_EVENT:
			TS_ERR("%s: should not happen. event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
			break;

		default:
			TS_ERR("%s: unknown event - %s in state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
		}
		break;

	case WAKEUP_PROBLEM_STATE:
		switch (event) {

		case WAKEUP_RESPONSE_EVENT:
			/* we have recovered after wake up problem */
			TS_FSM("%s: recovered after wake up problem\n",
				__func__);
			active();
			break;


		case IDLE_DETECT_EVENT:
			/* this is normal as we are relying on idle detect
			   mechanism to wake up controller */
			idle();
			break;

		case RESUME_WAKEUP_FAILED_EVENT:
		case RESUME_EVENT:
		case SUSPEND_EVENT:
		case SLEEP_CMD_FAILED_EVENT:
		case SLEEP_CMD_SUCCESS_EVENT:
			TS_ERR("%s: should not happen. event - %s state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
			break;

		default:
			TS_ERR("%s: unknown event - %s in state - %s\n",
				__func__, event_name(event),
				state_name(g_state));
		}
		break;

	default:
		TS_ERR("%s: unknown state - %s\n", __func__,
			state_name(g_state));
	}

	TS_FSM("%s: new state - %s\n", __func__, state_name(g_state));
	mutex_unlock(&p_egalax_i2c_dev->mutex_wq);

	return g_state;
}

static void wakeup_controller(void)
{
	int rc;
	int event_gpio = p_egalax_i2c_dev->hw_cfg.gpio.event;

	/* change event PIN direction to output and set it to low */
	rc = gpio_direction_output(event_gpio, 0);
	if (rc < 0) {
		TS_ERR("%s: failed to set event_gpio direction to output\n",
			__func__);
		return;
	}

	/* change event PIN direction back to input */
	rc = gpio_direction_input(event_gpio);
	if (rc < 0) {
		TS_ERR("%s: failed to set event_gpio direction back to input\n",
			__func__);
		return;
	}
}

#ifdef CONFIG_CP_TS_SOFTKEY
static void report_softkey(int status, int btn_id)
{
	int keyValue = 0;

	switch (btn_id) {
	case BTN_HOME:
		keyValue = KEY_HOME;
		break;
	case BTN_BACKK:
		keyValue = KEY_BACK;
		break;
	case BTN_MENU:
		keyValue = KEY_MENU;
		break;
	case BTN_SEARCH:
		keyValue = KEY_SEARCH;
		break;
	default:
		return;
	}
	TS_DEBUG("report soft button key:0x%02x status:%d\n", keyValue, status);
	input_report_key(softkey_input_dev, keyValue, status);
	input_sync(softkey_input_dev);
}
#endif

static int LastUpdateID;
static void ProcessReport(unsigned char *buf, int buflen)
{
	int i;
	short X = 0, Y = 0, ContactID = 0, Status = 0;
	struct rotation_ctrl *rotate_ctrl = &p_egalax_i2c_dev->rotate_ctrl;

	if (buflen != MAX_I2C_LEN || buf[0] != REPORTID_MTOUCH) {
		TS_ERR("I2C incorrect buflen=%d\n", buflen);
		return;
	}

	Status = buf[1] & 0x01;
	ContactID = (buf[1] & 0x7C) >> 2;
	X = ((buf[3] << 8) + buf[2]) >> 4;
	Y = ((buf[5] << 8) + buf[4]) >> 4;

	/* TODO: error if contactID > MAX_SUPPORT_POINT */
	if (ContactID >= MAX_SUPPORT_POINT) {
		TS_ERR("Invalid contact id = %d\n", ContactID);
		return;
	}

	if (rotate_ctrl->rotate > 0 && rotate_ctrl->rotate < ROTATE_MAX) {
		if (rotate_ctrl->flip_x)
			X = MAX_X_RES - X;

		if (rotate_ctrl->flip_y)
			Y = MAX_Y_RES - Y;

		if (rotate_ctrl->swap_xy) {
			short tmp;

			tmp = X;
			X = Y;
			Y = tmp;
		}
	}

#ifdef CONFIG_TOUCHSCREEN_SCALE
	if (!((g_x_new == g_x_full) && (g_y_new == g_y_full)) &&
		!((g_x_new == 0) && (g_y_new == 0))) {
		if (X < X_MIN(g_x_new) || X > X_MAX(g_x_new) ||
			Y < Y_MIN(g_y_new) || Y > Y_MAX(g_y_new))
			return;
		X = X_SCALED(X, g_x_new);
		Y = Y_SCALED(Y, g_y_new);
	}
#endif

	PointBuf[ContactID].Status = Status;
	PointBuf[ContactID].X = X;
	PointBuf[ContactID].Y = Y;

	TS_DEBUG("got touch point[%d]: status=%d X=%d Y=%d\n",
		ContactID, Status, X, Y);

	if (!Status || (ContactID <= LastUpdateID)) {
		for (i = 0; i < MAX_SUPPORT_POINT; i++) {
			if (PointBuf[i].Status > 0) {
				input_report_abs(input_dev,
						ABS_MT_TRACKING_ID, i);
				input_report_abs(input_dev,
						ABS_MT_TOUCH_MAJOR,
						PointBuf[i].Status);
				input_report_abs(input_dev,
						ABS_MT_WIDTH_MAJOR, 0);
				input_report_abs(input_dev,
						ABS_MT_POSITION_X,
						PointBuf[i].X);
				input_report_abs(input_dev,
						ABS_MT_POSITION_Y,
						PointBuf[i].Y);
				TS_DEBUG("input sync point data [%d]!\n", i);
			}
			input_mt_sync(input_dev);

			if (PointBuf[i].Status == 0)
				PointBuf[i].Status--;
		}
		input_sync(input_dev);
		TS_DEBUG("input sync point data done!\n");
	}

	LastUpdateID = ContactID;

	/* set flags indicating that TS was touched */
	mutex_lock(&p_egalax_i2c_dev->mutex_wq);
	g_touched = 1;
	mutex_unlock(&p_egalax_i2c_dev->mutex_wq);
}

static int egalax_i2c_measure(struct i2c_client *client)
{
	u8 x_buf[MAX_I2C_LEN];
	int i, count, wakeup_response_received, loop = 3;
#ifdef CONFIG_CP_TS_SOFTKEY
	int btn_id, btn_state;
#endif

	DBG();

	do {
		count = i2c_master_recv(client, x_buf, MAX_I2C_LEN);
	} while (count == -EAGAIN && --loop);

	if (count < 0 || (x_buf[0] != REPORTID_VENDOR &&
				x_buf[0] != REPORTID_MTOUCH)) {
		TS_ERR("%s: received bogus data. count - %d, x_buf[0]: %d\n",
			__func__, count, x_buf[0]);

		/*
		 * TODO: Ignore invalid data for now w/o printing it to the
		 * console until this gets sorted out with EETI/Compal
		 */
		return -EFAULT;
	}

	TS_DEBUG("read data with len=%d\n", count);

	if (x_buf[0] == REPORTID_VENDOR) {
		TS_DEBUG_PKT("received command packet", x_buf);

		/* see if this is a wake up response - sent after TS controller
		 * becomes active after being in idle or sleep stte */
		for (wakeup_response_received = true, i = 0;
		     i < MAX_I2C_LEN; i++) {
			if (x_buf[i] != wakeup_response[i]) {
				wakeup_response_received = false;
				break;
			}
		}

		if (wakeup_response_received) {
			TS_DEBUG("%s: wake up response received\n", __func__);
			state_transition(WAKEUP_RESPONSE_EVENT);
		}

		/* if this is response to FW version or controller name */
		if (x_buf[4] == 0x44) {
			TS_INFO("FW version: %c%c%c%c%c\n",
				x_buf[5], x_buf[6], x_buf[7],
				x_buf[8], x_buf[9]);
		}

		if (x_buf[4] == 0x45) {
			TS_INFO("TS controller name: %c%c%c%c%c\n",
				x_buf[5], x_buf[6], x_buf[7], x_buf[8],
				x_buf[9]);
		}
	}

	if (count > 0 && x_buf[0] == REPORTID_MTOUCH) {
		ProcessReport(x_buf, count);
		return count;
	}

#ifdef CONFIG_CP_TS_SOFTKEY
	if (count > 0 && x_buf[0] == REPORTID_VENDOR &&
		x_buf[1] == 6 && x_buf[4] == 0x36 && x_buf[5] == 0x2F) {
		TS_DEBUG("got virtual_key report: id=%d state=%d\n",
			x_buf[7], x_buf[6]);
		btn_state = x_buf[6] & 0x01;
		btn_id = x_buf[7];
		if (btn_id >= 0 && btn_id < MAX_BTN) {
			if (btn_state == 0 && BtnState[btn_id]) {
				report_softkey(btn_state, btn_id);
				memset(BtnState, 0, sizeof(short)*MAX_BTN);
			} else if (btn_state) {
				if ((++BtnState[btn_id]) % BTN_REPORT_MOD == 0)
					report_softkey(btn_state, btn_id);
			}
		}
		return count;
	}
#endif

	return count;
}

static void egalax_i2c_wq(struct work_struct *work)
{
	struct _egalax_i2c *egalax_i2c = container_of(
		work, struct _egalax_i2c, work);
	struct i2c_client *client = egalax_i2c->client;
	int gpio = irq_to_gpio(client->irq);

	TS_DEBUG("egalax_i2c_wq run\n");

	/* continue recv data while GPIO is pulled low */
	while (!gpio_get_value(gpio)) {
		egalax_i2c_measure(client);
		schedule();
	}

	TS_DEBUG("egalax_i2c_wq leave\n");
}

static void egalax_i2c_reset_wq_func(struct work_struct *work)
{
	int rc;

	if (p_egalax_i2c_dev == NULL) {
		TS_ERR("%s() egalax_i2c == NULL\n", __func__);
		return;
	}

	if (p_egalax_i2c_dev->hw_cfg.reset_time > MIN_LONG_RESET_TIME)
		msleep(p_egalax_i2c_dev->hw_cfg.reset_time);
	rc = egalax_probe_part2(p_egalax_i2c_dev);

	if (rc != 0)
		printk(KERN_INFO "%s() egalax_probe_part2() returned %d\n",
			__func__, rc);
}

static irqreturn_t egalax_i2c_interrupt(int irq, void *dev_id)
{
	struct _egalax_i2c *egalax_i2c = (struct _egalax_i2c *)dev_id;

	TS_DEBUG("egalax_i2c_interrupt with irq:%d\n", irq);

	/* postpone I2C transactions to the workqueue as it may block */
	queue_work(egalax_i2c->ktouch_wq, &egalax_i2c->work);

	return IRQ_HANDLED;
}

static int
proc_debug_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int rc;
	unsigned int debug;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &debug) != 1) {
		TS_ERR("echo <debug> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_DEBUG);
		return count;
	}

	gDbg = debug;

	return count;
}

static int
proc_rotate_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;
	struct _egalax_i2c *egalax_i2c = (struct _egalax_i2c *)data;
	struct rotation_ctrl *ctrl = &egalax_i2c->rotate_ctrl;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len, "Current screen rotation configuration "
		"is %u degree clockwise\n",
		rotate_string[ctrl->rotate]);

	return len;
}

static int
proc_rotate_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int rc;
	unsigned int rotate;
	struct _egalax_i2c *egalax_i2c = (struct _egalax_i2c *)data;
	struct rotation_ctrl *ctrl = &egalax_i2c->rotate_ctrl;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &rotate) != 1) {
		TS_ERR("echo <rotate> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_ROTATE);
		return count;
	}

	if (rotate >= ROTATE_MAX) {
		TS_ERR("Rotation index invalid. "
			"Use a value between 0 ~ %d\n", ROTATE_MAX - 1);
		return count;
	}

	switch (rotate) {
	case ROTATE_NONE:
		ctrl->flip_x = 0;
		ctrl->flip_y = 0;
		ctrl->swap_xy = 0;
		ctrl->rotate = ROTATE_NONE;
		break;

	case ROTATE_CLOCKWISE_90:
		ctrl->flip_x = 1;
		ctrl->flip_y = 0;
		ctrl->swap_xy = 1;
		ctrl->rotate = ROTATE_CLOCKWISE_90;
		break;

	case ROTATE_CLOCKWISE_180:
		ctrl->flip_x = 1;
		ctrl->flip_y = 1;
		ctrl->swap_xy = 0;
		ctrl->rotate = ROTATE_CLOCKWISE_180;
		break;

	case ROTATE_CLOCKWISE_270:
		ctrl->flip_x = 0;
		ctrl->flip_y = 1;
		ctrl->swap_xy = 1;
		ctrl->rotate = ROTATE_CLOCKWISE_270;
		break;

	default:
		break;
	}

	TS_INFO("New rotation configuration is %u degree "
		"clockwise flip_x=%d flip_y=%d swap_xy=%d\n",
		rotate_string[ctrl->rotate], ctrl->flip_x, ctrl->flip_y,
		ctrl->swap_xy);

	return count;
}

static int
proc_debug_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len, "%d\n", gDbg);

	return len;
}

static int
proc_cmd_write(struct file *file, const char __user *buffer,
	unsigned long count, void *data)
{
	int rc;
	unsigned int cmd, idle_interval;
	u8 cmdbuf[MAX_I2C_LEN];
	unsigned char kbuf[MAX_PROC_BUF_SIZE];
	unsigned char cmd_str[3][10] = { "query", "idle", "sleep" };

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d\n", rc);
		return -EFAULT;
	}

	rc = sscanf(kbuf, "%u %u", &cmd, &idle_interval);
	if (rc != 1 && rc != 2) {
		TS_ERR("echo <0=Query, 1=Idle, 2=Sleep> "
			"<idle scan interval> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_CMD);
		return count;
	}

	switch (cmd) {
	case 0:
		memcpy(cmdbuf, cmd_str_query_firmware, MAX_I2C_LEN);
		break;

	case 1:
		memcpy(cmdbuf, cmd_str_idle, MAX_I2C_LEN);
		if (rc == 2 && idle_interval < 10)
			cmdbuf[7] = idle_interval;
		else
			cmdbuf[7] = g_idle_scan_interval;
		TS_INFO("idle scan interval is %u ms\n", (cmdbuf[7] + 1) * 50);
		break;

	case 2:
		memcpy(cmdbuf, cmd_str_sleep, MAX_I2C_LEN);
		break;

	default:
		TS_ERR("echo <0=Query, 1=Idle, 2=Sleep> "
			"<idle scan interval> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_CMD);
		return count;
	}

	/* TS controller may be in IDLE state. In this case send will fail. We
	   need to wake up controller and disable idle detection. Note that
	   "idle detection" will remain disabled until TS controller is woken up
	   by writing to "wake" proc entry or suspending/resuming */
	if (g_idle_detect_period > 0) {
		cancel_delayed_work_sync(&idle_detect_work);
		wakeup_controller();
	}

	/* send command to TS controller. Note that the response will be
	 * received in the interrupt handler */
	rc = i2c_master_send(p_egalax_i2c_dev->client, cmdbuf, MAX_I2C_LEN);
	if (rc != MAX_I2C_LEN) {
		TS_ERR("Unable to send [%s] command\n", cmd_str[cmd]);
		return rc;
	}

	TS_DEBUG_PKT("Sent [%s] command. Command string", cmdbuf, cmd_str[cmd]);

	/* if query */
	if (cmd == 0) {
		/* now query controller name */
		/* wait for 500 msec to let the previous command to settle */
		msleep(500);
		rc = i2c_master_send(p_egalax_i2c_dev->client,
				cmd_str_query_controller, MAX_I2C_LEN);
		if (rc != MAX_I2C_LEN) {
			TS_ERR("Unable to send query controller command\n");
			return rc;
		}

		TS_DEBUG_PKT("Sent [query controller] command. Command string",
			cmdbuf);
		return rc;
	}
	return count;
}

static int
proc_wakeup_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int rc;
	unsigned int wakeup;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &wakeup) != 1) {
		TS_ERR("echo <wakeup> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_WAKEUP);
		return count;
	}

	if (wakeup)
		wakeup_controller();

	/* set state to ACTIVE */
	active();

	return count;
}

#ifdef CONFIG_TOUCHSCREEN_SCALE
static int
proc_x_scale_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int rc;
	unsigned int x_scale;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &x_scale) != 1) {
		TS_ERR("echo <x-scale> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_X_SCALE);
		return count;
	}

	g_x_new = x_scale;

	return count;
}

static int
proc_x_scale_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len,
		"x-axis %sscaled (scale = %d/%d)\n",
		((g_x_new == 0 || g_x_new == g_x_full) ? "non" : ""),
		g_x_new, g_x_full);

	return len;
}

static int
proc_y_scale_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int rc;
	unsigned int y_scale;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &y_scale) != 1) {
		TS_ERR("echo <y-scale> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_Y_SCALE);
		return count;
	}

	g_y_new = y_scale;

	return count;
}

static int
proc_y_scale_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len,
		"y-axis %sscaled (scale = %d/%d)\n",
		((g_y_new == 0 || g_y_new == g_y_full) ? "non" : ""),
		g_y_new, g_y_full);

	return len;
}

static int
proc_x_full_screen_write(struct file *file, const char __user *buffer,
			unsigned long count, void *data)
{
	int rc;
	unsigned int x_full_screen;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &x_full_screen) != 1) {
		TS_ERR("echo <x-full-screen> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_X_FULL_SCREEN);
		return count;
	}

	g_x_full = x_full_screen;

	return count;
}

static int
proc_x_full_screen_read(char *buffer, char **start, off_t off, int count,
			int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len, "%d\n", g_x_full);

	return len;
}

static int
proc_y_full_screen_write(struct file *file, const char __user *buffer,
			unsigned long count, void *data)
{
	int rc;
	unsigned int y_full_screen;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &y_full_screen) != 1) {
		TS_ERR("echo <y-full-screen> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_Y_FULL_SCREEN);
		return count;
	}

	g_y_full = y_full_screen;

	return count;
}

static int
proc_y_full_screen_read(char *buffer, char **start, off_t off, int count,
			int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len, "%d\n", g_y_full);

	return len;
}
#endif
static int
proc_idle_scan_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len, "%d\n", g_idle_scan_interval);

	return len;
}

static int
proc_idle_scan_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int rc;
	unsigned int idle_scan;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &idle_scan) != 1) {
		TS_ERR("echo <idle_scan> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_IDLE_SCAN);
		return count;
	}

	/* valid values for idle scan 0-9 */
	if (idle_scan > 9) {
		TS_ERR("idle scan valid values are 0-9. Enered value is %u\n",
			idle_scan);
		return count;
	}

	TS_INFO("idle scan interval is %u ms\n",
		(idle_scan + 1) * 50);

	g_idle_scan_interval = idle_scan;

	return count;
}

static int
proc_idle_detect_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len, "%d\n", g_idle_detect_period);

	return len;
}

static int
proc_idle_detect_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int rc;
	unsigned int idle_detect;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	rc = copy_from_user(kbuf, buffer, count);
	if (rc) {
		TS_ERR("copy_from_user failed status=%d", rc);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%u", &idle_detect) != 1) {
		TS_ERR("echo <idle_detect> > /proc/%s/%s\n",
			PROC_PARENT_DIR, PROC_ENTRY_IDLE_DETECT);
		return count;
	}

	TS_INFO("idle detect period is %u ms\n", idle_detect);

	g_idle_detect_period = idle_detect;

	/* make sure that the new value is used from now on */
	cancel_delayed_work_sync(&idle_detect_work);

	/* set state to ACTIVE */
	active();

	return count;
}

static int proc_init(struct _egalax_i2c *egalax_i2c)
{
	struct proc_dir *proc = &egalax_i2c->proc;
	int rc;
	struct proc_dir_entry *proc_debug, *proc_rotate, *proc_cmd,
		*proc_wakeup;
	struct proc_dir_entry *proc_idle_scan, *proc_idle_detect;
#ifdef CONFIG_TOUCHSCREEN_SCALE
	struct proc_dir_entry *proc_x_scale, *proc_y_scale;
	struct proc_dir_entry *proc_x_full_screen, *proc_y_full_screen;
#endif

	proc->parent = proc_mkdir(PROC_PARENT_DIR, NULL);

	proc_debug = create_proc_entry(PROC_ENTRY_DEBUG, 0644, proc->parent);
	if (proc_debug == NULL) {
		rc = -ENOMEM;
		goto err_del_parent;
	}
	proc_debug->read_proc = proc_debug_read;
	proc_debug->write_proc = proc_debug_write;
	proc_debug->data = NULL;

	proc_rotate = create_proc_entry(PROC_ENTRY_ROTATE, 0644, proc->parent);
	if (proc_rotate == NULL) {
		rc = -ENOMEM;
		goto err_del_debug;
	}
	proc_rotate->read_proc = proc_rotate_read;
	proc_rotate->write_proc = proc_rotate_write;
	proc_rotate->data = egalax_i2c;

	proc_cmd = create_proc_entry(PROC_ENTRY_CMD, 0644, proc->parent);
	if (proc_cmd == NULL) {
		rc = -ENOMEM;
		goto err_del_rotate;
	}
	proc_cmd->read_proc = NULL;
	proc_cmd->write_proc = proc_cmd_write;
	proc_cmd->data = NULL;

	proc_wakeup = create_proc_entry(PROC_ENTRY_WAKEUP, 0644, proc->parent);
	if (proc_wakeup == NULL) {
		rc = -ENOMEM;
		goto err_del_cmd;
	}
	proc_wakeup->read_proc = NULL;
	proc_wakeup->write_proc = proc_wakeup_write;
	proc_wakeup->data = NULL;

	proc_idle_scan = create_proc_entry(PROC_ENTRY_IDLE_SCAN, 0644,
					proc->parent);
	if (proc_idle_scan == NULL) {
		rc = -ENOMEM;
		goto err_del_debug;
	}
	proc_idle_scan->read_proc = proc_idle_scan_read;
	proc_idle_scan->write_proc = proc_idle_scan_write;
	proc_idle_scan->data = NULL;

	proc_idle_detect = create_proc_entry(
		PROC_ENTRY_IDLE_DETECT, 0644, proc->parent);
	if (proc_idle_detect == NULL) {
		rc = -ENOMEM;
		goto err_del_debug;
	}
	proc_idle_detect->read_proc = proc_idle_detect_read;
	proc_idle_detect->write_proc = proc_idle_detect_write;
	proc_idle_detect->data = NULL;


#ifdef CONFIG_TOUCHSCREEN_SCALE
	proc_x_scale = create_proc_entry(PROC_ENTRY_X_SCALE, 0644,
					proc->parent);
	if (proc_x_scale == NULL) {
		rc = -ENOMEM;
		goto err_del_wakeup;
	}
	proc_x_scale->read_proc = proc_x_scale_read;
	proc_x_scale->write_proc = proc_x_scale_write;
	proc_x_scale->data = NULL;

	proc_y_scale = create_proc_entry(PROC_ENTRY_Y_SCALE, 0644,
					proc->parent);
	if (proc_y_scale == NULL) {
		rc = -ENOMEM;
		goto err_del_x_scale;
	}
	proc_y_scale->read_proc = proc_y_scale_read;
	proc_y_scale->write_proc = proc_y_scale_write;
	proc_y_scale->data = NULL;

	proc_x_full_screen = create_proc_entry(PROC_ENTRY_X_FULL_SCREEN, 0644,
					proc->parent);
	if (proc_x_full_screen == NULL) {
		rc = -ENOMEM;
		goto err_del_y_scale;
	}
	proc_x_full_screen->read_proc = proc_x_full_screen_read;
	proc_x_full_screen->write_proc = proc_x_full_screen_write;
	proc_x_full_screen->data = NULL;

	proc_y_full_screen = create_proc_entry(PROC_ENTRY_Y_FULL_SCREEN, 0644,
					proc->parent);
	if (proc_y_full_screen == NULL) {
		rc = -ENOMEM;
		goto err_del_x_full_screen;
	}
	proc_y_full_screen->read_proc = proc_y_full_screen_read;
	proc_y_full_screen->write_proc = proc_y_full_screen_write;
	proc_y_full_screen->data = NULL;
#endif

	return 0;

#ifdef CONFIG_TOUCHSCREEN_SCALE
err_del_x_full_screen:
	remove_proc_entry(PROC_ENTRY_X_FULL_SCREEN, proc->parent);
err_del_y_scale:
	remove_proc_entry(PROC_ENTRY_Y_SCALE, proc->parent);
err_del_x_scale:
	remove_proc_entry(PROC_ENTRY_X_SCALE, proc->parent);
err_del_wakeup:
	remove_proc_entry(PROC_ENTRY_WAKEUP, proc->parent);
#endif
err_del_cmd:
	remove_proc_entry(PROC_ENTRY_CMD, proc->parent);
err_del_rotate:
	remove_proc_entry(PROC_ENTRY_ROTATE, proc->parent);
err_del_debug:
	remove_proc_entry(PROC_ENTRY_DEBUG, proc->parent);
err_del_parent:
	remove_proc_entry(PROC_PARENT_DIR, NULL);
	return rc;
}

static int proc_term(struct _egalax_i2c *egalax_i2c)
{
	struct proc_dir *proc = &egalax_i2c->proc;

#ifdef CONFIG_TOUCHSCREEN_SCALE
	remove_proc_entry(PROC_ENTRY_X_SCALE, proc->parent);
	remove_proc_entry(PROC_ENTRY_Y_SCALE, proc->parent);
	remove_proc_entry(PROC_ENTRY_X_FULL_SCREEN, proc->parent);
	remove_proc_entry(PROC_ENTRY_Y_FULL_SCREEN, proc->parent);
#endif
	remove_proc_entry(PROC_ENTRY_WAKEUP, proc->parent);
	remove_proc_entry(PROC_ENTRY_CMD, proc->parent);
	remove_proc_entry(PROC_ENTRY_ROTATE, proc->parent);
	remove_proc_entry(PROC_ENTRY_DEBUG, proc->parent);
	remove_proc_entry(PROC_PARENT_DIR, NULL);
	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int device_suspend(struct i2c_client *client)
{
	int loop;

	if (g_is_hw_detected == 0)
		return 0;

	/* cancel idle detect work regardless of state. we do want to do it
	   before mutex is locked in state_transition(). Otherwise we are
	   running into recursive lock deadlock */
	if (g_idle_detect_period > 0)
		cancel_delayed_work_sync(&idle_detect_work);

	/* do state transition */
	state_transition(SUSPEND_EVENT);

	/* if we were in IDLE_STATE we do not transit to READY_TO_SLEEP_STATE
	 * directly but rather first go into wait for wakeup response state. To
	 * cover for this case we want to wait for READY_TO_SLEEP_STATE for some
	 * time - say 300 msec checking the state every 100 msec */
	loop = 3;
	while (g_state != READY_TO_SLEEP_STATE && loop--)
		msleep(100);

	/* if we achieved READY_TO_SLEEP_STATE */
	if (g_state == READY_TO_SLEEP_STATE) {

		/* note that we have to do the following sequence: (disable_irq,
		   cancel_work_sync) in suspend thread. It will deadlock if
		   executing it in IRQ thread. Also this needs to be done before
		   we issue sleep command, to avoid potential i2c error messages
		   from IRQ thread processing touches while issuing
		   i2c_master_recv */

		/* make sure that no new IRQ work is generated */
		disable_irq(client->irq);

		/* make sure that outstanding IRQ work is cancelled */
		cancel_work_sync(&p_egalax_i2c_dev->work);

		/* send sleep command */
		if (send_sleep()) {

			TS_INFO("%s: suspended\n", __func__);
			state_transition(SLEEP_CMD_SUCCESS_EVENT);
		} else {
			TS_ERR("%s: failed to send sleep command\n", __func__);
			state_transition(SLEEP_CMD_FAILED_EVENT);
		}
	} else
		/* most probably we did not receive wakeup response. We are
		 * staying in the same state here. */
		TS_ERR("%s: failed to achieve READY_TO_SLEEP_STATE\n",
			__func__);

	return 0;
}

static int device_resume(struct i2c_client *client)
{
	int loop;

	if (g_is_hw_detected == 0)
		return 0;

	/* do state transition */
	state_transition(RESUME_EVENT);

	/* when transiting from SUSPENDED to ACTIVE state we first go into
	 * wait for wakeup response state. We want to wait for ACTIVE_STATE for
	 * some time - say 300 msec checking the state every 100 msec */
	loop = 3;
	while (g_state != ACTIVE_STATE && loop--)
		msleep(100);

	/* if we did not receive wakeup response */
	if (g_state != ACTIVE_STATE) {
		TS_ERR("%s: failed to achieve active state\n", __func__);
		/* inform FSM */
		state_transition(RESUME_WAKEUP_FAILED_EVENT);
	} else
		TS_INFO("%s: resumed\n", __func__);

	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void egalax_i2c_early_suspend(struct early_suspend *h)
{
	device_suspend(p_egalax_i2c_dev->client);
}

static void egalax_i2c_late_resume(struct early_suspend *h)
{
	device_resume(p_egalax_i2c_dev->client);
}

/* we early suspend handler to be called after EARLY_SUSPEND_LEVEL_BLANK_SCREEN
   handler is called, so we increase priority by 10 */
static struct early_suspend egalax_i2c_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 10,
	.suspend = egalax_i2c_early_suspend,
	.resume = egalax_i2c_late_resume,
};

#endif /* CONFIG_HAS_EARLYSUSPEND */

#ifdef CONFIG_PM
static int egalax_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
#ifndef CONFIG_HAS_EARLYSUSPEND
	/* if early suspend is not enabled suspend device here */
	TS_INFO("suspending device\n");
	return device_suspend(client);
#endif
	return 0;
}

static int egalax_i2c_resume(struct i2c_client *client)
{
#ifndef CONFIG_HAS_EARLYSUSPEND
	/* if early suspend is not enabled resume device here */
	TS_INFO("resuming device\n");
	return device_resume(client);
#endif
	return 0;
}
#else
#define egalax_i2c_suspend       NULL
#define egalax_i2c_resume        NULL
#endif

/* idle detect work function */
static void egalax_idle_detect_wq_func(struct work_struct *p_work)
{
	state_transition(IDLE_DETECT_EVENT);
}

static struct input_dev *allocate_Input_Dev(void)
{
	int ret;
	struct input_dev *pInputDev = NULL;

	pInputDev = input_allocate_device();
	if (pInputDev == NULL) {
		TS_ERR("Failed to allocate input device\n");
		return NULL;
	}

	pInputDev->name = "eGalax Touch Screen";
	pInputDev->phys = "I2C";
	pInputDev->id.bustype = BUS_I2C;
	pInputDev->id.vendor = 0x0EEF;
	pInputDev->id.product = 0x0020;
	pInputDev->id.version = 0x0000;

	set_bit(EV_ABS, pInputDev->evbit);

	input_set_abs_params(pInputDev, ABS_MT_POSITION_X, 0, MAX_X_RES, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_POSITION_Y, 0, MAX_Y_RES, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(pInputDev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);

	ret = input_register_device(pInputDev);
	if (ret) {
		TS_ERR("Unable to register input device\n");
		input_free_device(pInputDev);
		pInputDev = NULL;
		return NULL;
	}

#ifdef CONFIG_CP_TS_SOFTKEY
	softkey_input_dev = input_allocate_device();
	if (softkey_input_dev == NULL) {
		TS_ERR("Failed to allocate input device for softkeys\n");
		goto input_done;
	}

	softkey_input_dev->name = "softkey";
	softkey_input_dev->phys = "what?";
	softkey_input_dev->id.bustype = BUS_I2C;
	softkey_input_dev->id.vendor = 0x0001;
	softkey_input_dev->id.product = 0x0002;
	softkey_input_dev->id.version = 0x0100;

	set_bit(EV_KEY, softkey_input_dev->evbit);
	__set_bit(KEY_SEARCH, softkey_input_dev->keybit);
	__set_bit(KEY_MENU, softkey_input_dev->keybit);
	__set_bit(KEY_BACK, softkey_input_dev->keybit);
	__set_bit(KEY_HOME, softkey_input_dev->keybit);

	ret = input_register_device(softkey_input_dev);
	if (ret) {
		TS_ERR("Unable to register input device for soft keys\n");
		input_free_device(softkey_input_dev);
		softkey_input_dev = NULL;
		goto input_done;
	}
input_done:
#endif

	return pInputDev;
}

static int __devinit egalax_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct egalax_i2c_ts_cfg *cfg;

	DBG();

	if (client->dev.platform_data == NULL) {
		TS_ERR("Missing board dependent configurations\n");
		ret = -EFAULT;
		goto err_exit;
	}

	cfg = (struct egalax_i2c_ts_cfg *)client->dev.platform_data;

	if (cfg->gpio.event < 0) {
		TS_ERR("Invalid GPIO: event=%d\n", cfg->gpio.event);
		ret = -EFAULT;
		goto err_exit;
	}

	p_egalax_i2c_dev = kzalloc(
		sizeof(struct _egalax_i2c), GFP_KERNEL);
	if (!p_egalax_i2c_dev) {
		TS_ERR("Unable to request memory for device\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	/* store hardware/board dependent data */
	memcpy(&p_egalax_i2c_dev->hw_cfg, cfg,
		sizeof(p_egalax_i2c_dev->hw_cfg));

	p_egalax_i2c_dev->client = client;
	mutex_init(&p_egalax_i2c_dev->mutex_wq);

	p_egalax_i2c_dev->ktouch_wq = create_workqueue("egalax_touch_wq");
	if (p_egalax_i2c_dev->ktouch_wq == NULL) {
		TS_ERR("Unable to create workqueue\n");
		ret = -ENOMEM;
		goto err_free_dev;
	}

	INIT_WORK(&p_egalax_i2c_dev->work, egalax_i2c_wq);

	i2c_set_clientdata(client, p_egalax_i2c_dev);

	/* Enable touchscreen regulator */
	if (!cfg->supply) {
		p_egalax_i2c_dev->ts_regulator = NULL;
	} else {
		p_egalax_i2c_dev->ts_regulator = regulator_get(
			&client->dev, cfg->supply);
		if (IS_ERR(p_egalax_i2c_dev->ts_regulator)) {
			TS_ERR("Unable to find touchscreen regulator\n");
			p_egalax_i2c_dev->ts_regulator = NULL;
		} else {
			ret = regulator_enable(p_egalax_i2c_dev->ts_regulator);
			if (ret) {
				TS_ERR("Can't enable touchscreen regulator\n");
				goto err_free_regulator;
			}
		}
	}

	/* reserve GPIO for touchscreen event interrupt */
	ret = gpio_request(cfg->gpio.event, "egalax i2c ts event");
	if (ret < 0) {
		TS_ERR("Unable to request gpio=%d\n", cfg->gpio.event);
		goto err_free_regulator;
	}
	gpio_direction_input(cfg->gpio.event);

	/* reserve GPIO for touchscreen controller reset */
	if (cfg->gpio.reset >= 0) {
		ret = gpio_request(cfg->gpio.reset, "egalax i2c ts reset");
		if (ret < 0) {
			TS_ERR("Unable to request GPIO pin %d\n",
				cfg->gpio.reset);
			goto err_free_event_gpio;
		}
		gpio_direction_output(cfg->gpio.reset, 1);

		/* now reset the touchscreen controller */
		gpio_set_value(cfg->gpio.reset, cfg->reset_level);

		if (cfg->reset_time < MIN_LONG_RESET_TIME) {
			msleep(p_egalax_i2c_dev->hw_cfg.reset_time);
			ret = egalax_probe_part2(p_egalax_i2c_dev);
		} else
			/* slow reset, have to queue so start up is
			   not affected */
			queue_work(system_long_wq, &shared_work);
	} else
		/* reset pin not used but still have to complete probe */
		ret = egalax_probe_part2(p_egalax_i2c_dev);

	return ret;

err_free_event_gpio:
	gpio_free(cfg->gpio.event);
	i2c_set_clientdata(client, NULL);

err_free_regulator:
	if (p_egalax_i2c_dev->ts_regulator) {
		regulator_disable(p_egalax_i2c_dev->ts_regulator);
		regulator_put(p_egalax_i2c_dev->ts_regulator);
		p_egalax_i2c_dev->ts_regulator = NULL;
	}

err_free_dev:
	if (p_egalax_i2c_dev->ktouch_wq)
		destroy_workqueue(p_egalax_i2c_dev->ktouch_wq);
	mutex_destroy(&p_egalax_i2c_dev->mutex_wq);
	kfree(p_egalax_i2c_dev);
	p_egalax_i2c_dev = NULL;

err_exit:
	return ret;
}

static int egalax_probe_part2(struct _egalax_i2c *p_egalax_dev)
{
	int ret, cnt, read_cnt;
	u8 x_buf[MAX_I2C_LEN] = {0};
	struct i2c_client *client = p_egalax_dev->client;
	int gpio = irq_to_gpio(client->irq);
	struct egalax_i2c_ts_cfg *cfg;

	cfg = (struct egalax_i2c_ts_cfg *)client->dev.platform_data;

	if (cfg->gpio.reset != -1) {
		/*
		 * release the reset so the controller can come up
		 */
		if (cfg->reset_level == 0)
			gpio_set_value(cfg->gpio.reset, 1);
		else
			gpio_set_value(cfg->gpio.reset, 0);
		msleep(100);
	}

	/*
	 * initiate an I2C read to put the touchscreen controller into a known
	 * state
	 */

	ret = i2c_master_recv(client, x_buf, MAX_I2C_LEN);
	if (ret > 0) {
		TS_INFO_PKT("eGalax I2C touchscreen message", x_buf);
	} else {
		TS_ERR("unable to talk to the touchscreen controller\n");
		ret = -ENODEV;
		goto err_free_reset_gpio;
	}

	/* reserve the irq line */
	ret = request_irq(client->irq, egalax_i2c_interrupt,
			IRQF_TRIGGER_FALLING,
			client->name, p_egalax_i2c_dev);
	if (ret) {
		TS_ERR("request_irq(%d) failed\n", client->irq);
		goto err_free_reset_gpio;
	}

	/* drain the FIFO so the INT line can go back to high */
	cnt = 0;
	read_cnt = 0;
	while (!gpio_get_value(gpio)) {
		read_cnt++;
		ret = i2c_master_recv(client, x_buf, MAX_I2C_LEN);
		if (ret < 0)
			cnt++;

		if (cnt >= 3) {
			TS_ERR("Unable to read from touchscreen via i2c\n");
			goto err_free_irq;
			break;
		}

		if (read_cnt >= 1000) {
			TS_ERR("gpio(%d), irq(%d) seems to be stuck low\n",
				cfg->gpio.event, client->irq);
			goto err_free_irq;
		}
	}

	ret = proc_init(p_egalax_i2c_dev);
	if (ret) {
		TS_ERR("proc fs install failed\n");
		goto err_free_irq;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&egalax_i2c_early_suspend_desc);
	TS_INFO("register for early suspend\n");
#endif /* CONFIG_HAS_EARLYSUSPEND */

	/* set state to ACTIVE */
	active();

	TS_INFO("eGalax I2C touchscreen driver probed\n");
	TS_INFO("reset=%d event=%d irq=%d\n", cfg->gpio.reset, cfg->gpio.event,
		client->irq);

	g_is_hw_detected = 1;
	return ret;

err_free_irq:
	free_irq(client->irq, p_egalax_i2c_dev);

err_free_reset_gpio:
	if (cfg->gpio.reset >= 0)
		gpio_free(cfg->gpio.reset);

	/* free the interrupt gpio */
	gpio_free(cfg->gpio.event);
	return ret;
}

static int __devexit egalax_i2c_remove(struct i2c_client *client)
{
	struct _egalax_i2c *egalax_i2c = i2c_get_clientdata(client);

	DBG();

	proc_term(egalax_i2c);

	if (p_egalax_i2c_dev->ktouch_wq)
		destroy_workqueue(p_egalax_i2c_dev->ktouch_wq);

	mutex_destroy(&p_egalax_i2c_dev->mutex_wq);

	free_irq(client->irq, egalax_i2c);

	if (egalax_i2c->hw_cfg.gpio.reset >= 0)
		gpio_free(egalax_i2c->hw_cfg.gpio.reset);
	gpio_free(egalax_i2c->hw_cfg.gpio.event);

	/* Disable and free touchscreen regulator */
	if (p_egalax_i2c_dev->ts_regulator) {
		regulator_disable(p_egalax_i2c_dev->ts_regulator);
		regulator_put(p_egalax_i2c_dev->ts_regulator);
		p_egalax_i2c_dev->ts_regulator = NULL;
	}

	i2c_set_clientdata(client, NULL);
	kfree(egalax_i2c);
	p_egalax_i2c_dev = NULL;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&egalax_i2c_early_suspend_desc);
	TS_INFO("unregister for early suspend\n");
#endif /* CONFIG_HAS_EARLYSUSPEND */
	return 0;
}

static struct i2c_device_id egalax_i2c_idtable[] = {
	{ "egalax_i2c", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, egalax_i2c_idtable);

static struct i2c_driver egalax_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "egalax_i2c",
	},
	.id_table = egalax_i2c_idtable,
	.class = I2C_CLASS_TOUCHSCREEN,
	.probe = egalax_i2c_probe,
	.remove = __devexit_p(egalax_i2c_remove),
	.suspend = egalax_i2c_suspend,
	.resume = egalax_i2c_resume,
};

static void egalax_i2c_ts_exit(void)
{
	DBG();

	i2c_del_driver(&egalax_i2c_driver);

	cancel_work_sync(&shared_work);

	if (input_dev) {
		input_unregister_device(input_dev);
		input_free_device(input_dev);
		input_dev = NULL;
		TS_INFO("Input device unregistered\n");
	}

#ifdef CONFIG_CP_TS_SOFTKEY
	if (softkey_input_dev) {
		input_unregister_device(softkey_input_dev);
		input_free_device(softkey_input_dev);
		softkey_input_dev = NULL;
	}
#endif
}

static int egalax_i2c_ts_init(void)
{
	int result;

	DBG();

	input_dev = allocate_Input_Dev();
	if (input_dev == NULL) {
		TS_ERR("allocate_Input_Dev failed\n");
		result = -ENOMEM;
		goto fail;
	}

	TS_INFO("Input device registered\n");

	memset(PointBuf, 0, sizeof(struct point_data)*MAX_SUPPORT_POINT);
#ifdef CONFIG_CP_TS_SOFTKEY
	memset(BtnState, 0, sizeof(short)*MAX_BTN);
#endif

	return i2c_add_driver(&egalax_i2c_driver);

fail:
	egalax_i2c_ts_exit();
	return result;
}

module_init(egalax_i2c_ts_init);
module_exit(egalax_i2c_ts_exit);

MODULE_DESCRIPTION("egalax touch screen i2c driver");
MODULE_LICENSE("GPL");
