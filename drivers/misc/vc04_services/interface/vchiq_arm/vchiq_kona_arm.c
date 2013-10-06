/*
 * Copyright (c) 2010-2011 Broadcom Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/bug.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>

#ifdef CONFIG_ARCH_KONA

#include <chal/chal_ipc.h>
#include <mach/irqs.h>
#include <chal/chal_icd.h>
#include <mach/io_map.h>
#include <plat/pwr_mgr.h>
#define IPC_SHARED_MEM_BASE       KONA_INT_SRAM_BASE

#else

#include <csp/chal_ipc.h>
#include <csp/chal_intid.h>
#include <csp/chal_icd.h>
#define IPC_SHARED_MEM_BASE       MM_IO_BASE_SRAM

#endif

#include <mach/vc_gpio.h>

#ifdef CONFIG_BCM_VC_PMU_REQUEST
#include <mach/vc_pmu_request.h>
#endif

#include <vchiq_platform_data.h>

#include "vchiq_arm.h"
#include "vchiq_kona_arm.h"
#include "vchiq_connected.h"

#include "vchiq_memdrv.h"
#include "vchiq_build_info.h"

#ifdef CONFIG_BCM_HDMI_DET
#include <linux/broadcom/hdmi.h>
#endif

#include <vc_mem.h>

#if defined(VCHIQ_SM_ALLOC_VCDDR)
#include "debug_sym.h"
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>

#define PROC_WRITE_BUF_SIZE 256

static void
vchiq_early_suspend(struct early_suspend *h);
static void
vchiq_late_resume(struct early_suspend *h);

static struct early_suspend g_vchiq_early_suspend = {
	.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB + 5,
	.suspend = vchiq_early_suspend,
	.resume  = vchiq_late_resume
};

#endif

#define VCHIQ_DOORBELL_IRQ BCM_INT_ID_IPC_OPEN

typedef struct {
	 unsigned int                 instNum;

	 const char                  *instance_name;
	 const VCHIQ_PLATFORM_DATA_T *platform_data;

	 struct proc_dir_entry        *instance_cfg_dir;
	 struct proc_dir_entry        *vchiq_version_cfg_entry;
	 struct proc_dir_entry        *vchiq_control_cfg_entry;
} VCHIQ_KERNEL_STATE_T;

struct platform_state {
	struct timer_list suspend_failure_timer;
	atomic_t          suspend_failure_timer_state;

	VCHIQ_ARM_STATE_T arm_state;
};

struct vc_suspend_info {
	unsigned long suspend_stage;
	unsigned long suspend_cb_func;
};


#define VCHIQ_NUM_VIDEOCORES 1

#define SUSPEND_FAILURE_TIMER 1
#define RESUME_FAILURE_TIMER  2
#define TIMEOUT_FAILURE_TIMER 3
#define SUSPEND_FAILURE_TIMER_DURATION_MS  1000

#define DMA_QOS_VAL 100


static const char *const copyright = "Copyright (c) 2011-2012 Broadcom";

static VCHIQ_KERNEL_STATE_T    *vchiq_kernel_state[VCHIQ_NUM_VIDEOCORES];
static unsigned int             vchiq_num_instances;

static CHAL_IPC_HANDLE   ipcHandle;

static int               g_initialized;

static VCHIQ_STATE_T    *g_vchiq_state;
static VCHIQ_SLOT_ZERO_T *g_vchiq_slot_zero;

static int               g_use_autosuspend;
static int               g_use_suspend_timer = 1;
#ifdef CONFIG_HAS_EARLYSUSPEND
static int               g_early_susp_ctrl;
static int               g_earlysusp_suspend_allowed;
#endif
static void             *g_vchiq_ipc_shared_mem;
static int               g_vchiq_ipc_shared_mem_size;
#if defined(VCHIQ_SM_ALLOC_VCDDR)
static VC_MEM_ACCESS_HANDLE_T g_vchiq_mem_hndl;
static int                    g_vchiq_ipc_shared_mem_addr;
#endif

static irqreturn_t
vchiq_doorbell_irq(int irq, void *dev_id);

static void
suspend_failure_timer_callback(unsigned long context);

static int
read_vc_debug_var(VC_MEM_ACCESS_HANDLE_T handle,
	const char *symbol,
	void *buf, size_t bufsize);



int __init
vchiq_kona_init(VCHIQ_STATE_T *state)
{
	g_vchiq_state = state;

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&g_vchiq_early_suspend);
#endif

	return 0;
}

void __exit
vchiq_kona_exit(VCHIQ_STATE_T *state)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&g_vchiq_early_suspend);
#endif
	WARN_ON(state != g_vchiq_state);
	g_vchiq_state = NULL;
	free_irq(VCHIQ_DOORBELL_IRQ, state);
}

VCHIQ_STATUS_T
vchiq_platform_init_state(VCHIQ_STATE_T *state)
{
	VCHIQ_STATUS_T status = VCHIQ_ERROR;
	struct timer_list *failure_timer;
	struct platform_state *plat_state;

	state->platform_state = kzalloc(sizeof(struct platform_state),
		GFP_KERNEL);

	if (!state->platform_state)
		goto out;

	status = vchiq_arm_init_state(state,
		&((struct platform_state *)state->platform_state)->arm_state);
	if (status != VCHIQ_SUCCESS) {
		kfree(state->platform_state);
		state->platform_state = NULL;
		goto out;
	} else
		plat_state = (struct platform_state *)state->platform_state;

	failure_timer = &plat_state->suspend_failure_timer;
	init_timer(failure_timer);
	failure_timer->data = (unsigned long)(state);
	failure_timer->function = suspend_failure_timer_callback;
	atomic_set(&plat_state->suspend_failure_timer_state, 0);

out:
	return status;
}

inline VCHIQ_ARM_STATE_T*
vchiq_platform_get_arm_state(VCHIQ_STATE_T *state)
{
	return state->platform_state
		? &((struct platform_state *)state->platform_state)->arm_state
		: NULL;
}

void
remote_event_signal(REMOTE_EVENT_T *event)
{
	wmb();

	event->fired = 1;

	dsb();         /* data barrier operation */

	if (event->armed)
		/* trigger vc interrupt */
		chal_ipc_int_vcset(ipcHandle, IPC_INTERRUPT_SOURCE_0);
}

int
vchiq_copy_from_user(void *dst, const void *src, int size)
{
	 if ((uint32_t)src < TASK_SIZE)
		return copy_from_user(dst, src, size);
	 else {
		memcpy(dst, src, size);
		return 0;
	 }
}

