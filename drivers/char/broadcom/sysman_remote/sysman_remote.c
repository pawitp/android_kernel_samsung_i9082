/*****************************************************************************
 * Copyright 2012 Broadcom Corporation.  All rights reserved.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>

#include "vchiq.h"
#include "vchiq_connected.h"

#define DRIVER_NAME  "sysman"

#define ENABLE_LDO_OFF_DURING_SUSPEND

/*#define ENABLE_LOG_DBG*/

#define SYSMAN_VCHI_FOURCC VCHIQ_MAKE_FOURCC('S', 'M', 'A', 'N')
#define SYSMAN_MAX_REMOTE_DOMAINS 16

/* max length of the regulator supply name (including \0 terminator) */
#define MAX_REMOTE_NAME_LEN 16

#ifdef ENABLE_LOG_DBG
#define LOG_DBG(fmt, arg...)  printk(KERN_INFO "[D] " fmt "\n", ##arg)
#else
#define LOG_DBG(fmt, arg...)
#endif
#define LOG_INFO(fmt, arg...)  printk(KERN_INFO "[I] " fmt "\n", ##arg)
#define LOG_ERR(fmt, arg...)  printk(KERN_ERR  "[E] " fmt "\n", ##arg)

static int vc_sysman_remote_probe(struct platform_device *p_dev);
static int __devexit vc_sysman_remote_remove(struct platform_device *p_dev);
#ifdef ENABLE_LDO_OFF_DURING_SUSPEND
static int vc_sysman_remote_suspend(struct platform_device *p_dev,
				    pm_message_t state);
static int vc_sysman_remote_resume(struct platform_device *p_dev);
#endif
static int vc_sysman_proc_read(char *buf, char **start, off_t offset, int count,
			       int *eof, void *data);
static int vc_sysman_proc_write(struct file *file, const char __user *buffer,
				unsigned long count, void *data);

static DEFINE_MUTEX(sysman_state_mutex);

/* Structures exporting functions. */
static struct platform_driver vc_sysman_remote_driver = {
	.probe = vc_sysman_remote_probe,
	.remove = __devexit_p(vc_sysman_remote_remove),
#ifdef ENABLE_LDO_OFF_DURING_SUSPEND
	.suspend = vc_sysman_remote_suspend,
	.resume = vc_sysman_remote_resume,
#endif
	.driver = {
		   .name = "vc-sysman-remote"}
};

struct remote_sysman_reg_state {
	char name[MAX_REMOTE_NAME_LEN];
	struct regulator *reg;
	int reg_request_count;	/* reference count (not expected to go above 1) */
};

struct remote_sysman_state {
	VCHIQ_SERVICE_HANDLE_T service_handle;
	VCHIQ_INSTANCE_T initialise_instance;
	struct platform_device *p_dev;
	struct remote_sysman_reg_state reg_states[SYSMAN_MAX_REMOTE_DOMAINS];
	struct proc_dir_entry *proc_entry;
};

static struct remote_sysman_state *remote_sm_stt;

/*
 * Function to find index of already acquired register, or store info on new
 * register and acquire it.
 */
static int get_reg_idx(struct device *dev,
		       struct remote_sysman_reg_state *reg_state,
		       const char *reg_name)
{
	int i = 0;
	int ret = -1;
	for (i = 0; i < SYSMAN_MAX_REMOTE_DOMAINS; i++) {
		if (strnicmp(reg_name, reg_state[i].name,
			     (MAX_REMOTE_NAME_LEN - 1)) == 0) {
			ret = i;
			break;
		}
		if ((0 > ret) && (0 == strlen(reg_state[i].name)))
			ret = i;
	}

	if (ret >= 0 && !reg_state[ret].reg) {
		struct regulator *reg;
		LOG_DBG("%s: Getting regulator %s", __func__, reg_name);
		reg = regulator_get(dev, reg_name);
		if (IS_ERR(reg)) {
			ret = -1;
		} else {
			strlcpy(reg_state[ret].name, reg_name,
				MAX_REMOTE_NAME_LEN);
			reg_state[ret].reg = reg;
			reg_state[ret].reg_request_count = 0;
		}
	}

	return ret;
}

static VCHIQ_STATUS_T change_regulator_state(struct remote_sysman_state *state,
					     const char *name, uint32_t enable)
{
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
	int reg_idx = get_reg_idx(&state->p_dev->dev, state->reg_states, name);

	if (reg_idx < 0) {
		status = VCHIQ_ERROR;
		LOG_ERR("%s: ERROR: Failed to get regulator for %s",
			__func__, name);
		goto out;
	}

	if (enable) {
		if (!(state->reg_states[reg_idx].reg_request_count++)) {
			LOG_DBG("%s: calling regulator_enable for %s",
				__func__, state->reg_states[reg_idx].name);
			regulator_enable(state->reg_states[reg_idx].reg);
		} else {
			LOG_DBG("%s: regulator already enabled for %s count %d",
				__func__, state->reg_states[reg_idx].name,
				state->reg_states[reg_idx].reg_request_count);
		}
	} else {
		if (!(--state->reg_states[reg_idx].reg_request_count)) {
			LOG_DBG("%s: calling regulator_disable for %s",
				__func__, state->reg_states[reg_idx].name);
#if 0 //defined(CONFIG_LCD_POWER_CAMLDO2)
			if (!strcmp(state->reg_states[reg_idx].name, "camldo2_uc")) 
				goto out;
#endif				
			regulator_disable(state->reg_states[reg_idx].reg);
			/* TBD - remove disabled regulators from array and
			 * call regulator_put?? */
		} else {
			LOG_DBG("%s: not disabling regulator for %s count %d",
				__func__, state->reg_states[reg_idx].name,
				state->reg_states[reg_idx].reg_request_count);
		}
	}

out:
	return status;
}

