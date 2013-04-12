/*****************************************************************************
* Copyright 2006 - 2011 Broadcom Corporation.  All rights reserved.
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
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/proc_fs.h>

#ifdef CONFIG_SYSFS
#define PROC_FAKE_BATTERY "fake-battery"
static struct proc_dir_entry *fake_battery_proc_entry;
#endif

static int power_get_property(struct power_supply *psy,
                                   enum power_supply_property psp,
                                   union power_supply_propval *val)
{
   switch (psp)
   {
      case POWER_SUPPLY_PROP_ONLINE:
         /* fake power supply always online */
         val->intval = 1;
         break;

      default:
         return -EINVAL;
   }

   return 0;
}

static enum power_supply_property power_props[] = {
   POWER_SUPPLY_PROP_ONLINE,
};

static int battery_get_property(struct power_supply *psy,
                                     enum power_supply_property psp,
                                     union power_supply_propval *val)
{
   switch (psp)
   {
      case POWER_SUPPLY_PROP_STATUS:
         val->intval = POWER_SUPPLY_STATUS_FULL;
         break;
      case POWER_SUPPLY_PROP_TECHNOLOGY:
         val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
         break;
      case POWER_SUPPLY_PROP_VOLTAGE_NOW:
         val->intval = 4200000;
         break;
      case POWER_SUPPLY_PROP_VOLTAGE_MAX:
         val->intval = 4200000;
         break;
      case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
         val->intval = 4200000;
         break;
      case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
         val->intval = 0;
         break;
      case POWER_SUPPLY_PROP_HEALTH:
         val->intval = POWER_SUPPLY_HEALTH_GOOD;
         break;
      case POWER_SUPPLY_PROP_CAPACITY:
         val->intval = 100;
         break;
      case POWER_SUPPLY_PROP_PRESENT:
         val->intval = 1;
         break;
      default:
         return -EINVAL;
   }

   return 0;
}

static enum power_supply_property battery_props[] = {
   POWER_SUPPLY_PROP_STATUS,
   POWER_SUPPLY_PROP_TECHNOLOGY,
   POWER_SUPPLY_PROP_VOLTAGE_NOW,
   POWER_SUPPLY_PROP_VOLTAGE_MAX,
   POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
   POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
   POWER_SUPPLY_PROP_HEALTH,
   POWER_SUPPLY_PROP_CAPACITY,
   POWER_SUPPLY_PROP_PRESENT,
};

static char *power_supplied_to[] = {
   "battery",
};

static struct power_supply psy_ac = {
   .name = "ac",
   .type = POWER_SUPPLY_TYPE_MAINS,
   .supplied_to = power_supplied_to,
   .num_supplicants = ARRAY_SIZE(power_supplied_to),
   .properties = power_props,
   .num_properties = ARRAY_SIZE(power_props),
   .get_property = power_get_property,
};

static struct power_supply psy_usb = {
   .name = "usb",
   .type = POWER_SUPPLY_TYPE_USB,
   .supplied_to = power_supplied_to,
   .num_supplicants = ARRAY_SIZE(power_supplied_to),
   .properties = power_props,
   .num_properties = ARRAY_SIZE(power_props),
   .get_property = power_get_property,
};

static struct power_supply psy_battery = {
   .name           = "battery",
   .type           = POWER_SUPPLY_TYPE_BATTERY,
   .properties     = battery_props,
   .num_properties = ARRAY_SIZE(battery_props),
   .get_property   = battery_get_property,
};

static int __devinit battery_probe(struct platform_device *dev)
{
   int ret;

   printk("%s:\n",__FUNCTION__);

   ret = power_supply_register(&dev->dev, &psy_ac);
   if (ret)
      goto err_psy_reg_ac;
   ret = power_supply_register(&dev->dev, &psy_usb);
   if (ret)
      goto err_psy_reg_usb;
   ret = power_supply_register(&dev->dev, &psy_battery);
   if (ret)
      goto err_psy_reg_battery;

   printk("%s: return 0\n",__FUNCTION__);
   return 0;

err_psy_reg_battery:
   power_supply_unregister(&psy_usb);
err_psy_reg_usb:
   power_supply_unregister(&psy_ac);
err_psy_reg_ac:

   printk("%s: return %d\n",__FUNCTION__, ret);
   return ret;
}

static int __devexit battery_remove(struct platform_device *dev)
{
   printk("%s:\n",__FUNCTION__);
   power_supply_unregister(&psy_battery);
   power_supply_unregister(&psy_usb);
   power_supply_unregister(&psy_ac);

   return 0;
}

#ifdef CONFIG_PM
static int battery_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int battery_resume(struct platform_device *dev)
{
	return 0;
}
#endif

#ifdef CONFIG_SYSFS
static int fake_battery_proc_read( char *buf, char **start, off_t offset, int count,
      int *eof, void *data )
{
   sprintf(buf, "in-use\n");
   *eof = 1;

   return strlen(buf);
}
#endif

static struct platform_driver battery_driver = {
   .driver.name  = "fake-battery",
   .driver.owner = THIS_MODULE,
   .probe        = battery_probe,
   .remove       = __devexit_p(battery_remove),
#ifdef CONFIG_PM
   .suspend      = battery_suspend,
   .resume       = battery_resume,
#endif
};

static struct platform_device *battery_platform_device = NULL;

static int __init battery_init(void)
{
   int ret;

   printk("%s:\n",__FUNCTION__);

   battery_platform_device = platform_device_alloc("fake-battery", -1);
   if (!battery_platform_device)
   {
      printk("%s: platform_device_alloc failed\n",__FUNCTION__);
      return -ENOMEM;
   }

   ret = platform_driver_register(&battery_driver);
   if (ret)
   {
      printk("%s: platform_driver_register failed %d\n",__FUNCTION__, ret);
      goto err_platform_driver_register;
   }

   ret = platform_device_add(battery_platform_device);
   if (ret)
   {
      printk("%s: platform_device_add failed %d\n",__FUNCTION__, ret);
      goto err_platform_device_add;
   }

#ifdef CONFIG_SYSFS
   fake_battery_proc_entry = create_proc_entry( PROC_FAKE_BATTERY, 0660, NULL);
   if ( ! fake_battery_proc_entry )
   {
      ret = -EFAULT;
      printk("%s: create_proc_entry failed", __FUNCTION__);
      goto err_platform_device_add;
   }
   fake_battery_proc_entry->read_proc = fake_battery_proc_read;
   fake_battery_proc_entry->write_proc = NULL;
#endif


   return 0;

err_platform_device_add:
   platform_driver_unregister(&battery_driver);
err_platform_driver_register:
   platform_device_del(battery_platform_device);
   platform_device_put(battery_platform_device);

   return ret;
}

static void __exit battery_exit(void)
{
   if (battery_platform_device)
   {
      platform_device_del(battery_platform_device);
      platform_device_put(battery_platform_device);
   }
   platform_driver_unregister(&battery_driver);
}

module_init(battery_init);
module_exit(battery_exit);
MODULE_AUTHOR( "Broadcom" );
MODULE_DESCRIPTION( "Fake battery driver." );
MODULE_LICENSE( "GPL" );