static int
get_vc_suspend_syms(struct vc_suspend_info *suspend_info)
{
	VC_MEM_ACCESS_HANDLE_T mem_hndl;
	int ret = -1;

	if (!suspend_info)
		goto out;

	suspend_info->suspend_stage = -1;
	suspend_info->suspend_cb_func = -1;

	if (OpenVideoCoreMemory(&mem_hndl) != 0)
		goto out;

	ret = read_vc_debug_var(mem_hndl, VCHIQ_VC_SUSPEND_ITER_SYMBOL,
				(VC_MEM_ADDR_T *)&suspend_info->suspend_stage,
				sizeof(suspend_info->suspend_stage));
	if (ret == 0) {
		ret = -1;
		goto close;
	}

	ret = read_vc_debug_var(mem_hndl, VCHIQ_VC_SUSP_RES_CALLBACK_SYMBOL,
				(VC_MEM_ADDR_T *)&suspend_info->suspend_cb_func,
				sizeof(suspend_info->suspend_stage));
	if (ret == 0) {
		ret = -1;
		goto close;
	}

	ret = 0;

close:
	CloseVideoCoreMemory(mem_hndl);
out:
	return ret;
}

static void
start_suspend_failure_timer(VCHIQ_STATE_T *state, int type)
{
	struct timer_list *sft_p = &((struct platform_state *)
			state->platform_state)->suspend_failure_timer;
	atomic_t *timer_state = &((struct platform_state *)
			state->platform_state)->suspend_failure_timer_state;
	long old_state = atomic_xchg(timer_state, type);
	BUG_ON(old_state != 0);
	del_timer(sft_p);
	sft_p->expires = jiffies +
		msecs_to_jiffies(SUSPEND_FAILURE_TIMER_DURATION_MS);
	add_timer(sft_p);
}

static void
stop_suspend_failure_timer(VCHIQ_STATE_T *state)
{
	struct timer_list *sft_p = &((struct platform_state *)
				state->platform_state)->suspend_failure_timer;
	atomic_t *timer_state = &((struct platform_state *)
			state->platform_state)->suspend_failure_timer_state;
	del_timer(sft_p);
	atomic_set(timer_state, 0);
}

void
vchiq_platform_handle_timeout(VCHIQ_STATE_T *state)
{
	struct vc_suspend_info suspend_info;
	stop_suspend_failure_timer(state);
	get_vc_suspend_syms(&suspend_info);
	vchiq_log_error(vchiq_susp_log_level,
		"%s - ERROR VideoCore %s timed out. VC suspend stage "
		"0x%08lx, cb func 0x%08lx", __func__,
		state->conn_state == VCHIQ_CONNSTATE_RESUME_TIMEOUT ?
			"RESUME" : "SUSPEND",
		suspend_info.suspend_stage, suspend_info.suspend_cb_func);
	BUG();
}

static void
suspend_failure_timer_callback(unsigned long context)
{
	VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)context;
	atomic_t *timer_state = &((struct platform_state *)
			state->platform_state)->suspend_failure_timer_state;
	long old_state = atomic_xchg(timer_state, 0);
	if (old_state == 0) {
		vchiq_log_error(vchiq_susp_log_level,
			"%s - ERROR timer callback triggered.  Unknown reason",
			__func__);
		BUG();
	} else if (old_state != TIMEOUT_FAILURE_TIMER) {
		/* In order to extract useful info from videocore we can't be in
		 * atomic context, so defer to task for logging purposes... */
		vchiq_set_conn_state(state,
			(old_state == SUSPEND_FAILURE_TIMER) ?
				VCHIQ_CONNSTATE_PAUSE_TIMEOUT :
				VCHIQ_CONNSTATE_RESUME_TIMEOUT);
		request_poll(state, NULL, 0);

		/* ... however - don't trust that the task will ever trigger.
		 * Restart timer just in case. */
		start_suspend_failure_timer(state, TIMEOUT_FAILURE_TIMER);
	} else {
		vchiq_log_error(vchiq_susp_log_level,
			"%s - ERROR VideoCore %s timed out.  Failed to trigger "
			"task to extract VC status", __func__,
			state->conn_state == VCHIQ_CONNSTATE_RESUME_TIMEOUT ?
			"RESUME" : "SUSPEND");
		BUG();
	}
}


VCHIQ_STATUS_T
vchiq_platform_suspend(VCHIQ_STATE_T *state)
{
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
	VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);
	unsigned int *wakeaddr_p = (unsigned int *)
					&g_vchiq_slot_zero->platform_data[0];

	vchiq_log_trace(vchiq_susp_log_level, "%s", __func__);

	write_lock_bh(&arm_state->susp_res_lock);
	if (arm_state->wake_address != 0) {
		write_unlock_bh(&arm_state->susp_res_lock);
		vchiq_log_error(vchiq_susp_log_level,
			"%s - ERROR VideoCore already suspended", __func__);
		/* It should be impossible to get in to this state. */
		BUG();
		goto out;
	}
	arm_state->suspend_start_time = cpu_clock(0);
	write_unlock_bh(&arm_state->susp_res_lock);

	/* Invalidate the wake address */

	writel(~0, wakeaddr_p);
	wmb();

	/* Initiate videocore suspend */
	status = vchiq_pause_internal(state);
	if (status != VCHIQ_SUCCESS) {
		write_lock_bh(&arm_state->susp_res_lock);
		set_suspend_state(arm_state, VC_SUSPEND_FAILED);
		if (!vchiq_videocore_wanted(state))
			start_suspend_timer(arm_state);
		vchiq_log_error(vchiq_susp_log_level,
			"VideoCore suspend failed!");
		write_unlock_bh(&arm_state->susp_res_lock);
		goto out;
	}

	start_suspend_failure_timer(state, SUSPEND_FAILURE_TIMER);
	vchiq_log_info(vchiq_susp_log_level, "%s - waiting for pause event",
		__func__);

out:
	vchiq_log_trace(vchiq_susp_log_level, "%s exit %d", __func__, status);

	return status;
}