static VCHIQ_STATUS_T process_message(VCHIQ_HEADER_T * vchiq_header,
				      VCHIQ_SERVICE_HANDLE_T service,
				      struct remote_sysman_state *state)
{
	uint32_t required = *(uint32_t *)(vchiq_header->data);
	char *txt = vchiq_header->data + (sizeof(uint32_t));

	/* message is finished with, return it to vchiq */
	vchiq_release_message(service, vchiq_header);

	return change_regulator_state(state, txt, required);
}

static VCHIQ_STATUS_T sysman_remote_callback(VCHIQ_REASON_T reason,
					     VCHIQ_HEADER_T * vchiq_header,
					     VCHIQ_SERVICE_HANDLE_T service,
					     void *context)
{
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
	struct remote_sysman_state *state = VCHIQ_GET_SERVICE_USERDATA(service);

	switch (reason) {
	case VCHIQ_MESSAGE_AVAILABLE:
		{
			VCHIQ_ELEMENT_T msg;
			int status = VCHIQ_RETRY;

			mutex_lock(&sysman_state_mutex);
			vchiq_use_service(service);

			if (state && state->p_dev &&
			    vchiq_header && vchiq_header->size) {
				status =
				    process_message(vchiq_header, service,
						    state);
			}

			/* send status response to videocore */
			msg.data = &status;
			msg.size = sizeof(status);
			vchiq_queue_message(service, &msg, 1);

			vchiq_release_service(service);
			mutex_unlock(&sysman_state_mutex);
		}
		break;
	default:
		break;
	}

	return status;
}

static void vc_sysman_remote_connected_init(void)
{
	int ret = 0;
	VCHIQ_SERVICE_PARAMS_T vchiq_params = { 0 };

	LOG_INFO("%s: start", __func__);

	/* Initialize and create a VCHIQ connection */
	ret = vchiq_initialise(&remote_sm_stt->initialise_instance);
	if (ret != 0) {
		LOG_ERR("%s: failed to initialise VCHIQ instance (ret=%d)",
			__func__, ret);
		ret = -EIO;
		goto out;
	}
	ret = vchiq_connect(remote_sm_stt->initialise_instance);
	if (ret != 0) {
		LOG_ERR("%s: failed to connect VCHIQ instance (ret=%d)",
			__func__, ret);
		ret = -EIO;
		goto out;
	}

	vchiq_params.fourcc = SYSMAN_VCHI_FOURCC;
	vchiq_params.callback = sysman_remote_callback;
	vchiq_params.userdata = remote_sm_stt;
	vchiq_params.version = 1;
	vchiq_params.version_min = 0;

	ret = vchiq_open_service(remote_sm_stt->initialise_instance,
				 &vchiq_params, &remote_sm_stt->service_handle);
	if (ret != 0 || (remote_sm_stt->service_handle == 0)) {
		LOG_ERR("%s: failed to add SMAN service: error %d",
			__func__, ret);
		ret = -EPERM;
		goto out;
	}

	vchiq_release_service(remote_sm_stt->service_handle);

out:
	LOG_INFO("%s: end (ret=%d)", __func__, ret);
}

#ifdef ENABLE_LDO_OFF_DURING_SUSPEND
/* Temporarily release any claimed regulators on system suspend */
static int vc_sysman_remote_suspend(struct platform_device *p_dev,
				    pm_message_t state)
{
	int i;
	LOG_DBG("%s: state.event %d", __func__, state.event);
	mutex_lock(&sysman_state_mutex);

	for (i = 0; i < SYSMAN_MAX_REMOTE_DOMAINS; i++) {
		if (remote_sm_stt->reg_states[i].reg &&
		    remote_sm_stt->reg_states[i].reg_request_count) {
			LOG_DBG("%s: disabling reg %s before suspend", __func__,
				remote_sm_stt->reg_states[i].name);
			regulator_disable(remote_sm_stt->reg_states[i].reg);
		}
	}
	/* (no mutex_unlock - lock structure until resume) */

	return 0;
}

/* Restore any regulators released on system suspend */
static int vc_sysman_remote_resume(struct platform_device *p_dev)
{
	int i;
	LOG_DBG("%s", __func__);

	/* (no mutex_lock - lock obtained in suspend) */
	for (i = 0; i < SYSMAN_MAX_REMOTE_DOMAINS; i++) {
		if (remote_sm_stt->reg_states[i].reg &&
		    remote_sm_stt->reg_states[i].reg_request_count) {
			LOG_DBG("%s: restoring reg %s state", __func__,
				remote_sm_stt->reg_states[i].name);
			regulator_enable(remote_sm_stt->reg_states[i].reg);
		}
	}
	mutex_unlock(&sysman_state_mutex);

	return 0;
}
#endif

