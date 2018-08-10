#include <linux/module.h>
#include <linux/poll.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>
#include <linux/poll.h>


int major;
static struct class        *buttons_class;
static struct class_device *buttons_class_dev;
static struct timer_list    time;
static struct pin_desc     *pin_descs;

static unsigned char   pin_val;

volatile unsigned long *gpfcon = NULL;
volatile unsigned long *gpfdat = NULL;
volatile unsigned long *gpgcon = NULL;
volatile unsigned long *gpgdat = NULL;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
/*
static int buttons_dev_open (struct inode *, struct file *);
	
static int buttons_dev_close (struct inode *, struct file *);

void buttons_all_exit(void);

int buttons_all_init(void);
*/


static volatile int ev_press = 0;

int cnt = 0;


void free_irq(unsigned int, void *);
struct class *class_create(struct module *, const char *);
struct class_device *class_device_create(struct class *,struct class_device *,unsigned long ,struct device *,const char *);
void class_device_unregister(struct class_device *);
void class_destroy(struct class *);

struct pin_desc
{
 unsigned int pin;
 unsigned int key_val;
};

static struct pin_desc pins_desc[4]=
{
  {S3C2410_GPF0,0x01},
  {S3C2410_GPF2,0x02},
  {S3C2410_GPG3,0x03},
  {S3C2410_GPG11,0x04},
};

static struct fasync_struct *sign;


static	 unsigned buttons_dev_poll(struct file *file, poll_table *wait)
{
				unsigned int mask = 0;
				
				poll_wait(file,&button_waitq,wait);
	
				if(ev_press)
					mask |= POLLIN|POLLRDNORM;
				 
				return mask;
}  

static int buttons_all_irq(int irq,void *dev_id)
{
  pin_descs = (struct pin_desc *)dev_id;
  printk("cnt = %d",cnt++);
  printk("\n");
  mod_timer(&time, jiffies + HZ/100);  // HZ = 1s HZ/100 = 10ms

  return IRQ_RETVAL(IRQ_HANDLED);
}

static void timer_function(unsigned long data)
{
  unsigned int pinval;
  struct pin_desc * pindesc = pin_descs;
  pinval = s3c2410_gpio_getpin(pindesc->pin);
  if(pinval)
  	pin_val = 0x80 | pindesc->key_val;
  else
  	pin_val = pindesc->key_val;
  
//  printk("pin_val:%d",&pin_val);
  ev_press = 1;
  wake_up_interruptible(&button_waitq);
  kill_fasync(&sign, SIGIO, POLL_OUT);
}


static int buttons_drv_open (struct inode *inode, struct file *file)
{
  *gpfcon &= ~((0x3 << (4*2)) | (0x3 << (5*2)) | (0x3 << (6*2)));
  *gpfcon |=  ((0x1 << (4*2))  | (0x1 << (5*2))  | (0x1 << (6*2)));
/*  
  *gpfcon &= ~((3<<0) & (3<<4));
  *gpgcon &= ~((2<<6) & (2<<22));
  *gpfcon |= (3<<0) & (3<<4);
  *gpgcon |= (2<<6) & (2<<22);
*/
 request_irq(IRQ_EINT0, buttons_all_irq,IRQT_BOTHEDGE,"s2",&pins_desc[0]);
 request_irq(IRQ_EINT2, buttons_all_irq,IRQT_BOTHEDGE,"s3",&pins_desc[1]);
 request_irq(IRQ_EINT11,buttons_all_irq,IRQT_BOTHEDGE,"s4",&pins_desc[2]);
 request_irq(IRQ_EINT19,buttons_all_irq,IRQT_BOTHEDGE,"s5",&pins_desc[3]);

 return 0;
}

static int buttons_drv_write (struct file *filp, const char __user *buf, size_t size, loff_t *offp)
{
  return 0;
}

static int buttons_drv_read (struct file *filp, char __user *buf, size_t size, loff_t *offp)
{
  wait_event_interruptible(button_waitq, ev_press);

  copy_to_user(buf, &pin_val, 1);

  if(pin_val == 0x81)
  	*gpfdat |= (1<<4); 
  
  if(pin_val == 0x1)
  	*gpfdat &= ~(1<<4); 
  
  if(pin_val == 0x82)
  	*gpfdat |= (1<<5);  
  
  if(pin_val == 0x2)
  	*gpfdat &= ~(1<<5); 
  
  if(pin_val == 0x83)
  	*gpfdat |= (1<<6);
  
  if(pin_val == 0x3)
  	*gpfdat &= ~(1<<6); 
  
  if(pin_val == 0x84)
  	*gpfdat |= (1<<4) | (1<<5) | (1<<6);
  if(pin_val == 0x4)
    *gpfdat &= ~((1<<4) | (1<<5) | (1<<6));
  ev_press = 0;
 // printk("ev_press%d",&ev_press);
  return 0;
}
static int buttons_drv_close (struct inode *inode, struct file *file)
{
  free_irq(IRQ_EINT0, &pins_desc[0]);
  free_irq(IRQ_EINT2, &pins_desc[1]);
  free_irq(IRQ_EINT11, &pins_desc[2]);
  free_irq(IRQ_EINT19, &pins_desc[3]);
  return 1;
}

int buttons_dev_fasync (int fd, struct file *file, int on)
	{
    printk("buttons_dev_fasync used!\n");
	return fasync_helper(fd, file, on, &sign);
		
	}


static struct file_operations buttons_dev_fops = {
		.owner	=	THIS_MODULE,	
		.open	=	buttons_drv_open,	   	   
		.write	=	buttons_drv_write,    
		.read   =   buttons_drv_read,
		.release=   buttons_drv_close,
		.poll   =   buttons_dev_poll,
		.fasync =   buttons_dev_fasync,
	};


int buttons_all_init(void)
{

  init_timer(&time);
  time.function = timer_function;
//  time.data = (unsigned long) dev;
//  time.expires = jiffies + HZ;
  add_timer(&time);

  major = register_chrdev(0, "buttons_all", &buttons_dev_fops);
  
  buttons_class = class_create(THIS_MODULE, "buttons_all");
  buttons_class_dev = class_device_create(buttons_class, NULL, MKDEV(major, 0), NULL, "buttons");

  gpfcon = (volatile unsigned long*)ioremap(0x56000050,16);

  gpfdat = gpfcon + 1;
  
  gpgcon = (volatile unsigned long*)ioremap(0x56000060,16);

  gpgdat = gpgcon + 1;
  return 0;
}

void buttons_all_exit(void)
{
  unregister_chrdev(major,"buttons_all");
  
  class_device_unregister(buttons_class_dev);
  class_destroy(buttons_class);

  iounmap(gpfcon);
  iounmap(gpgcon);

}

module_init(buttons_all_init);
module_exit(buttons_all_exit);

MODULE_LICENSE("GPL");