void
vchiq_platform_paused(VCHIQ_STATE_T *state)
{
	VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);
	unsigned int *wakeaddr_p = (unsigned int *)
					&g_vchiq_slot_zero->platform_data[0];
	unsigned long expiry, early_expiry;
	const unsigned long timeout_val =
			msecs_to_jiffies(SUSPEND_FAILURE_TIMER_DURATION_MS);
	int timed_out = 0, warned = 0;
	struct vc_suspend_info suspend_info;

	stop_suspend_failure_timer(state);

	vchiq_log_trace(vchiq_susp_log_level, "%s", __func__);

	vchiq_log_info(vchiq_susp_log_level, "%s - pause event received",
		__func__);

	/* wait for wake address */
	early_expiry = jiffies + (timeout_val >> 1);
	expiry = early_expiry + (timeout_val >> 1);
	do {
		cpu_relax();
		arm_state->wake_address = readl(wakeaddr_p);
		if (time_is_after_jiffies(early_expiry))
			continue;
		if (time_is_before_jiffies(expiry)) {
			timed_out = 1;
			break;
		} else if (warned)
			continue;

		/* This is only run once, for timeout > timeout_val/2.*/
		get_vc_suspend_syms(&suspend_info);
		WARN(1, "%s - WARNING: early timeout waiting for VideoCore wake"
			" address. VC suspend stage 0x%08lx, cb func 0x%08lx",
			__func__, suspend_info.suspend_stage,
			suspend_info.suspend_cb_func);
		warned = 1;
	} while (arm_state->wake_address == ~0);

	if (timed_out || arm_state->wake_address == 0)
		get_vc_suspend_syms(&suspend_info);
	if (timed_out && (arm_state->wake_address == ~0)) {
		vchiq_log_error(vchiq_susp_log_level, "%s - ERROR: "
			"timed out waiting for VideoCore wake address. "
			"VC suspend stage 0x%08lx, cb func 0x%08lx",
			__func__, suspend_info.suspend_stage,
			suspend_info.suspend_cb_func);
		BUG();
	}
	vchiq_log_info(vchiq_susp_log_level, "%s - suspend continue received",
		__func__);

	chal_ipc_sleep_vc(ipcHandle);
	msleep(1);
	writel(~0, wakeaddr_p);
	wmb();

	vc_gpio_suspend();

#ifdef CONFIG_BCM_VC_PMU_REQUEST
	vc_pmu_req_suspend();
#endif

#ifdef CONFIG_BCM_HDMI_DET
	hdmi_detection_power_ctrl(false);
#endif

#ifdef CONFIG_ARCH_KONA
	msleep(1);
	/* indicate to the PMU that videocore is in reset */
	pwr_mgr_mm_crystal_clk_is_idle(true);
#endif

	write_lock_bh(&arm_state->susp_res_lock);
	if (arm_state->wake_address == 0) {
		set_suspend_state(arm_state, VC_SUSPEND_REJECTED);
		if (arm_state->vc_resume_state < VC_RESUME_IN_PROGRESS) {
			set_resume_state(arm_state, VC_RESUME_REQUESTED);
			request_poll(state, NULL, 0);
		}
		if (!vchiq_videocore_wanted(state))
			start_suspend_timer(arm_state);
		vchiq_log_error(vchiq_susp_log_level, "%s: Videocore suspend "
			"rejected! VC suspend stage 0x%08lx, cb func 0x%08lx",
			__func__, suspend_info.suspend_stage,
			suspend_info.suspend_cb_func);
	} else {
		unsigned long long awake_time = 0;
		unsigned long nanosec_rem = 0;
		unsigned long long suspend_time = 0;
		unsigned long nanosec_susp_rem = 0;

		arm_state->sleep_start_time = cpu_clock(0);
		if (arm_state->last_wake_time) {
			awake_time = arm_state->sleep_start_time -
						arm_state->last_wake_time;
			nanosec_rem = do_div(awake_time, 1000000000);
		}

		suspend_time = arm_state->sleep_start_time -
						arm_state->suspend_start_time;
		nanosec_susp_rem = do_div(suspend_time, 1000000000);

		set_suspend_state(arm_state, VC_SUSPEND_SUSPENDED);
		/* Kick the slot handler again to see if we need to resume */
		if (arm_state->vc_resume_state == VC_RESUME_REQUESTED)
			request_poll(state, NULL, 0);
		vchiq_log_warning(vchiq_susp_log_level, "VideoCore suspended - "
			"wake address %x, (awake %lu.%06lus, suspended in "
			"%lu.%06lus)",
			arm_state->wake_address, (unsigned long)awake_time,
			nanosec_rem / 1000, (unsigned long)suspend_time,
			nanosec_susp_rem / 1000);
		arm_state->autosuspend_override = 0;
	}
	write_unlock_bh(&arm_state->susp_res_lock);

	vchiq_log_trace(vchiq_susp_log_level, "%s exit", __func__);
}

VCHIQ_STATUS_T
vchiq_platform_resume(VCHIQ_STATE_T *state)
{
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
	VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);

	vchiq_log_trace(vchiq_susp_log_level, "%s", __func__);

	vchiq_log_info(vchiq_susp_log_level, "Resuming VideoCore at address %x",
		arm_state->wake_address);
	arm_state->resume_start_time = cpu_clock(0);

#ifdef CONFIG_ARCH_KONA
	/* indicate to the PMU that videocore is about to come out of reset */
	pwr_mgr_mm_crystal_clk_is_idle(false);
#endif

#ifdef CONFIG_BCM_HDMI_DET
	hdmi_detection_power_ctrl(true);
#endif

#ifdef CONFIG_BCM_VC_PMU_REQUEST
	vc_pmu_req_resume();
#endif

	vc_gpio_resume();

	/* Write the wake address to wake up videocore */
	chal_ipc_wakeup_vc(ipcHandle, arm_state->wake_address);

	start_suspend_failure_timer(state, RESUME_FAILURE_TIMER);

	vchiq_log_info(vchiq_susp_log_level,
		"Waiting for response from VideoCore...");

	vchiq_log_trace(vchiq_susp_log_level, "%s exit %d", __func__, status);
	return status;
}



void
vchiq_platform_resumed(VCHIQ_STATE_T *state)
{
	VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);
	unsigned long long sleep_time = 0;
	unsigned long nanosec_rem = 0;
	unsigned long long resume_time = 0;
	unsigned long nanosec_res_rem = 0;

	stop_suspend_failure_timer(state);

	vchiq_log_trace(vchiq_susp_log_level, "%s", __func__);
	write_lock_bh(&arm_state->susp_res_lock);
	set_resume_state(arm_state, VC_RESUME_RESUMED);
	arm_state->wake_address = 0;
	if (arm_state->sleep_start_time) {
		arm_state->last_wake_time = cpu_clock(0);
		sleep_time = arm_state->last_wake_time -
						arm_state->sleep_start_time;
		nanosec_rem = do_div(sleep_time, 1000000000);
		arm_state->sleep_start_time = 0; /* Not asleep - invalidate */
		resume_time = arm_state->last_wake_time -
						arm_state->resume_start_time;
		nanosec_res_rem = do_div(resume_time, 1000000000);
	}
	vchiq_log_warning(vchiq_susp_log_level,
		"VideoCore awake (slept %lu.%06lus, resumed in %lu.%06lus)",
		(unsigned long)sleep_time, nanosec_rem / 1000,
		(unsigned long)resume_time, nanosec_res_rem / 1000);

	/* we may have missed a poll in VCHIQ_CONNSTATE_CONNECTED so retry */
	request_poll(state, NULL, 0);
	write_unlock_bh(&arm_state->susp_res_lock);
	vchiq_log_trace(vchiq_susp_log_level, "%s exit", __func__);
}

