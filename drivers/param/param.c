



#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <asm/unistd.h>
#include <asm/errno.h>
#include <asm/uaccess.h>

#include <param.h>










struct proc_dir_entry*	gp_param_proc_dir;


static char			sa_Param_dev_path[PARAM_DEV_PATH_LEN]	= { 0x0, };
static struct file*		sp_Param_device;
static mm_segment_t	ss_Old_FS;









static int param_get_dev_path(void)
{
#define DEFAULT_GPT_ENTRIES			128
#define MMCBLK_PART_INFO_PATH_LEN	128
#define PARTITION_NAME_LEN			128

	struct file*		p_file;
	mm_segment_t	s_Old_FS;
	char				a_part_info_path[MMCBLK_PART_INFO_PATH_LEN]	= { 0, };
	char				a_partition_name[PARTITION_NAME_LEN]			= { 0, };
	int				v_index;


	memset( sa_Param_dev_path, 0x0, PARAM_DEV_PATH_LEN );


	for( v_index = 0; v_index < DEFAULT_GPT_ENTRIES; v_index++ )
	{
		memset( a_part_info_path, 0x0, MMCBLK_PART_INFO_PATH_LEN );
		snprintf( a_part_info_path, MMCBLK_PART_INFO_PATH_LEN, "/sys/block/mmcblk0/mmcblk0p%d/partition_name", v_index + 1 );


		p_file	= filp_open( a_part_info_path, O_RDONLY, NULL );
		if( IS_ERR(p_file) )
		{
			PARAM_LOG( KERN_ERR "[%s] %s file open was failed!: %ld\n", __FUNCTION__, a_part_info_path, PTR_ERR(p_file) );
		}
		else
		{
			s_Old_FS	= get_fs();
			set_fs( get_ds() );


			memset( a_partition_name, 0x0, PARTITION_NAME_LEN );
			p_file->f_op->read( p_file, a_partition_name, PARTITION_NAME_LEN, &p_file->f_pos );


			set_fs( s_Old_FS );
			filp_close( p_file, NULL );


			/***
				Use the "strncmp" function to avoid following garbage character
			***/
			if( !strncmp( PARAM_PART_NAME, a_partition_name, strlen(PARAM_PART_NAME) ) )
			{
				snprintf( sa_Param_dev_path, PARAM_DEV_PATH_LEN, "/dev/block/mmcblk0p%d", v_index + 1 );
				PARAM_LOG( KERN_INFO "SEC_PARAM : %s device was found\n", sa_Param_dev_path );


				break;
			}
		}
	}


	if( sa_Param_dev_path[0] != 0x0 )
	{
		return	NULL;
	}
	else
	{
		return	-EFAULT;
	}
}





static int param_device_open(void)
{
	if( sa_Param_dev_path[0] == 0x0 )
	{
		if( !param_get_dev_path() )
		{
			PARAM_LOG( KERN_INFO "[%s] : %s partition was found for sec_param device.\n", __FUNCTION__, sa_Param_dev_path );
		}
		else
		{
			PARAM_LOG( KERN_ERR "[%s] : Can't find sec_param device!!!\n", __FUNCTION__ );
			return	-EFAULT;
		}
	}


	sp_Param_device	= filp_open( sa_Param_dev_path, O_RDWR|O_SYNC, NULL );
	if( IS_ERR(sp_Param_device) )
	{
		PARAM_LOG( KERN_ERR "[%s] param device(%s) open was failed!: %ld\n", __FUNCTION__, sa_Param_dev_path, PTR_ERR(sp_Param_device) );


		return	-EFAULT;
	}
	else
	{
		sp_Param_device->f_flags	|= O_NONBLOCK;
		ss_Old_FS	= get_fs();
		set_fs( get_ds() );


		return	NULL;
	}
}




static void param_device_close(void)
{
	set_fs( ss_Old_FS );
	sp_Param_device->f_flags	&= ~O_NONBLOCK;
	filp_close( sp_Param_device, NULL );

	return;
}





