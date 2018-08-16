#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <asm/io.h>

static void led_release (struct device * dev)
{
}

static struct physmap_flash_data new_nor_flash_data = {
	.width		= 2,
};


static struct resource new_nor_flash_resource = {
	.start		= 0,
	.end		= 0x1000000,
	.flags		= IORESOURCE_MEM,
};


static struct platform_device new_nor_flash = {
	.name		= "new_nor_flash",
	.id		= 0,
	.dev		= {
		.platform_data	= &new_nor_flash_data,
			.release    = led_release,
	},
	.num_resources	= 1,
	.resource	= &new_nor_flash_resource,
};



static int  new_nor_init_dev(void)
{
  platform_device_register(&new_nor_flash);
  return 0;
}

static void __exit new_nor_exit_dev(void)
{
  platform_device_unregister(&new_nor_flash);
}

module_init(new_nor_init_dev);
module_exit(new_nor_exit_dev);

MODULE_LICENSE("GPL");