int
vchiq_platform_videocore_wanted(VCHIQ_STATE_T *state)
{
	int early_susp_override = 0;
	(void)state;
#ifdef CONFIG_HAS_EARLYSUSPEND
	early_susp_override = (!g_earlysusp_suspend_allowed) &&
		g_early_susp_ctrl;
#endif

	return early_susp_override || !g_use_autosuspend;
}

int
vchiq_platform_use_suspend_timer(void)
{
	return g_use_suspend_timer;
}

void
vchiq_dump_platform_use_state(VCHIQ_STATE_T *state)
{
	char *enabled  = "ENABLED";
	char *disabled = "DISABLED";
	char *en_dis = g_use_autosuspend ? enabled : disabled;
	vchiq_log_warning(vchiq_susp_log_level, "Autosuspend %s", en_dis);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	en_dis = g_early_susp_ctrl ? enabled : disabled;
	vchiq_log_warning(vchiq_susp_log_level,
		"Early suspend control %s: (suspend allowed=%d)",
		en_dis, g_earlysusp_suspend_allowed || !g_early_susp_ctrl);
#endif
	en_dis = vchiq_platform_use_suspend_timer() ? enabled : disabled;
	vchiq_log_warning(vchiq_susp_log_level, "Suspend timer %s", en_dis);
}


static int version_read(char *buffer,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	int len = 0;

	len += sprintf(buffer + len,
			"%s %s\n%s\nversion %s\n",
			vchiq_get_build_date(),
			vchiq_get_build_time(),
			copyright,
			vchiq_get_build_version());

	return len;
}

static int vchiq_control_cfg_output(char *buffer,
		char **start,
		off_t off,
		int count,
		int *eof,
		void *data)
{
	int len = 0;

	VCHIQ_ARM_STATE_T *arm_state =
			vchiq_platform_get_arm_state(g_vchiq_state);
	VCHIQ_KERNEL_STATE_T    *kernState = data;

	len += sprintf(buffer + len, "%s %s\n%s\nversion %s\n",
			vchiq_get_build_date(),
			vchiq_get_build_time(),
			copyright,
			vchiq_get_build_version());

	len += sprintf(buffer + len, "VCHIQ instance '%s' %s\n",
			kernState->instance_name,
			get_conn_state_name(g_vchiq_state->conn_state));

	len += sprintf(buffer + len, "VideoCore %s\n",
			(arm_state->vc_suspend_state == VC_SUSPEND_SUSPENDED) ?
				"SUSPENDED" : "AWAKE");

	return len;

}

static void vchiq_autosuspend_test(void)
{
	int success = 1;
	int cnt = 1;

	VCHIQ_ARM_STATE_T *arm_state =
		vchiq_platform_get_arm_state(g_vchiq_state);
	while (success) {
		vchiq_log_info(vchiq_susp_log_level,
			"%s: Testing suspend / resume functionality - "
			"iteration %d", __func__, cnt++);
		if (vchiq_use_internal(g_vchiq_state, NULL, USE_TYPE_VCHIQ) !=
				VCHIQ_SUCCESS) {
			success = 0;
			vchiq_log_error(vchiq_susp_log_level,
				"%s: resume FAILED", __func__);
			continue;
		}
		vchiq_log_info(vchiq_susp_log_level,
			"%s: calling vchiq_release_internal", __func__);
		if (vchiq_release_internal(g_vchiq_state, NULL)
				== VCHIQ_SUCCESS) {
			if (wait_for_completion_interruptible(
					&arm_state->vc_suspend_complete) != 0) {
				success = 0;
				vchiq_log_error(vchiq_susp_log_level,
					"%s: Interrupted. Exiting.", __func__);
				continue;
			}
			read_lock_bh(&arm_state->susp_res_lock);
			if (arm_state->vc_suspend_state == VC_SUSPEND_FAILED) {
				success = 0;
				vchiq_log_error(vchiq_susp_log_level,
					"%s: suspend FAILED", __func__);
			}
			read_unlock_bh(&arm_state->susp_res_lock);
		} else {
			vchiq_log_error(vchiq_susp_log_level,
				"%s: release FAILED", __func__);
			success = 0;
		}
	}
}

static void vchiq_suspend_test(void)
{
	int success = 1;
	int cnt = 1;
	while (success) {
		vchiq_log_info(vchiq_susp_log_level,
			"%s: Testing suspend / resume functionality - "
			"iteration %d", __func__, cnt++);
		success = vchiq_arm_force_suspend(g_vchiq_state) != VCHIQ_ERROR;
		if (success) {
			int susp;
			vchiq_log_info(vchiq_susp_log_level,
				"%s: resuming...", __func__);
			susp = vchiq_arm_allow_resume(g_vchiq_state);
			if (!g_use_autosuspend && susp) {
				vchiq_log_info(vchiq_susp_log_level,
					"%s: ERROR: failed to resume videocore",
					__func__);
				success = 0;
			}
		}
	}
}

