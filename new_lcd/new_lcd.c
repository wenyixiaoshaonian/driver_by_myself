#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#include <asm/mach/map.h>
#include <asm/arch/regs-lcd.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/fb.h>

static int new_lcd_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info);

struct controller	
{
	    unsigned long	lcdcon1;
		unsigned long	lcdcon2;
		unsigned long	lcdcon3;
		unsigned long	lcdcon4;
		unsigned long	lcdcon5;
		unsigned long	lcdsaddr1;
		unsigned long	lcdsaddr2;
		unsigned long	lcdsaddr3;
		unsigned long	redlut;
		unsigned long	greenlut;
		unsigned long	bluelut;
		unsigned long	reserved[9];
		unsigned long	dithmode;
		unsigned long	tpal;
		unsigned long	lcdintpnd;
		unsigned long	lcdsrcpnd;
		unsigned long	lcdintmsk;
		unsigned long	lpcsel;

};


static struct fb_ops fbops = {
	.owner		    = THIS_MODULE,
	.fb_setcolreg	= new_lcd_setcolreg,   //调色板函数
	.fb_fillrect	= cfb_fillrect,        //填充矩形
	.fb_copyarea	= cfb_copyarea,        //复制区域
	.fb_imageblit	= cfb_imageblit,       //图像
  };



static struct fb_info *new_lcd;

static volatile unsigned long *gpbcon;
static volatile unsigned long *gpbdat;

static volatile unsigned long *gpccon;
static volatile unsigned long *gpdcon;
static volatile unsigned long *gpgcon;

static u32  pseudo_palette[16];

static volatile struct controller* controller_reg;

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int new_lcd_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
   unsigned int val;

	if(regno > 16)
		  return 1;

	/*  用red green blue 三原色构造出val        */
	val  = chan_to_field(red,   &info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,  &info->var.blue);

	pseudo_palette[regno] = val;
	return 0;
}



