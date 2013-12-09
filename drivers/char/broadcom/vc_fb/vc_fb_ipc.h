/*****************************************************************************
* Copyright 2001 - 2012 Broadcom Corporation.  All rights reserved.
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

#define DISPLAY_IPC_CTRL_SIZE  256

enum DISPLAY_IPC_COMMAND_T {
	DISPLAY_IPC_UNUSED,
	DISPLAY_IPC_POWER,
	DISPLAY_IPC_BRIGHTNESS,
	DISPLAY_IPC_FB_INIT,
	DISPLAY_IPC_FB_UPDATE,
	DISPLAY_IPC_CTRL_INIT,
	DISPLAY_IPC_CTRL_WRITE,
	DISPLAY_IPC_CTRL_READ,
	DISPLAY_IPC_FB_TERM,
	DISPLAY_IPC_SNAP_TAKE,
	DISPLAY_IPC_SNAP_TERM,
};

enum DISPLAY_IPC_FB_FORMAT {
	DISPLAY_IPC_FB_FORMAT_RGBA32 = 0,
	DISPLAY_IPC_FB_FORMAT_RGB565,
	DISPLAY_IPC_FB_FORMAT_XRGB8888,
	DISPLAY_IPC_FB_FORMAT_RGBX8888,
	DISPLAY_IPC_FB_FORMAT_BGRX8888,
	DISPLAY_IPC_FB_FORMAT_RGBX32,

	DISPLAY_IPC_FB_FORMAT_MAX,
};

enum DISPLAY_IPC_FB_MODE {
	DISPLAY_IPC_FB_MODE_SINGLE_BUFFER = 0,
	DISPLAY_IPC_FB_MODE_SINGLE_BUFFER_AUTO,
	DISPLAY_IPC_FB_MODE_DOUBLE_BUFFER,

	DISPLAY_IPC_FB_MODE_MAX,
};

enum DISPLAY_IPC_POWER_T {
	DISPLAY_IPC_POWER_OFF = 0,
	DISPLAY_IPC_POWER_BLANK = 1,
	DISPLAY_IPC_POWER_PARTIAL = 2,
	DISPLAY_IPC_POWER_RESERVED = 3,
	DISPLAY_IPC_POWER_FULL = 4
};


struct DISPLAY_IPC_T {
	uint32_t command;
	uint32_t display_number;

	union {
		struct {
			uint32_t new_state;
		} power;
		struct {
			uint32_t level;
		} brightness;
		struct {
			uint32_t format;
			uint32_t mode;
			uint32_t keep_buffers;
			uint32_t no_padding;
		} fb_init;
		struct {
			uint32_t use_buffer;
		} fb_update;
		struct {
			uint32_t target;
			uint32_t count;
		} ctrl;
	} request;

	union {
		struct {
			uint32_t width;
			uint32_t height;
			uint32_t pitch;
			void *image_data[2];
		} fb_init;
		struct {
			uint32_t buffer_size;
			void *data_buffer;
		} ctrl_init;
		struct {
			uint32_t buffer_size;
			uint32_t width;
			uint32_t height;
			uint32_t pitch;
			void *data_buffer;
		} take_snap;
	} response;

	uint32_t result;

	uint32_t req;
	uint32_t ack;
};