static int vc_sysman_remote_probe(struct platform_device *p_dev)
{
	int ret = 0;
	LOG_INFO("%s: start", __func__);
	remote_sm_stt = kzalloc(sizeof(struct remote_sysman_state), GFP_KERNEL);
	if (!remote_sm_stt) {
		ret = -ENOMEM;
		goto exit;
	}

	remote_sm_stt->p_dev = p_dev;

	remote_sm_stt->proc_entry = create_proc_entry(DRIVER_NAME, 0444, NULL);
	if (remote_sm_stt->proc_entry == NULL) {
		ret = -EFAULT;
		goto out_efault;
	}
	remote_sm_stt->proc_entry->data = (void *)remote_sm_stt;
	remote_sm_stt->proc_entry->read_proc = vc_sysman_proc_read;
	remote_sm_stt->proc_entry->write_proc = vc_sysman_proc_write;

	vchiq_add_connected_callback(vc_sysman_remote_connected_init);

	goto exit;

out_efault:
	kfree(remote_sm_stt);
	remote_sm_stt = NULL;
exit:
	LOG_INFO("%s: end, ret=%d", __func__, ret);
	return ret;
}

static int vc_sysman_proc_read(char *buf,
			       char **start,
			       off_t offset, int count, int *eof, void *data)
{
	char *p = buf;
	struct remote_sysman_state *state = (struct remote_sysman_state *)data;
	int index = 0;

	(void)start;
	(void)count;

	if (offset > 0) {
		*eof = 1;
		return 0;
	}

	for (index = 0; index < SYSMAN_MAX_REMOTE_DOMAINS; index++) {
		p += sprintf(p, "Reg: %02d", index);
		p += sprintf(p, " Name: %16s", state->reg_states[index].name);
		p += sprintf(p, " Address: %8p", state->reg_states[index].reg);
		p += sprintf(p, " Request count: %d\n",
			     state->reg_states[index].reg_request_count);
	}

	*eof = 1;
	return p - buf;
}

static int vc_sysman_proc_write(struct file *file,
				const char __user *buffer,
				unsigned long count, void *data)
{
	int rc = count;
	char input_str[MAX_REMOTE_NAME_LEN << 1];
	struct remote_sysman_state *state = (struct remote_sysman_state *)data;
	int enable = -1;
	char *reg_name = input_str;

	memset(input_str, 0, sizeof(input_str));

	if (count > sizeof(input_str)) {
		LOG_ERR("%s: input string length too long", __func__);
		rc = -EFAULT;
		goto out;
	}

	if (copy_from_user(input_str, buffer, count - 1)) {
		LOG_ERR("%s: failed to get input string", __func__);
		rc = -EFAULT;
		goto out;
	}

	if (strnicmp(input_str, "enable", strlen("enable")) == 0)
		enable = 1;
	else if (strnicmp(input_str, "disable", strlen("disable")) == 0)
		enable = 0;

	if (0 <= enable) {
		strsep(&reg_name, " ");
		change_regulator_state(state, reg_name, enable);
	}

out:
	return rc;
}

static int __devexit vc_sysman_remote_remove(struct platform_device *p_dev)
{
	int i = 0;
	if (!remote_sm_stt)
		goto out;

	remove_proc_entry(remote_sm_stt->proc_entry->name, NULL);
	for (i = 0; i < SYSMAN_MAX_REMOTE_DOMAINS; i++) {
		/* release any claimed regulators */
		struct regulator *reg = remote_sm_stt->reg_states[i].reg;
		if (reg) {
			if (remote_sm_stt->reg_states[i].reg_request_count)
				regulator_disable(reg);
			regulator_put(reg);
		}
	}
	if (remote_sm_stt->service_handle) {
		vchiq_use_service(remote_sm_stt->service_handle);
		vchiq_close_service(remote_sm_stt->service_handle);
		vchiq_shutdown(remote_sm_stt->initialise_instance);
	}
	kfree(remote_sm_stt);
	remote_sm_stt = NULL;

out:
	return 0;
}

static int __init vc_sysman_remote_init(void)
{
	int ret;
	LOG_INFO("vc-sysman-remote: Videocore remote sysman driver\n");

	ret = platform_driver_register(&vc_sysman_remote_driver);
	if (ret) {
		LOG_ERR("%s : Unable to register VC remote sysman driver (%d)",
			__func__, ret);
	} else {
		LOG_INFO("%s : Registered Videocore remote sysman driver",
			 __func__);
	}

	return ret;
}

static void __exit vc_sysman_remote_exit(void)
{
	LOG_INFO("%s: start", __func__);

	platform_driver_unregister(&vc_sysman_remote_driver);

	LOG_INFO("%s: end", __func__);
}

subsys_initcall(vc_sysman_remote_init);
module_exit(vc_sysman_remote_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("VC Remote Sysman Driver");
MODULE_LICENSE("GPL");
