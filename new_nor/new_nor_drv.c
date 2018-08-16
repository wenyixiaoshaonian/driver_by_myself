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
static struct resource *new_resource;

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



static int new_nor_flash_probe(struct platform_device *pdev)
{
	
/*
	const char **probe_type;
    static struct physmap_flash_data *new_nor_flash_data;
	
	new_map_info = kzalloc(sizeof(struct map_info), GFP_KERNEL);
	new_mtd_info = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
    new_resource = kzalloc(sizeof(struct resource), GFP_KERNEL);
	
	new_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);  //获取资源
	
	new_map_info->name = pdev->dev.bus_id;
	new_map_info->phys = new_resource->start;
	new_map_info->size = new_resource->end - new_resource->start + 1;
	new_map_info->bankwidth = new_nor_flash_data->width;
	new_map_info->set_vpp = new_nor_flash_data->set_vpp;

	new_map_info->virt = ioremap(new_map_info->phys, new_map_info->size);

	simple_map_init(new_map_info);

	probe_type = rom_probe_types;
	new_mtd_info = do_map_probe(*probe_type, new_map_info);
	new_mtd_info->owner = THIS_MODULE;

	add_mtd_partitions(new_mtd_info, parts, 4);

	add_mtd_device(new_mtd_info);
*/
	return 0;
}

static int new_nor_flash_remove(struct platform_device *pdev)
{
  printk("remove!!!!!!");
/*
  del_mtd_partitions(new_mtd_info);
  del_mtd_device(new_mtd_info);
  map_destroy(new_map_info);
  iounmap(new_map_info->virt);
  release_resource(new_resource);
  kfree(new_resource); 
  kfree(new_mtd_info);
  kfree(new_map_info);
*/
  return 0;
}

struct platform_driver new_nor_flash_driver = {
	.probe		= new_nor_flash_probe,
	.remove		= new_nor_flash_remove,
	.driver		= {
		.name	= "new_nor_flash",
	}
};


static int  new_nor_init_drv(void)
{
  platform_driver_register(&new_nor_flash_driver);
  return 0;
}

static void __exit new_nor_exit_drv(void)
{
  platform_driver_unregister(&new_nor_flash_driver);
}

module_init(new_nor_init_drv);
module_exit(new_nor_exit_drv);

MODULE_LICENSE("GPL");