static int new_lcd_init (void)
{
  new_lcd = framebuffer_alloc(0, NULL);             //分配 info
  /*  ----------------------- info结构体的配置-------------------------------------------*/
//fix 参数
  strcpy(new_lcd->fix.id, "mylcd");               //名字        
  
  //new_lcd->fix.smem_start =                       //物理显存地址 先不设置
  new_lcd->fix.smem_len   = 480*272*16/8;           //缓冲区长度 resolution* 16(RGB)
  new_lcd->fix.type       = FB_TYPE_PACKED_PIXELS;  //see FB_TYPE_  宏
  new_lcd->fix.visual     = FB_VISUAL_TRUECOLOR;    // TFT为真彩色 Ture color
  new_lcd->fix.line_length= 480*16/8;               //每行长度 字节为单位

//var 参数
  new_lcd->var.xres               = 480;            //x方向分辨率
  new_lcd->var.yres               = 272;            //y方向分辨率
  new_lcd->var.xres_virtual       = 480;            //虚拟x方向分辨率
  new_lcd->var.yres_virtual       = 272;            //虚拟y方向分辨率
  new_lcd->var.xoffset            = 0;              //虚拟和实际x方向偏移值
  new_lcd->var.yoffset            = 0;              //虚拟和实际y方向偏移值
  new_lcd->var.bits_per_pixel     = 16;             //每个像素用多少位
  // RGB = 565
  new_lcd->var.red.offset         = 11;
  new_lcd->var.red.length         = 5;
  new_lcd->var.green.offset       = 5;
  new_lcd->var.green.length       = 6;
  new_lcd->var.blue.offset        = 0;
  new_lcd->var.blue.length        = 5;
  new_lcd->var.activate           = FB_ACTIVATE_NOW;
  
  new_lcd->fbops                  = &fbops;               //操作函数
  new_lcd->pseudo_palette         = pseudo_palette;       //假的调色板
 // new_lcd->screen_base          =                       //显存虚拟地址
  new_lcd->screen_size            = 480*272*16/8;         //显存大小
  /*  ----------------------- 硬件相关的设置---------------------------------------------*/
 //1.GPIO
 gpbcon = ioremap(0x56000010,8);
 gpbdat = gpbcon + 1;
 gpccon = ioremap(0x56000020,4);
 gpdcon = ioremap(0x56000030,4);
 gpgcon = ioremap(0x56000060,4);

 *gpccon = 0xaaaaaaaa;                               // GPIO管脚用于VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND 原理图
 *gpdcon = 0xaaaaaaaa;                               // GPIO管脚用于VD[23:8]
 
 *gpbcon &= ~(3<<0);                                 // 清零
 *gpbcon |=  (1<<0);                                 // LCD背光使能  
 *gpbdat &= ~(1<<0);                                 // 关背光,先关闭，使用时开启
 
 *gpgcon |=  (3<<8);                                 // LCD电源使能

 //2.LCD控制器
 controller_reg = ioremap(0X4D000000,sizeof(struct controller));

 controller_reg->lcdcon1 = (4<<8) | (3<<5) | (0x0c<<1) | (0<<0);                             
 //  [17:8]: VCLK = HCLK / [(CLKVAL+1) x 2] ; HCLK = 100MHZ; VCLK 手册上的cycle time 100ns = 10MHZ =>CLKVAL = 4
 //  [6:5]:  11 = TFT LCD panel;
 //  [4:1]:  1100 = 16 bpp for TFT
 //  [0] :   LCD 控制器 0:disable; 1:able ,先关闭，使用时开启

 controller_reg->lcdcon2 =(1<<24) | (271<<14) | (1<<6) | (9<<0);
 //垂直方向参数:VSPW = 9;VBPD = 1 ; LINEVAL =271; VFPD = 1 ;
 
 controller_reg->lcdcon3 =(1<<19) | (479<<8) | (1<<0);
 //水平方向参数:HBPD = 1;HOZVAL = 479; HFPD = 1;
 
 controller_reg->lcdcon4 =(40<<0);
 //水平方向参数:HSPW = 40;
 
 controller_reg->lcdcon5 =(1<<11) | (0<<10) | (1<<9) | (1<<8) | (1<<0);
 //极性，反转: FRM565 :1; INVVCLK = 0; INVVLINE = 1 ;INVVFRAME = 1; HWSWP = 1;  //一些信号需要反转，将地位数据设置在低位，将bit[3]的电源使能先关闭，使用时开启

 //3.显存
 new_lcd->screen_base = dma_alloc_writecombine(NULL,  new_lcd->fix.smem_len,&new_lcd->fix.smem_start,GFP_KERNEL); 
 controller_reg->lcdsaddr1 = (new_lcd->fix.smem_start >>1) & ~ (3<<30);   //存放显存的起始地址除最高两位和最低一位
 controller_reg->lcdsaddr2 = ((new_lcd->fix.smem_start + new_lcd->fix.smem_len )>>1) & 0x1fffff;  //& ~ (0xfff<<21);  显存结束地址
 controller_reg->lcdsaddr3 = (480*16/16);    //一行的长度（单位：半字节）

 /*  --------------------------------------- 完成---------------------------------------------*/

 //4.启动
 *gpbdat |= (1<<0);  // 开背光
 controller_reg->lcdcon1 |= (1<<0);  //开LCD 控制器
 controller_reg->lcdcon5 |= (1<<3);  //开LCD电源

 //注册
  register_framebuffer(new_lcd);   //注册 info

  return 0;
}

static void new_lcd_exit(void)
{
  unregister_framebuffer(new_lcd);  //卸载注册的 info
  iounmap(gpbcon);
  iounmap(gpccon);
  iounmap(gpdcon);
  iounmap(gpgcon);
  iounmap(controller_reg);
  dma_free_writecombine(NULL,new_lcd->fix.smem_len,new_lcd->screen_base,new_lcd->fix.smem_start);
  *gpbdat &= ~(1<<0);              // 开背光
  controller_reg->lcdcon1 &= ~(1<<0);  //开LCD 控制器
//  controller_reg->lcdcon5 &= ~(1<<3);  //开LCD电源
  framebuffer_release(new_lcd);
}



module_init(new_lcd_init);
module_exit(new_lcd_exit);

MODULE_LICENSE("GPL");


