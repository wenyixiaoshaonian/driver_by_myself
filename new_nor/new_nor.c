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

static const char *rom_probe_types[] = { "cfi_probe", "jedec_probe", "map_rom", NULL };

static struct map_info *new_map_info;
static struct mtd_info *new_mtd_info;

static struct physmap_flash_data new_nor_flash_data = {
	.width		= 2,
};
/*
static struct mtd_partition parts[] = {
	[0] = {
        .name   = "bootloader",
        .size   = 0x00040000,
		.offset	= 0,
	},
	[1] = {
        .name   = "params",
        .offset = MTDPART_OFS_APPEND,
        .size   = 0x00020000,
	},
	[2] = {
        .name   = "kernel",
        .offset = MTDPART_OFS_APPEND,
        .size   = 0x00200000,
	},
	[3] = {
        .name   = "root",
        .offset = MTDPART_OFS_APPEND,
        .size   = MTDPART_SIZ_FULL,
	}
};

*/
static struct mtd_partition nor_parts[] = {
	[0] = {
        .name   = "bootloader_nor",
        .size   = 0x00040000,
		.offset	= 0,
	},
	[1] = {
        .name   = "root_nor",
        .offset = MTDPART_OFS_APPEND,
        .size   = MTDPART_SIZ_FULL,
	}
};

static void led_release (struct device * dev)
{
}


static int new_nor_flash_remove(struct platform_device *dev)
{

  kfree(new_map_info);
  iounmap(new_map_info->virt);
  del_mtd_partitions(new_mtd_info);
//  del_mtd_device(new_mtd_info);
  return 0;
}

static int new_nor_flash_probe(struct platform_device *dev)
{
	
	const char **probe_type;

	
	new_map_info = kzalloc(sizeof(struct map_info), GFP_KERNEL);
	
	new_map_info->name = dev->dev.bus_id;
	new_map_info->phys = dev->resource->start;
	new_map_info->size = dev->resource->end - dev->resource->start + 1;
	new_map_info->bankwidth = new_nor_flash_data.width;
	new_map_info->set_vpp   = new_nor_flash_data.set_vpp;
	printk("%d,%d,%d,",new_map_info->phys,new_map_info->size,new_map_info->bankwidth);
	

	new_map_info->virt = ioremap(new_map_info->phys, new_map_info->size);
	

	simple_map_init(new_map_info);
	

 	probe_type = rom_probe_types;
	new_mtd_info = do_map_probe(*probe_type, new_map_info);
//	new_mtd_info->owner = THIS_MODULE;
//  new_mtd_info->priv = new_map_info;

 	add_mtd_partitions(new_mtd_info, nor_parts, 2);   // 创建两个设备
//  add_mtd_device(new_mtd_info);              //不创建设备
	return 0;
}

// 硬件信息
static struct platform_driver new_nor_flash_driver = {
	.probe		= new_nor_flash_probe,
	.remove		= new_nor_flash_remove,
	.driver		= {
		.name	= "new_nor_flash",
	},
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

static int  new_nor_init(void)
{
  platform_driver_register(&new_nor_flash_driver);

  platform_device_register(&new_nor_flash);

  return 0;
}

static void __exit new_nor_exit(void)
{
  platform_device_unregister(&new_nor_flash);

  platform_driver_unregister(&new_nor_flash_driver);
}

module_init(new_nor_init);
module_exit(new_nor_exit);

MODULE_LICENSE("GPL");


