/*****************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
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

/* ---- Include Files ---------------------------------------------------- */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "vc_vchi_fb.h"
#define VC_FB_VER       2
#define VC_FB_VER_MIN   0

/* ---- Private Constants and Types -------------------------------------- */

/* Logging macros */
#define LOG_ERR(fmt, ...)   printk(KERN_ERR     fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  printk(KERN_WARNING fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  printk(KERN_INFO    fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   printk(KERN_DEBUG   fmt "\n", ##__VA_ARGS__)

typedef struct opaque_vc_vchi_fb_handle_t {
	uint32_t num_connections;
	VCHI_SERVICE_HANDLE_T vchi_handle[VCHI_MAX_NUM_CONNECTIONS];
	struct semaphore msg_avail;
	struct mutex vchi_mutex;

	uint8_t msg_buf[VC_FB_MAX_MSG_LEN];
} FB_INSTANCE_T;

/* ---- Private Variables ------------------------------------------------ */

/* ---- Private Function Prototypes -------------------------------------- */

/* ---- Private Functions ------------------------------------------------ */

static void fb_vchi_callback(void *param,
			     const VCHI_CALLBACK_REASON_T reason,
			     void *msg_handle)
{
	FB_INSTANCE_T *instance = (FB_INSTANCE_T *) param;

	(void)msg_handle;

	if (reason != VCHI_CALLBACK_MSG_AVAILABLE)
		return;

	up(&instance->msg_avail);
}

VC_VCHI_FB_HANDLE_T vc_vchi_fb_init(VCHI_INSTANCE_T vchi_instance,
				    VCHI_CONNECTION_T **vchi_connections,
				    uint32_t num_connections)
{
	uint32_t i;
	FB_INSTANCE_T *instance;
	int32_t status;

	LOG_DBG("%s: start", __func__);

	if (num_connections > VCHI_MAX_NUM_CONNECTIONS) {
		LOG_ERR("%s: unsupported number of connections %u (max=%u)",
			__func__, num_connections, VCHI_MAX_NUM_CONNECTIONS);

		return NULL;
	}
	/* Allocate memory for this instance */
	instance = kzalloc(sizeof(*instance), GFP_KERNEL);
	memset(instance, 0, sizeof(*instance));

	instance->num_connections = num_connections;

	/* Create the message available semaphore */
	sema_init(&instance->msg_avail, 0);

	/* Create a lock for exclusive, serialized VCHI connection access */
	mutex_init(&instance->vchi_mutex);

	/* Open the VCHI service connections */
	for (i = 0; i < num_connections; i++) {
		SERVICE_CREATION_T params = {
			VCHI_VERSION_EX(VC_FB_VER, VC_FB_VER_MIN),
			VC_FB_SERVER_NAME,
			vchi_connections[i],
			0,
			0,
			fb_vchi_callback,
			instance,
			0,
			0,
			0
		};

		status = vchi_service_open(vchi_instance, &params,
					   &instance->vchi_handle[i]);
		if (status) {
			LOG_ERR
			    ("%s: failed to open VCHI service (%d)",
			     __func__, status);

			goto err_close_services;
		}
		/* Finished with the service for now */
		vchi_service_release(instance->vchi_handle[i]);
	}

	return instance;

err_close_services:
	for (i = 0; i < instance->num_connections; i++)
		vchi_service_close(instance->vchi_handle[i]);

	kfree(instance);

	return NULL;
}

int32_t vc_vchi_fb_stop(VC_VCHI_FB_HANDLE_T *handle)
{
	FB_INSTANCE_T *instance;
	uint32_t i;

	if (handle == NULL) {
		LOG_ERR("%s: invalid pointer to handle %p", __func__, handle);

		return -1;
	}

	if (*handle == NULL) {
		LOG_ERR("%s: invalid handle %p", __func__, *handle);

		return -1;
	}

	instance = *handle;

	mutex_lock(&instance->vchi_mutex);

	/* Close all VCHI service connections */
	for (i = 0; i < instance->num_connections; i++) {
		int32_t success;
		vchi_service_use(instance->vchi_handle[i]);

		success = vchi_service_close(instance->vchi_handle[i]);
	}

	mutex_unlock(&instance->vchi_mutex);

	kfree(instance);

	/* NULLify the handle to prevent the user from using it */
	*handle = NULL;

	return 0;
}