static int vchiq_control_cfg_parse(struct file *file,
	const char __user *buffer,
	unsigned long count,
	void *data)
{
	VCHIQ_KERNEL_STATE_T    *kernState = data;
	char                    *command;
	char                    kbuf[PROC_WRITE_BUF_SIZE + 1];

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf,
		buffer,
		count) != 0)
		return -EFAULT;
	kbuf[count - 1] = 0;

	command = kbuf;

	if (!g_vchiq_ipc_shared_mem_size) {
		if (!((strncmp("connect", command, strlen("connect")) == 0) ||
		(strncmp("version", command, strlen("version")) == 0))) {
			vchiq_log_warning(vchiq_arm_log_level,
			"%s: VC is not connected, dropping command: %s",
			__func__, command);
			/* Early exit. */
			return count;
		}
	}

	if (strncmp("connect", command, strlen("connect")) == 0) {
		if (vchiq_memdrv_initialise() != VCHIQ_SUCCESS)
			vchiq_log_error(vchiq_arm_log_level,
				"%s: failed to initialise vchiq for '%s'",
				__func__, kernState->instance_name);
		else
			vchiq_log_warning(vchiq_arm_log_level,
				"%s: initialised vchiq for '%s'", __func__,
				kernState->instance_name);
	} else if (strncmp("suspendtest", command, strlen("suspendtest"))
			== 0) {
		int orig_log_level = vchiq_susp_log_level;
		vchiq_susp_log_level = VCHIQ_LOG_INFO;
		if (g_use_autosuspend)
			vchiq_autosuspend_test();
		else
			vchiq_suspend_test();
		vchiq_log_info(vchiq_susp_log_level,
			"%s: Suspend / resume test exited", __func__);
		vchiq_susp_log_level = orig_log_level;
	} else if (strncmp("suspend", command, strlen("suspend")) == 0) {
		/* direct control of suspend from vchiq_control */
		vchiq_log_info(vchiq_susp_log_level,
			"%s: calling vchiq_arm_force_suspend", __func__);
		if (vchiq_arm_force_suspend(g_vchiq_state) == VCHIQ_SUCCESS) {
			vchiq_log_warning(vchiq_susp_log_level,
				"%s: suspended vchiq for '%s'", __func__,
				kernState->instance_name);
		} else {
			vchiq_log_error(vchiq_susp_log_level,
				"%s: failed to suspend vchiq '%s'",
				__func__, kernState->instance_name);
		}
	} else if (strncmp("resume", command, strlen("resume")) == 0) {
		/* direct control of resume from vchiq_control */
		vchiq_log_info(vchiq_susp_log_level,
				"%s: calling vchiq_arm_allow_resume", __func__);
		if (vchiq_arm_allow_resume(g_vchiq_state) == 1) {
			vchiq_log_warning(vchiq_susp_log_level,
				"%s: resume allowed for vchiq for '%s'"
				"- VideoCore remains asleep",
				__func__, kernState->instance_name);
		} else {
			vchiq_log_warning(vchiq_susp_log_level,
				"%s: resumed vchiq for '%s'",
				__func__, kernState->instance_name);
		}
	} else if (strncmp("autosuspend", command, strlen("autosuspend"))
			== 0) {
		/* enable autosuspend, using vchi_service_use/release usagei
		** counters to decide when to suspend */
		g_use_autosuspend = 1;
		vchiq_log_info(vchiq_susp_log_level,
			"%s: Enabling autosuspend for vchiq instance '%s'",
			__func__, kernState->instance_name);
		vchiq_check_suspend(g_vchiq_state);
	} else if (strncmp("noautosuspend", command, strlen("noautosuspend"))
			== 0) {
		/* disable autosuspend - allow direct control of suspend/resume
		** through vchiq_control */
		g_use_autosuspend = 0;
		vchiq_log_info(vchiq_susp_log_level,
			"%s: Disabling autosuspend for vchiq instance '%s'",
			__func__, kernState->instance_name);
		vchiq_arm_allow_resume(g_vchiq_state);

	} else if (strncmp("dumpuse", command, strlen("dumpuse")) == 0) {
		/* dump usage counts for all services to determine which
		** service(s) are preventing suspend */
		vchiq_dump_service_use_state(g_vchiq_state);
	} else if (strncmp("susptimer", command, strlen("susptimer")) == 0) {
		/* enable a short timeout before suspend to allow other "use"
		** commands in */
		g_use_suspend_timer = 1;
		if (g_use_autosuspend) {
			vchiq_log_info(vchiq_susp_log_level,
				"%s: Using timeout before suspend",
				__func__);
		}
	} else if (strncmp("nosusptimer", command, strlen("nosusptimer"))
			== 0) {
		/* disable timeout before suspend - enter suspend directly on
		** usage count hitting 0 (from lp task) */
		g_use_suspend_timer = 0;
		if (g_use_autosuspend) {
			vchiq_log_info(vchiq_susp_log_level,
				"%s: Not using timeout before suspend",
				__func__);
		}
#if defined(CONFIG_HAS_EARLYSUSPEND)
	} else if (strncmp("earlysuspctrl", command, strlen("earlysuspctrl"))
			== 0) {
		VCHIQ_ARM_STATE_T *arm_state =
			vchiq_platform_get_arm_state(g_vchiq_state);

		/* for configs with earlysuspend, allow suspend to be blocked
		** until the earlysuspend callback is called */
		g_early_susp_ctrl = 1;
		if (g_use_autosuspend) {
			vchiq_log_info(vchiq_susp_log_level,
				"%s: Using Early Suspend control for "
				"suspend/resume",
				__func__);
			write_lock_bh(&arm_state->susp_res_lock);
			vchiq_check_resume(g_vchiq_state);
			write_unlock_bh(&arm_state->susp_res_lock);
		}
	} else if (strncmp("noearlysuspctrl", command,
			strlen("noearlysuspctrl")) == 0) {
		/* disable control of suspend from earlysuspend callback */
		g_early_susp_ctrl = 0;
		if (g_use_autosuspend) {
			vchiq_log_info(vchiq_susp_log_level,
				"%s: Not using Early Suspend control for "
				"suspend/resume",
				__func__);
			vchiq_check_suspend(g_vchiq_state);
		}
#endif
	} else if (strncmp("version", command, strlen("version")) == 0) {

		vchiq_log_error(vchiq_arm_log_level,
			"%s %s\n%s\nversion %s\n",
			vchiq_get_build_date(),
			vchiq_get_build_time(),
			copyright,
			vchiq_get_build_version());
	} else {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: unknown command '%s'",
			__func__, command);
	}

	return count;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void
vchiq_early_suspend(struct early_suspend *h)
{
	if (g_early_susp_ctrl)
		vchiq_log_info(vchiq_susp_log_level,
			"%s: allowing suspend in vchiq", __func__);
	g_earlysusp_suspend_allowed = 1;
	vchiq_check_suspend(g_vchiq_state);
}

static void
vchiq_late_resume(struct early_suspend *h)
{
	VCHIQ_ARM_STATE_T *arm_state =
		vchiq_platform_get_arm_state(g_vchiq_state);

	if (g_early_susp_ctrl)
		vchiq_log_info(vchiq_susp_log_level,
			"%s: preventing suspend in vchiq", __func__);
	g_earlysusp_suspend_allowed = 0;
	write_lock_bh(&arm_state->susp_res_lock);
	vchiq_check_resume(g_vchiq_state);
	write_unlock_bh(&arm_state->susp_res_lock);
}
#endif


/****************************************************************************
*
* vchiq_userdrv_create_instance
*
*   The lower level drivers (vchiq_memdrv or vchiq_busdrv) will call this
*   function for each videocore that exists. We then register a character
*   driver which is what userspace uses to talk to us.
*
***************************************************************************/