static int param_device_read(off_t v_offset, unsigned char* p_buffer, int v_size)
{
	int				v_ret;


	if( IS_ERR(sp_Param_device) )
	{
		PARAM_LOG( KERN_ERR "[%s] param device handler was fault!: %ld\n", __FUNCTION__, PTR_ERR(sp_Param_device) );


		return	-EFAULT;
	}
	else
	{
		v_ret	= sp_Param_device->f_op->llseek( sp_Param_device, PARAM_OFFSET + v_offset, SEEK_SET );
		if( v_ret < 0 )
		{
			v_ret	= -EPERM;
		}
		else
		{
			v_ret	= sp_Param_device->f_op->read( sp_Param_device, p_buffer, v_size, &sp_Param_device->f_pos );
		}


		return	v_ret;
	}
}






static int param_device_write(off_t v_offset, unsigned char* p_data, int v_size)
{
	int				v_ret;


	if( IS_ERR(sp_Param_device) )
	{
		PARAM_LOG( KERN_ERR "[%s] param device handler was fault!: %ld\n", __FUNCTION__, PTR_ERR(sp_Param_device) );


		return	-EFAULT;
	}
	else
	{
		v_ret	= sp_Param_device->f_op->llseek( sp_Param_device, PARAM_OFFSET + v_offset, SEEK_SET );
		if( v_ret < 0 )
		{
			v_ret	= -EPERM;
		}
		else
		{
			v_ret	= sp_Param_device->f_op->write( sp_Param_device, p_data, v_size, &sp_Param_device->f_pos );
		}


		return	v_ret;
	}
}






static int param_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
//		PARAM_LOG( KERN_INFO "[%s] SEC_PARAM booting_now : %d\n", __FUNCTION__, s_efs.booting_now );
//		PARAM_LOG( KERN_INFO "[%s] SEC_PARAM fota_mode   : %d\n", __FUNCTION__, s_efs.fota_mode );
		PARAM_LOG( KERN_INFO "[%s] SEC_PARAM efs_info	  : %s\n", __FUNCTION__, s_efs.efs_info );
//		PARAM_LOG( KERN_INFO "[%s] SEC_PARAM boot_status_prev  : %d\n", __FUNCTION__, s_efs.boot_status_prev );
//		PARAM_LOG( KERN_INFO "[%s] SEC_PARAM boot_status  : %d\n", __FUNCTION__, s_efs.boot_status );
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.efs_info );
	}
}



static int param_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.efs_info) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.efs_info, 0x0, sizeof(s_efs.efs_info) );
	memcpy( s_efs.efs_info, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}












static int param_keystr_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.keystr );
	}
}




 
static int param_keystr_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.keystr) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.keystr, 0x0, sizeof(s_efs.keystr) );
	memcpy( s_efs.keystr, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}













static int param_rilprop_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.ril_prop );
	}
}



 
static int param_rilprop_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.ril_prop) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.ril_prop, 0x0, sizeof(s_efs.ril_prop) );
	memcpy( s_efs.ril_prop, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}















static int param_fsbuild_check_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.fsbuild_check );
	}
}




 
static int param_fsbuild_check_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.fsbuild_check) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );	
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.fsbuild_check, 0x0, sizeof(s_efs.fsbuild_check) );
	memcpy( s_efs.fsbuild_check, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}









static int param_model_name_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.model_name );
	}
}



 
static int param_model_name_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.model_name) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.model_name, 0x0, sizeof(s_efs.model_name) );
	memcpy( s_efs.model_name, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}











static int param_sw_version_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.sw_version );
	}
}



 
static int param_sw_version_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.sw_version) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.sw_version, 0x0, sizeof(s_efs.sw_version) );
	memcpy( s_efs.sw_version, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}








static int param_MD5_checksum_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.MD5_checksum );
	}
}



 
static int param_MD5_checksum_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.MD5_checksum) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );	
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.MD5_checksum, 0x0, sizeof(s_efs.MD5_checksum) );
	memcpy( s_efs.MD5_checksum, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}








static int param_recovery_opts_read_proc_debug(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int				v_ret;
	SEC_PARAM		s_efs	= { 0, };


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	*eof	= 1;
	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( offset, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}
	else
	{
		param_device_close();


		return	sprintf( page, "%s\n", s_efs.recovery_opts );
	}
}



 
static int param_recovery_opts_write_proc_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char*		p_buffer;
	int			v_ret;
	SEC_PARAM	s_efs;


	if( count < 1 )
		return	-EINVAL;


	if( count > sizeof(s_efs.recovery_opts) )
		return	-EFAULT;


	p_buffer	= kmalloc( count, GFP_KERNEL );
	if( !p_buffer )
		return	-ENOMEM;


	if( copy_from_user(p_buffer, buffer, count) )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	v_ret	= param_device_open();
	if( v_ret )
	{
		kfree( p_buffer );
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	memset( s_efs.recovery_opts, 0x0, sizeof(s_efs.recovery_opts) );
	memcpy( s_efs.recovery_opts, p_buffer, (int)count );


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();
		kfree( p_buffer );


		return	-EFAULT;
	}


	param_device_close();
	kfree( p_buffer );


	return	count;
}