int32_t vc_vchi_fb_get_scrn_info(VC_VCHI_FB_HANDLE_T handle,
				 VC_FB_SCRN scrn, VC_FB_SCRN_INFO_T *info)
{
	int ret;
	FB_INSTANCE_T *instance = handle;
	int32_t success;
	uint32_t msg_len;
	VC_FB_MSG_HDR_T *msg_hdr;
	VC_FB_GET_SCRN_INFO_T *get_scrn_info;

	if (handle == NULL) {
		LOG_ERR("%s: invalid handle %p", __func__, handle);

		return -1;
	}

	if (scrn >= VC_FB_SCRN_MAX) {
		LOG_ERR("%s: invalid screen %u", __func__, scrn);

		return -1;
	}

	if (info == NULL) {
		LOG_ERR("%s: invalid info pointer %p", __func__, info);

		return -1;
	}

	mutex_lock(&instance->vchi_mutex);
	vchi_service_use(instance->vchi_handle[0]);

	msg_len = sizeof(*msg_hdr) + sizeof(*get_scrn_info);
	memset(instance->msg_buf, 0, msg_len);

	msg_hdr = (VC_FB_MSG_HDR_T *) instance->msg_buf;
	msg_hdr->type = VC_FB_MSG_TYPE_GET_SCRN_INFO;

	get_scrn_info = (VC_FB_GET_SCRN_INFO_T *) msg_hdr->body;
	get_scrn_info->scrn = scrn;

	/* Send the message to the videocore */
	success = vchi_msg_queue(instance->vchi_handle[0],
				 instance->msg_buf, msg_len,
				 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (success != 0) {
		LOG_ERR("%s: failed to queue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	}
	/* We are expecting a reply from the videocore */
	down(&instance->msg_avail);

	success = vchi_msg_dequeue(instance->vchi_handle[0],
				   info, sizeof(*info),
				   &msg_len, VCHI_FLAGS_NONE);
	if (success != 0) {
		LOG_ERR("%s: failed to dequeue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	} else if (msg_len != sizeof(*info)) {
		LOG_ERR("%s: incorrect message length %u (expected=%u)",
			__func__, msg_len, sizeof(*info));

		ret = -1;
		goto unlock;
	}

	ret = 0;

unlock:
	vchi_service_release(instance->vchi_handle[0]);
	mutex_unlock(&instance->vchi_mutex);

	return ret;
}

int32_t vc_vchi_fb_alloc(VC_VCHI_FB_HANDLE_T handle,
			 VC_FB_ALLOC_T *alloc,
			 VC_FB_ALLOC_RESULT_T *alloc_result)
{
	FB_INSTANCE_T *instance = handle;
	int32_t success;
	uint32_t msg_len;
	VC_FB_MSG_HDR_T *msg_hdr;
	int32_t ret = -1;

	if (handle == NULL) {
		LOG_ERR("%s: invalid handle %p", __func__, handle);

		return -1;
	}

	if (alloc == NULL) {
		LOG_ERR("%s: invalid alloc pointer %p", __func__, alloc);

		return -1;
	}
	/* TODO check individual alloc member values */

	if (alloc_result == NULL) {
		LOG_ERR("%s: invalid alloc_result pointer 0x%p", __func__,
			alloc_result);

		return -1;
	}

	mutex_lock(&instance->vchi_mutex);
	vchi_service_use(instance->vchi_handle[0]);

	msg_len = sizeof(*msg_hdr) + sizeof(*alloc);
	memset(instance->msg_buf, 0, msg_len);

	msg_hdr = (VC_FB_MSG_HDR_T *) instance->msg_buf;
	msg_hdr->type = VC_FB_MSG_TYPE_ALLOC;

	/* Copy the user buffer into the message buffer */
	memcpy(msg_hdr->body, alloc, sizeof(*alloc));

	/* Send the message to the videocore */
	success = vchi_msg_queue(instance->vchi_handle[0],
				 instance->msg_buf, msg_len,
				 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (success != 0) {
		LOG_ERR("%s: failed to queue message (success=%d)",
			__func__, success);

		goto unlock;
	}
	/*  We are expecting a reply from the videocore */
	down(&instance->msg_avail);

	success = vchi_msg_dequeue(instance->vchi_handle[0],
				   alloc_result, sizeof(*alloc_result),
				   &msg_len, VCHI_FLAGS_NONE);
	if (success != 0) {
		LOG_ERR("%s: failed to dequeue message (success=%d)",
			__func__, success);

		goto unlock;
	} else if (msg_len != sizeof(*alloc_result)) {
		LOG_ERR("%s: incorrect message length %u (expected=%u)",
			__func__, msg_len, sizeof(*alloc_result));

		goto unlock;
	}
	/* success if we got to here */
	ret = 0;

unlock:
	vchi_service_release(instance->vchi_handle[0]);
	mutex_unlock(&instance->vchi_mutex);

	return ret;
}

int32_t vc_vchi_fb_free(VC_VCHI_FB_HANDLE_T handle, uint32_t res_handle)
{
	int32_t ret;
	FB_INSTANCE_T *instance = handle;
	int32_t success;
	uint32_t msg_len;
	VC_FB_MSG_HDR_T *msg_hdr;
	VC_FB_FREE_T *free;
	VC_FB_RESULT_T result;

	if (handle == NULL) {
		LOG_ERR("%s: invalid handle %p", __func__, handle);

		return -1;
	}

	mutex_lock(&instance->vchi_mutex);
	vchi_service_use(instance->vchi_handle[0]);

	msg_len = sizeof(*msg_hdr) + sizeof(*free);
	memset(instance->msg_buf, 0, msg_len);

	msg_hdr = (VC_FB_MSG_HDR_T *) instance->msg_buf;
	msg_hdr->type = VC_FB_MSG_TYPE_FREE;

	free = (VC_FB_FREE_T *) msg_hdr->body;
	free->res_handle = res_handle;

	/* Send the message to the videocore */
	success = vchi_msg_queue(instance->vchi_handle[0],
				 instance->msg_buf, msg_len,
				 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (success != 0) {
		LOG_ERR("%s: failed to queue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	}
	/* We are expecting a reply from the videocore */
	down(&instance->msg_avail);

	success = vchi_msg_dequeue(instance->vchi_handle[0],
				   &result, sizeof(result),
				   &msg_len, VCHI_FLAGS_NONE);
	if (success != 0) {
		LOG_ERR("%s: failed to dequeue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	} else if (msg_len != sizeof(result)) {
		LOG_ERR("%s: incorrect message length %u (expected=%u)",
			__func__, msg_len, sizeof(result));

		ret = -1;
		goto unlock;
	}

	ret = result.success ? -1 : 0;

unlock:
	vchi_service_release(instance->vchi_handle[0]);
	mutex_unlock(&instance->vchi_mutex);

	return ret;
}

int32_t vc_vchi_fb_pan(VC_VCHI_FB_HANDLE_T handle,
		       uint32_t res_handle, uint32_t y_offset)
{
	int ret;
	FB_INSTANCE_T *instance = handle;
	int32_t success;
	uint32_t msg_len;
	VC_FB_MSG_HDR_T *msg_hdr;
	VC_FB_PAN_T *pan;
	VC_FB_RESULT_T result;

	if (handle == NULL) {
		LOG_ERR("%s: invalid handle %p", __func__, handle);

		return -1;
	}

	mutex_lock(&instance->vchi_mutex);
	vchi_service_use(instance->vchi_handle[0]);

	msg_len = sizeof(*msg_hdr) + sizeof(*pan);
	memset(instance->msg_buf, 0, msg_len);

	msg_hdr = (VC_FB_MSG_HDR_T *) instance->msg_buf;
	msg_hdr->type = VC_FB_MSG_TYPE_PAN;

	pan = (VC_FB_PAN_T *) msg_hdr->body;
	pan->res_handle = res_handle;
	pan->y_offset = y_offset;

	/* Send the message to the videocore */
	success = vchi_msg_queue(instance->vchi_handle[0],
				 instance->msg_buf, msg_len,
				 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (success != 0) {
		LOG_ERR("%s: failed to queue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	}
	/* We are expecting a reply from the videocore */
	down(&instance->msg_avail);

	success = vchi_msg_dequeue(instance->vchi_handle[0],
				   &result, sizeof(result),
				   &msg_len, VCHI_FLAGS_NONE);
	if (success != 0) {
		LOG_ERR("%s: failed to dequeue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	} else if (msg_len != sizeof(result)) {
		LOG_ERR("%s: incorrect message length %u (expected=%u)",
			__func__, msg_len, sizeof(result));

		ret = -1;
		goto unlock;
	}

	ret = result.success ? -1 : 0;

unlock:
	vchi_service_release(instance->vchi_handle[0]);
	mutex_unlock(&instance->vchi_mutex);

	return ret;
}

int32_t vc_vchi_fb_swap(VC_VCHI_FB_HANDLE_T handle,
			uint32_t res_handle, uint32_t active_frame)
{
	int ret;
	FB_INSTANCE_T *instance = handle;
	int32_t success;
	uint32_t msg_len;
	VC_FB_MSG_HDR_T *msg_hdr;
	VC_FB_SWAP_T *swap;
	VC_FB_RESULT_T result;

	if (handle == NULL) {
		LOG_ERR("%s: invalid handle %p", __func__, handle);

		return -1;
	}

	mutex_lock(&instance->vchi_mutex);
	vchi_service_use(instance->vchi_handle[0]);

	msg_len = sizeof(*msg_hdr) + sizeof(*swap);
	memset(instance->msg_buf, 0, msg_len);

	msg_hdr = (VC_FB_MSG_HDR_T *) instance->msg_buf;
	msg_hdr->type = VC_FB_MSG_TYPE_SWAP;

	swap = (VC_FB_SWAP_T *) msg_hdr->body;
	swap->res_handle = res_handle;
	swap->active_frame = active_frame;

	/* Send the message to the videocore */
	success = vchi_msg_queue(instance->vchi_handle[0],
				 instance->msg_buf, msg_len,
				 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (success != 0) {
		LOG_ERR("%s: failed to queue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	}
	/* We are expecting a reply from the videocore */
	down(&instance->msg_avail);

	success = vchi_msg_dequeue(instance->vchi_handle[0],
				   &result, sizeof(result),
				   &msg_len, VCHI_FLAGS_NONE);
	if (success != 0) {
		LOG_ERR("%s: failed to dequeue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	} else if (msg_len != sizeof(result)) {
		LOG_ERR("%s: incorrect message length %u (expected=%u)",
			__func__, msg_len, sizeof(result));

		ret = -1;
		goto unlock;
	}

	ret = result.success ? -1 : 0;

unlock:
	vchi_service_release(instance->vchi_handle[0]);
	mutex_unlock(&instance->vchi_mutex);

	return ret;
}

int32_t vc_vchi_fb_cfg(VC_VCHI_FB_HANDLE_T handle,
		       VC_FB_CFG_T *cfg, VC_FB_CFG_RESULT_T *cfg_result)
{
	int32_t ret;
	FB_INSTANCE_T *instance = handle;
	int32_t success;
	uint32_t msg_len;
	VC_FB_MSG_HDR_T *msg_hdr;

	if (handle == NULL) {
		LOG_ERR("%s: invalid handle", __func__);

		ret = -1;
		goto out;
	}

	if (cfg == NULL) {
		LOG_ERR("%s: invalid cfg pointer", __func__);

		ret = -1;
		goto out;
	}

	if (cfg_result == NULL) {
		LOG_ERR("%s: invalid cfg_result pointer", __func__);

		ret = -1;
		goto out;
	}

	mutex_lock(&instance->vchi_mutex);
	vchi_service_use(instance->vchi_handle[0]);

	msg_len = sizeof(*msg_hdr) + sizeof(*cfg);
	memset(instance->msg_buf, 0, msg_len);

	msg_hdr = (VC_FB_MSG_HDR_T *) instance->msg_buf;
	msg_hdr->type = VC_FB_MSG_TYPE_CFG;

	/* Copy the user buffer into the message buffer */
	memcpy(msg_hdr->body, cfg, sizeof(*cfg));

	/* Send the message to the videocore */
	success = vchi_msg_queue(instance->vchi_handle[0],
				 instance->msg_buf, msg_len,
				 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (success != 0) {
		LOG_ERR("%s: failed to queue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	}
	/* We are expecting a reply from the videocore */
	down(&instance->msg_avail);

	success = vchi_msg_dequeue(instance->vchi_handle[0],
				   cfg_result, sizeof(*cfg_result),
				   &msg_len, VCHI_FLAGS_NONE);
	if (success != 0) {
		LOG_ERR("%s: failed to dequeue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	} else if (msg_len != sizeof(*cfg_result)) {
		LOG_ERR("%s: incorrect message length %u (expected=%u)",
			__func__, msg_len, sizeof(*cfg_result));

		ret = -1;
		goto unlock;
	}

	ret = cfg_result->success ? -1 : 0;

unlock:
	vchi_service_release(instance->vchi_handle[0]);
	mutex_unlock(&instance->vchi_mutex);

out:
	return ret;
}

int32_t vc_vchi_fb_read(VC_VCHI_FB_HANDLE_T handle, uint32_t res_handle,
			void *buf, int32_t size)
{
	int32_t ret, success = 0;
	FB_INSTANCE_T *instance = handle;
	uint32_t msg_len;
	VC_FB_MSG_HDR_T *msg_hdr;
	VC_FB_READ_T *fb_read;
	VC_FB_RESULT_T result;

	if (handle == NULL) {
		LOG_ERR("%s: invalid handle", __func__);

		ret = -1;
		goto out;
	}

	if (buf == NULL) {
		LOG_ERR("%s: invalid buffer pointer", __func__);

		ret = -1;
		goto out;
	}

	if (size <= 0) {
		LOG_ERR("%s: invalid buffer size %d", __func__, size);

		ret = -1;
		goto out;
	}

	mutex_lock(&instance->vchi_mutex);
	vchi_service_use(instance->vchi_handle[0]);

	LOG_INFO("%s: enter", __func__);

	msg_len = sizeof(*msg_hdr) + sizeof(*fb_read);
	memset(instance->msg_buf, 0, msg_len);

	msg_hdr = (VC_FB_MSG_HDR_T *) instance->msg_buf;
	msg_hdr->type = VC_FB_MSG_TYPE_READ;

	fb_read = (VC_FB_READ_T *) msg_hdr->body;
	fb_read->res_handle = res_handle;
	fb_read->size = size;

	/* Send the message to the videocore */
	success = vchi_msg_queue(instance->vchi_handle[0],
				 instance->msg_buf, msg_len,
				 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
	if (success != 0) {
		LOG_ERR("%s: failed to queue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	}
	LOG_INFO("%s: done sending msg", __func__);
	/* We are expecting a reply from the videocore */
	down(&instance->msg_avail);

	success = vchi_msg_dequeue(instance->vchi_handle[0],
				   &result, sizeof(result),
				   &msg_len, VCHI_FLAGS_NONE);

	LOG_INFO("%s: got reply message %x", __func__, result.success);

	if (success != 0) {
		LOG_ERR("%s: failed to dequeue message (success=%d)",
			__func__, success);

		ret = -1;
		goto unlock;
	} else if (msg_len != sizeof(result)) {
		LOG_ERR("%s: incorrect message length %u (expected=%u)",
			__func__, msg_len, sizeof(result));

		ret = -1;
		goto unlock;
	}

	LOG_INFO("%s: all good do bulk_receive %x", __func__, result.success);
	success = vchi_bulk_queue_receive(instance->vchi_handle[0],
					  buf,
					  size,
					  VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE,
					  0);

	if (success != 0)
		LOG_ERR("%s: vchi_bulk_queue_receive failed", __func__);

unlock:

	vchi_service_release(instance->vchi_handle[0]);
	mutex_unlock(&instance->vchi_mutex);

	ret = success;
	LOG_INFO("%s: exit", __func__);
out:
	return ret;
}
