#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/sizes.h>

#include <asm/hardware.h>
//#include <asm/arch/board.h>
#include <asm/arch/gpio.h>



struct nand_reg 
{
unsigned long NFCONF ;
unsigned long NFCONT ;
unsigned long NFCMD  ;
unsigned long NFADDR ;
unsigned long NFDATA ;

unsigned long NFECCD0;
unsigned long NFECCD1;

unsigned long NFECCD ;
unsigned long NFSTAT ;
unsigned long NFESTAT0; 
unsigned long NFESTAT1; 
unsigned long NFMECC0 ;
unsigned long NFMECC1 ;
unsigned long NFSECC  ;
unsigned long NFSBLK;
unsigned long NFEBLK;
};

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


static struct nand_chip *new_chip;
static struct nand_reg *nand_regs;
static struct mtd_info *new_info; 

struct clk *clk_get(struct device *dev, const char *id);

int clk_enable(struct clk *clk);

void new_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
	if (ctrl & NAND_CLE)
      nand_regs->NFCMD = dat;
	else
      nand_regs->NFADDR = dat;
}

static void new_select_chip(struct mtd_info *mtd, int chipnr)
{
  if(chipnr == -1)
    nand_regs->NFCONT |=  (1<<1);
  else
  	nand_regs->NFCONT &= ~(1<<1);
  
}

static int new_dev_ready(struct mtd_info *mtd, struct nand_chip *chip)
{
  return (nand_regs->NFSTAT & (1<<0));
}

static int new_nand_init(void)
{
  struct clk *clk;
  new_chip = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);
  
  nand_regs = ioremap(0x4E000000,sizeof(struct nand_reg));
  
  //根据nand_scan中使用到的函数来编写相应的底层函数,如果默认函数不能用的话 要自己重新写
  
  new_chip->select_chip = new_select_chip;
  new_chip->cmd_ctrl    = new_cmd_ctrl;              //判断时命令还是地址
  new_chip->IO_ADDR_R   = &nand_regs->NFDATA;           // 读函数中需要用到
  new_chip->IO_ADDR_W	= &nand_regs->NFDATA;		   // 写函数中需要用到
  new_chip->dev_ready   = new_dev_ready;             // 读状态
  new_chip->ecc.mode    = NAND_ECC_SOFT;               // enable ECC
  
  //硬件操作
   //使能时钟
   clk = clk_get(NULL, "nand");
   clk_enable(clk);
   
   // TWRPH0 Duration >= 12ns;  TACLS Duration >= 0;  TWRPH1 Duration >= 5ns;
   // TWRPH0 >=1  ;  TACLS >= 0;  TWRPH1 >= 0
  nand_regs->NFCONF = (0 << 12) | (1<<8) | (0<<4);

  nand_regs->NFCONT = (1<<1) | (1<<0);  //使能控制器，先取消片选

  //info的创建 设置，使用scan
  new_info = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
  new_info->owner = THIS_MODULE;
  new_info->priv = new_chip;
  
  nand_scan(new_info, 1);

  //添加分区
  add_mtd_partitions(new_info, parts, 4);
  return 0;
}

static void new_init_exit(void)
{
  kfree(new_chip);
  iounmap(nand_regs);
  kfree(new_info);
  del_mtd_partitions(parts);
}

module_init(new_nand_init);
module_exit(new_init_exit);

MODULE_LICENSE("GPL");


