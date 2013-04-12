#ifndef __PARAM_H__
#define __PARAM_H__




#define PARAM_PART_NAME				"cal"
#define PARAM_OFFSET				0x40000
#define PARAM_DEV_PATH_LEN			128
#define PARAM_UNIT_MAX				32







#define NORMAL_MODE         0
#define FOTA_MODE           1


#define CHARGING_MODE   0
#define DIRECT_BOOTING_MODE 1
#define RECOVERY_ENTER_MODE 2
#define RECOVERY_END_MODE   3
#define DOWNLOAD_FAIL       4










typedef struct __tag_sec_param {

	char efs_info[PARAM_UNIT_MAX];
	char keystr[PARAM_UNIT_MAX];			// 키 스트링 유출 방지.
	char ril_prop[PARAM_UNIT_MAX];
	char fsbuild_check[PARAM_UNIT_MAX];		// for "AT+FSBUILDC=1,0"

	char	model_name[PARAM_UNIT_MAX];
	char	sw_version[PARAM_UNIT_MAX];

	char	MD5_checksum[PARAM_UNIT_MAX];	// for "AT+FSBUILDC=1,2"

	char	recovery_opts[PARAM_UNIT_MAX];	// for recovery mode options

}SEC_PARAM;










#if 1
#define PARAM_LOG	printk
#else
#define PARAM_LOG	printk
#endif





#endif	// __PARAM_H__
