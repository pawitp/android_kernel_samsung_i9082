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

#if !defined( CAMERA_SETTINGS_H )
#define  CAMERA_SETTINGS_H

#if defined (CONFIG_CAPRI_28145)

#define  HW_CFG_CAMERA_GPIO \
  {  1, 0x0, 0x0, "CAM1_PWDN"},		\
  {101, 0x0, 0x0, "CAM2_RST"},		\
  {115, 0x0, 0x0, "CAM2_PWDN"}

#else

#define  HW_CFG_CAMERA_GPIO \
  {  8, 0x0, 0x0, "CAM1_PWDN"},		\
  {  9, 0x0, 0x0, "CAM1_RST"},		\
  { 10, 0x0, 0x0, "CAM2_RST"},		\
  { 11, 0x0, 0x1, "CAM2_PWDN"},	   \
  {176, 0x0, 0x0, "CAM2_REG_ON"}

#endif

#endif /* CAMERA_SETTINGS_H */