VCHIQ_STATUS_T vchiq_userdrv_create_instance(
	const VCHIQ_PLATFORM_DATA_T *platform_data)
{
	VCHIQ_KERNEL_STATE_T *kernState;
	struct proc_dir_entry *vc_cfg_dir;

	vchiq_log_warning(vchiq_arm_log_level,
		"%s: [bi] vchiq_num_instances = %d, VCHIQ_NUM_VIDEOCORES = %d",
		__func__, vchiq_num_instances, VCHIQ_NUM_VIDEOCORES);

	if (vchiq_num_instances >= VCHIQ_NUM_VIDEOCORES) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: already created %d instances",
			__func__, VCHIQ_NUM_VIDEOCORES);

		return VCHIQ_ERROR;
	}

	/* Allocate some memory */
	kernState = kmalloc(sizeof(*kernState), GFP_KERNEL);
	if (kernState == NULL) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: failed to allocate memory",
			__func__);

		return VCHIQ_ERROR;
	}

	memset(kernState, 0, sizeof(*kernState));

	vchiq_kernel_state[vchiq_num_instances] = kernState;

	/* Do some bookkeeping */
	kernState->instNum = vchiq_num_instances++;
	kernState->instance_name = platform_data->instance_name;
	kernState->platform_data = platform_data;

	/* Create kona-specific proc entries */
	vc_cfg_dir = vchiq_proc_top();

	kernState->vchiq_version_cfg_entry =
		create_proc_entry("version", 0,
		vc_cfg_dir);
	if (kernState->vchiq_version_cfg_entry == NULL) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: failed to create proc directory",
			__func__);

		return VCHIQ_ERROR;
	}
	kernState->vchiq_version_cfg_entry->data = (void *)kernState;
	kernState->vchiq_version_cfg_entry->read_proc = &version_read;
	kernState->vchiq_version_cfg_entry->write_proc = NULL;

	kernState->instance_cfg_dir = proc_mkdir(kernState->instance_name,
		vc_cfg_dir);
	if (kernState->instance_cfg_dir == NULL) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: failed to create proc directory",
			__func__);

		return VCHIQ_ERROR;
	}

	kernState->vchiq_control_cfg_entry =
		create_proc_entry("vchiq_control", 0,
			kernState->instance_cfg_dir);
	if (kernState->vchiq_control_cfg_entry == NULL) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: failed to create proc entry",
			__func__);

		return VCHIQ_ERROR;
	}
	kernState->vchiq_control_cfg_entry->data = (void *)kernState;
	kernState->vchiq_control_cfg_entry->read_proc =
		&vchiq_control_cfg_output;
	kernState->vchiq_control_cfg_entry->write_proc =
		&vchiq_control_cfg_parse;

	vchiq_log_info(vchiq_arm_log_level,
		"%s: initialised vchiq for '%s'\n",
		__func__,
		kernState->instance_name);

	return VCHIQ_SUCCESS;
}
EXPORT_SYMBOL(vchiq_userdrv_create_instance);

/****************************************************************************
*
* vchiq_userdrv_suspend
*
*   The lower level drivers (vchiq_memdrv or vchiq_busdrv) will call this
*   function to suspend each videocore.
*
***************************************************************************/

VCHIQ_STATUS_T vchiq_userdrv_suspend(const VCHIQ_PLATFORM_DATA_T *platform_data)
{
	VCHIQ_KERNEL_STATE_T *kernState = NULL;
	VCHIQ_STATUS_T status;
	int i;

	for (i = 0; i < vchiq_num_instances; i++) {
		if (vchiq_kernel_state[i]->platform_data == platform_data) {
			kernState = vchiq_kernel_state[i];
			break;
		}
	}

	if (!kernState) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: failed to find state for instance %s",
			__func__,
			platform_data->instance_name);

		return VCHIQ_ERROR;
	}


	/* force videocore to suspend, even if autosuspend didn't want us to */
	status = vchiq_arm_force_suspend(g_vchiq_state);

	if (status == VCHIQ_SUCCESS)
		vchiq_log_warning(vchiq_susp_log_level,
			"%s: suspended vchiq for '%s'",
			__func__,
			kernState->instance_name);
	else
		vchiq_log_error(vchiq_susp_log_level,
			"%s: failed to suspend vchiq '%s'",
			__func__,
			kernState->instance_name);

	return status;
}
EXPORT_SYMBOL(vchiq_userdrv_suspend);

/****************************************************************************
*
* vchiq_userdrv_resume
*
*   The lower level drivers (vchiq_memdrv or vchiq_busdrv) will call this
*   function to resume each videocore.
*
***************************************************************************/

VCHIQ_STATUS_T vchiq_userdrv_resume(const VCHIQ_PLATFORM_DATA_T *platform_data)
{
	VCHIQ_KERNEL_STATE_T *kernState = NULL;
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
	int i, suspended;

	for (i = 0; i < vchiq_num_instances; i++) {
		if (vchiq_kernel_state[i]->platform_data == platform_data) {
			kernState = vchiq_kernel_state[i];
			break;
		}
	}

	if (!kernState) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: failed to find state for instance %s",
			__func__,
			platform_data->instance_name);

		return VCHIQ_ERROR;
	}

	vchiq_log_info(vchiq_susp_log_level,
		"%s: resuming vchiq for '%s'", __func__,
		kernState->instance_name);

	suspended = vchiq_arm_allow_resume(g_vchiq_state);

	if (suspended) {
		vchiq_log_warning(vchiq_susp_log_level,
			"%s: vchiq for '%s' remains suspended", __func__,
			kernState->instance_name);
	} else {
		vchiq_log_warning(vchiq_susp_log_level,
			"%s: resumed vchiq for '%s'", __func__,
			kernState->instance_name);
	}

	return status;
}
EXPORT_SYMBOL(vchiq_userdrv_resume);

/*
 * Due to the limitations at the RTL level, there are some GPIO pins that
 * cannot be muxed to the videocore. As a workaround, we are using a combination
 * of the IPC doorbells and shared memory to communicate between the host and
 * videocore to control the pins that are muxed to the host.
 *
 * For the time being, we are piggybacking off of the sharedmem driver because
 * it is the fastest way. The long term solution would see the code that deals
 * with the GPIO moved to a separate service/driver.
 *
 * Operation details:
 *    - syncing:
 *         Before the videocore can request GPIO operations from the host, the
 *         videocore needs to make sure the host is up first. When the videocore
 *         boots up, it rings an IPC doorbell. If the host is already up at
 *         that moment, it will ring the same IPC doorbell back. Upon receiving
 *         the doorbell, the videocore will know the host is up and ready.
 *         Before this time, the videocore will return failure on any host GPIO
 *         requests.
 *
 *         To cover the scenario where the videocore comes up before the host,
 *         the host will automatically ring the IPC doorbell to notify the
 *         videocore.
 *
 *    - setting/clearing:
 *         1. Videocore writes GPIO pin to be set/clear into the shared memory
 *            at GPIO_MAILBOX_WRITE. If performing a set, the value is also
 *            bitwise OR'd with GPIO_MAILBOX_WRITE_SET.
 *         2. Videocore rings the IPC doorbell and waits for a reply.
 *         3. Host answers the doorbell and sets/clears the GPIO pin.
 *         4. Host rings the same IPC doorbell to notify the videocore.
 *         5. Videocore gets the doorbell and returns to the user.
 *
 *    - reading:
 *         1. Videocore writes GPIO pin to be read into the shared memory at
 *            GPIO_MAILBOX_READ.
 *         2. Videocore rings the IPC doorbell and waits for a reply.
 *         3. Host answers the doorbell and reads the value of the GPIO and
 *            stores it back into the shared memory at GPIO_MAILBOX_READ.
 *         4. Host rings the same IPC doorbell to notify the videocore.
 *         5. Videocore gets the doorbell and reads out the value of the GPIO
 *            from the shared memory and returns it to the user.
 */