extern int (*set_recovery_mode)(void);

int _set_recovery_mode(void)
{
	int			v_ret;
	SEC_PARAM	s_efs;


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}


//	s_efs.booting_now	= RECOVERY_ENTER_MODE;


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}


	param_device_close();


	return	0;
}











extern int (*set_recovery_mode_done)(void);

int _set_recovery_mode_done(void)
{
	int			v_ret;
	SEC_PARAM	s_efs;


	PARAM_LOG( KERN_INFO "_set_recovery_mode_done++" );


	v_ret	= param_device_open();
	if( v_ret )
	{
		return	-EFAULT;
	}


	memset( &s_efs, 0xff, sizeof(SEC_PARAM) );


	v_ret	= param_device_read( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param read was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}


//	s_efs.booting_now	= RECOVERY_END_MODE;


	v_ret	= param_device_write( 0, (unsigned char *)&s_efs, sizeof(SEC_PARAM) );
	if( v_ret < 0 )
	{
		PARAM_LOG( KERN_ERR "[%s] : param write was failed!: %d\n", __FUNCTION__, v_ret );
		param_device_close();


		return	-EFAULT;
	}


	param_device_close();


	PARAM_LOG( KERN_INFO "_set_recovery_mode_done--" );


	return	0;
}













static int __init param_init(void)
{
	struct proc_dir_entry *ent;


	gp_param_proc_dir	= proc_mkdir( "LinuStoreIII", NULL );
	if( !gp_param_proc_dir )
	{
		PARAM_LOG( KERN_ERR "[%s] : param proc dir create was failed!\n", __FUNCTION__ );
		return	-EFAULT;
	}


	ent = create_proc_entry( "efs_info", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_read_proc_debug;
	ent->write_proc = param_write_proc_debug;


	ent = create_proc_entry( "keystr", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_keystr_read_proc_debug;
	ent->write_proc = param_keystr_write_proc_debug;


	ent = create_proc_entry( "ril_prop", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_rilprop_read_proc_debug;
	ent->write_proc = param_rilprop_write_proc_debug;


	ent = create_proc_entry( "fsbuild_check", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_fsbuild_check_read_proc_debug;
	ent->write_proc = param_fsbuild_check_write_proc_debug;


	ent = create_proc_entry( "model_name", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_model_name_read_proc_debug;
	ent->write_proc = param_model_name_write_proc_debug;


	ent = create_proc_entry( "sw_version", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_sw_version_read_proc_debug;
	ent->write_proc = param_sw_version_write_proc_debug;


	ent = create_proc_entry( "MD5_checksum", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_MD5_checksum_read_proc_debug;
	ent->write_proc = param_MD5_checksum_write_proc_debug;


	ent = create_proc_entry( "recovery_opts", S_IFREG | S_IWUSR | S_IRUGO, gp_param_proc_dir );
	ent->read_proc = param_recovery_opts_read_proc_debug;
	ent->write_proc = param_recovery_opts_write_proc_debug;


#if 0 /* build error temporary comment jinuk.jeon 20110713 samsung*/
	set_recovery_mode = _set_recovery_mode;
	set_recovery_mode_done = _set_recovery_mode_done;
#endif


	return 0;
}





static void __exit param_exit(void)
{
	remove_proc_entry( "efs_info", gp_param_proc_dir );
	remove_proc_entry( "keystr", gp_param_proc_dir );
	remove_proc_entry( "ril_prop", gp_param_proc_dir );
	remove_proc_entry( "fsbuild_check", gp_param_proc_dir );
	remove_proc_entry( "model_name", gp_param_proc_dir );
	remove_proc_entry( "sw_version", gp_param_proc_dir );
	remove_proc_entry( "MD5_checksum", gp_param_proc_dir );
	remove_proc_entry( "recovery_opts", gp_param_proc_dir );
	remove_proc_entry( "LinuStoreIII", NULL );
}










module_init(param_init);
module_exit(param_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Samsung Param Operation");
