#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/dma.h>


#define RAMBLOCK_SIZE  (1024*1024)


static struct gendisk *ram_disk;

static unsigned char *area; 
static request_queue_t *new_queue;

static int major;

static DEFINE_SPINLOCK(lock);

static int new_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
    //容量= heads*sectors*cylinders*512
	geo->heads = 2;     //磁头 有几面
	geo->sectors = 32;  //柱面 有几环
	geo->cylinders = (RAMBLOCK_SIZE/2/32/512);//每环中有几个扇区
	return 0;
}



static struct block_device_operations ram_fops = {
	.owner	= THIS_MODULE,
	.getgeo = new_getgeo,
};


static void new_do_request(request_queue_t *q)
{
	struct request *req;
	while ((req = elv_next_request(q)) != NULL)
		{
		
		//设置传输三要素
	    unsigned long offset = req->sector * 512;
		unsigned long len    = req->current_nr_sectors * 512;

	    if (rq_data_dir(req) == READ)
		{
			memcpy(req->buffer, area+offset, len);
		}
		else
		{
			memcpy(area+offset, req->buffer, len);
		}	
	
		  end_request(req, 1);   //0成功 2失败
		}
}

static int new_mtdram_init(void)
{

  new_queue = blk_init_queue(new_do_request, &lock);
  major     = register_blkdev(0,"ramblock");        //分配一个主设备号 无操作函数

  ram_disk  = alloc_disk(16);
  
  ram_disk->major          = major;
  ram_disk->first_minor    = 0;
  sprintf(ram_disk->disk_name, "ramblock");     //块设备名称
  ram_disk->fops           = &ram_fops;  
  set_capacity(ram_disk, RAMBLOCK_SIZE/512);    //设置容量，扇区 /512
  ram_disk->queue          = new_queue;

  area = kzalloc(RAMBLOCK_SIZE, GFP_KERNEL);

  add_disk(ram_disk);

  return 0;
}

static void new_mtdram_exit(void)
{
  unregister_blkdev(major,"ramblock");
  kfree(area);
  del_gendisk(ram_disk);
  put_disk(ram_disk);
  blk_cleanup_queue(new_queue);
}

module_init(new_mtdram_init);
module_exit(new_mtdram_exit);

MODULE_LICENSE("GPL");