static void
service_gpio(uint32_t irq_status)
{
	uint32_t gpio_mailbox_write =
		(*(uint32_t *)(g_vchiq_ipc_shared_mem +
			IPC_SHARED_MEM_CHANNEL_ARM_OFFSET +
			IPC_SHARED_MEM_GPIO_WRITE_OFFSET));
	uint32_t gpio_mailbox_read  =
		(*(uint32_t *)(g_vchiq_ipc_shared_mem +
			IPC_SHARED_MEM_CHANNEL_ARM_OFFSET +
			IPC_SHARED_MEM_GPIO_READ_OFFSET));

#define GPIO_MAILBOX_WRITE_SET        (0x80000000)
#define GPIO_MAILBOX_WRITE_PIN_MASK   (0x7FFFFFFF)

	if (irq_status &
		(IPC_INTERRUPT_STATUS_ENABLED << IPC_INTERRUPT_SOURCE_2)) {
		uint32_t reg = gpio_mailbox_write;

		if (reg & GPIO_MAILBOX_WRITE_SET)
			/* GPIO set */
			gpio_set_value(reg & GPIO_MAILBOX_WRITE_PIN_MASK , 1);
		else
			/* GPIO clear */
			gpio_set_value(reg & GPIO_MAILBOX_WRITE_PIN_MASK , 0);

		/* Notify videocore that GPIO has been changed */
		chal_ipc_int_vcset(ipcHandle, IPC_INTERRUPT_SOURCE_2);

		irq_status &= ~(IPC_INTERRUPT_STATUS_ENABLED <<
			IPC_INTERRUPT_SOURCE_2);
	}

	if (irq_status &
		(IPC_INTERRUPT_STATUS_ENABLED << IPC_INTERRUPT_SOURCE_3)) {
		gpio_mailbox_read = gpio_get_value(gpio_mailbox_read);
		/* GPIO set */

		/* Notify videocore that GPIO has been set */
		chal_ipc_int_vcset(ipcHandle, IPC_INTERRUPT_SOURCE_3);

		irq_status &= ~(IPC_INTERRUPT_STATUS_ENABLED <<
			IPC_INTERRUPT_SOURCE_3);
	}

	if (irq_status & (IPC_INTERRUPT_STATUS_ENABLED <<
		IPC_INTERRUPT_SOURCE_4)) {
		/* Reply back to the videocore to tell them we are ready */
		chal_ipc_int_vcset(ipcHandle, IPC_INTERRUPT_SOURCE_4);

		irq_status &= ~(IPC_INTERRUPT_STATUS_ENABLED <<
			IPC_INTERRUPT_SOURCE_4);
	}
}

/*
 * Local functions
 */

#define VCHIQ_IPC_MASK (\
	IPC_INTERRUPT_SOURCE_4 | \
	IPC_INTERRUPT_SOURCE_3 | \
	IPC_INTERRUPT_SOURCE_2 | \
	IPC_INTERRUPT_SOURCE_1 | \
	IPC_INTERRUPT_SOURCE_0)

#define VCHIQ_IPC_SOURCE_MAX (IPC_INTERRUPT_SOURCE_4+1)

static irqreturn_t
vchiq_doorbell_irq(int irq, void *dev_id)
{
	IPC_INTERRUPT_SOURCE source;
	uint32_t             status;
	VCHIQ_STATE_T       *state = dev_id;

	/* get the interrupt status value */
	chal_ipc_get_int_status(ipcHandle, &status);

	if ((status & VCHIQ_IPC_MASK) == 0)
		return IRQ_NONE;

	/* clear all the interrupts first */
	for (source = IPC_INTERRUPT_SOURCE_0;
		source < VCHIQ_IPC_SOURCE_MAX; source++) {
		if (status & (IPC_INTERRUPT_STATUS_ENABLED << source))
			chal_ipc_int_clr(ipcHandle, source);
	}

	if (status & (IPC_INTERRUPT_STATUS_ENABLED << IPC_INTERRUPT_SOURCE_0))
		/* signal the stack that there is something to pick up */
		remote_event_pollall(state);
	else if ((status & (IPC_INTERRUPT_STATUS_ENABLED <<
				IPC_INTERRUPT_SOURCE_2)) ||
				(status & (IPC_INTERRUPT_STATUS_ENABLED <<
					IPC_INTERRUPT_SOURCE_3)) ||
				(status & (IPC_INTERRUPT_STATUS_ENABLED <<
					IPC_INTERRUPT_SOURCE_4)))
		/* this is a GPIO request */
		service_gpio(status);

	return (status & ~VCHIQ_IPC_MASK) ? IRQ_NONE : IRQ_HANDLED;
}

static int
read_vc_debug_var(VC_MEM_ACCESS_HANDLE_T handle,
	const char *symbol,
	void *buf, size_t bufsize)
{
	VC_MEM_ADDR_T vcMemAddr;
	size_t vcMemSize;
	uint8_t *mapAddr;
	off_t  vcMapAddr;

	if (!LookupVideoCoreSymbol(handle, symbol,
		&vcMemAddr,
		&vcMemSize)) {
		vchiq_loud_error_header();
		vchiq_loud_error(
			"failed to find VC symbol \"%s\".",
			symbol);
		vchiq_loud_error_footer();
		return 0;
	}

	if (vcMemSize != bufsize) {
		vchiq_loud_error_header();
		vchiq_loud_error(
			"VC symbol \"%s\" is the wrong size.",
			symbol);
		vchiq_loud_error_footer();
		return 0;
	}

	vcMapAddr = (off_t)vcMemAddr & VC_MEM_TO_ARM_ADDR_MASK;
	vcMapAddr += mm_vc_mem_phys_addr;
	mapAddr = ioremap_nocache(vcMapAddr, vcMemSize);
	if (mapAddr == 0) {
		vchiq_loud_error_header();
		vchiq_loud_error(
			"failed to ioremap \"%s\" @ 0x%x "
			"(phys: 0x%x, size: %u).",
			VCHIQ_IPC_SHARED_MEM_SIZE_SYMBOL,
			(unsigned int)vcMapAddr,
			(unsigned int)vcMemAddr,
			(unsigned int)vcMemSize);
		vchiq_loud_error_footer();
		return 0;
	}

	memcpy(buf, mapAddr, bufsize);
	iounmap(mapAddr);

	return 1;
}

/****************************************************************************
*
*   vchiq_memdrv_initialise
*
***************************************************************************/

VCHIQ_STATUS_T vchiq_memdrv_initialise(void)
{
	VCHIQ_STATE_T *state;
	VCHIQ_STATUS_T status;
	int err = 0;
	int i;

	if (g_initialized) {
		vchiq_log_warning(vchiq_arm_log_level,
			"%s: already initialized", __func__);
		return VCHIQ_SUCCESS;
	}

#if defined(VCHIQ_SM_ALLOC_VCDDR)
	VC_MEM_ADDR_T vcMemAddr;
	size_t vcMemSize;
	uint8_t *mapAddr;
	off_t  vcMapAddr;
	uint32_t boot_state = 0;
	uint32_t boot_state_info = 0;

	g_vchiq_ipc_shared_mem_size = 0;

	if (OpenVideoCoreMemory(&g_vchiq_mem_hndl) == 0) {

		if (!read_vc_debug_var(g_vchiq_mem_hndl,
			VCHIQ_IPC_SHARED_MEM_SYMBOL,
			&g_vchiq_ipc_shared_mem_addr,
			sizeof(g_vchiq_ipc_shared_mem_addr)) ||
			!read_vc_debug_var(g_vchiq_mem_hndl,
				VCHIQ_IPC_SHARED_MEM_SIZE_SYMBOL,
				&g_vchiq_ipc_shared_mem_size,
				sizeof(g_vchiq_ipc_shared_mem_size))) {
			vchiq_loud_error_header();
			vchiq_loud_error("VideoCore boot failed - "
				"invalid image?");
			vchiq_loud_error_footer();
		} else if (!g_vchiq_ipc_shared_mem_size ||
			!g_vchiq_ipc_shared_mem_addr) {
			vchiq_loud_error_header();
			vchiq_loud_error("VideoCore boot failed");
			vchiq_loud_error("VCHIQ shared memory buffer not set");
			if (LookupVideoCoreSymbol(g_vchiq_mem_hndl,
				"boot_state",
				&vcMemAddr,
				&vcMemSize) &&
				read_vc_debug_var(g_vchiq_mem_hndl,
					"boot_state",
					&boot_state, sizeof(boot_state)) &&
				read_vc_debug_var(g_vchiq_mem_hndl,
					"boot_state_info",
					&boot_state_info,
					sizeof(boot_state_info))) {

				vchiq_loud_error("Boot state: %.4s (0x%x) "
						" Info: 0x%x",
					(char *)&boot_state,
					boot_state,
					boot_state_info);
			} else
				vchiq_loud_error("Boot state: unknown "
					"(old image)");
			vchiq_loud_error_footer();
		} else {
			vcMapAddr = (off_t)g_vchiq_ipc_shared_mem_addr &
				VC_MEM_TO_ARM_ADDR_MASK;
			vcMapAddr = vcMapAddr + mm_vc_mem_phys_addr;
			mapAddr = ioremap_nocache(vcMapAddr,
				(size_t)g_vchiq_ipc_shared_mem_size);
			if (mapAddr != 0) {
				g_vchiq_ipc_shared_mem = mapAddr;
				/* Do not **iounmap** at this time, we can now
				** use the shared memory mapped.
				*/
			} else {
				vchiq_loud_error_header();
				vchiq_loud_error(
					"Failed to ioremap shared memory "
					"region for vchiq @ 0x%x "
					"(phys: 0x%x, size: %u).",
					(unsigned int)vcMapAddr,
					(unsigned int)
						g_vchiq_ipc_shared_mem_addr,
					(unsigned int)
						g_vchiq_ipc_shared_mem_size);
				vchiq_loud_error_footer();
			}
		}

		CloseVideoCoreMemory(g_vchiq_mem_hndl);
	} else {
		vchiq_loud_error_header();
		vchiq_loud_error("Failed to open VideoCore memory.");
		vchiq_loud_error_footer();
	}

	if ((g_vchiq_ipc_shared_mem_size == 0) ||
		(g_vchiq_ipc_shared_mem == NULL))
		return VCHIQ_ERROR;

	vchiq_log_info(vchiq_arm_log_level,
		"VideoCore allocated %u (0x%x) bytes of shared memory.\n",
		g_vchiq_ipc_shared_mem_size,
		g_vchiq_ipc_shared_mem_size);
	vchiq_log_info(vchiq_arm_log_level,
		"Shared memory (0x%x) mapped @ 0x%p for kernel usage.\n",
		(unsigned int)g_vchiq_ipc_shared_mem_addr,
		g_vchiq_ipc_shared_mem);
#else
	g_vchiq_ipc_shared_mem = IPC_SHARED_MEM_SLOTS_VIRT;
	g_vchiq_ipc_shared_mem_size = IPC_SHARED_MEM_SLOTS_SIZE;
#endif

	vchiq_log_warning(vchiq_arm_log_level,
		"%s: ipc shared memory address                       = 0x%p",
		__func__, g_vchiq_ipc_shared_mem);
	vchiq_log_warning(vchiq_arm_log_level,
		"%s: ipc shared memory size (vc+arm channels, extra) = 0x%x",
		__func__, g_vchiq_ipc_shared_mem_size);
	vchiq_log_warning(vchiq_arm_log_level,
		"%s: VCHIQ_MAX_SERVICES        = %d",
		__func__, VCHIQ_MAX_SERVICES);

	g_vchiq_slot_zero = (VCHIQ_SLOT_ZERO_T *)g_vchiq_ipc_shared_mem;
	state = g_vchiq_state;

	status = vchiq_platform_deferred_init(state, g_vchiq_slot_zero);

	if (status != VCHIQ_SUCCESS) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: vchiq_init_state failed",
			__func__);
		goto failed_init_state;
	}

	ipcHandle = chal_ipc_config(NULL);
	chal_icd_set_security(0, VCHIQ_DOORBELL_IRQ, eINT_STATE_SECURE);
	for (i = 0; i < IPC_INTERRUPT_SOURCE_MAX; i++)
		chal_ipc_int_secmode(ipcHandle, i, IPC_INTERRUPT_MODE_OPEN);

	/* clear all interrupts */
	for (i = 0; i < IPC_INTERRUPT_SOURCE_MAX; i++)
		chal_ipc_int_clr(ipcHandle, i);

	err = request_irq(VCHIQ_DOORBELL_IRQ, vchiq_doorbell_irq,
		IRQF_DISABLED | IRQF_SHARED, "IPC driver", state);
	if (err != 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: failed to register irq=%d err=%d",
			__func__,
			VCHIQ_DOORBELL_IRQ, err);
		goto failed_request_irq;
	} else
		/* Tell the videocore we are ready for servicing GPIO
		** requests */
		chal_ipc_int_vcset(ipcHandle, IPC_INTERRUPT_SOURCE_4);

	g_initialized = 1;

	vchiq_call_connected_callbacks();

	return VCHIQ_SUCCESS;

failed_request_irq:
failed_init_state:
	return VCHIQ_ERROR;
}
EXPORT_SYMBOL(vchiq_memdrv_initialise);
