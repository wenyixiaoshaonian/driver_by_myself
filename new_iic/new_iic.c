#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <asm/io.h>


static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = {0x68,I2C_CLIENT_END }; // 前七位


static unsigned short force_addr[] = {ANY_I2C_BUS,0x68,I2C_CLIENT_END}; //强制让设备地址为0x60的设备存在

static unsigned short *forces[] = { force_addr,NULL};

static volatile unsigned long *retcon;
static struct i2c_driver new_iic;

static struct i2c_client *new_client;
static int major;
static struct class *cls;

static int  new_i2c_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
  unsigned char address;
  unsigned char date;
  int result;
  struct i2c_msg msgs[2];
  copy_from_user(&address,buf,1);
  //先写地址
  msgs[0].addr   = new_client->addr;
  msgs[0].buf    = &address;
  msgs[0].len    = 1;
  msgs[0].flags  = 0;
  // 再读数据
  msgs[1].addr   = new_client->addr;
  msgs[1].buf    = &date;
  msgs[1].len    = 1;
  msgs[1].flags  = 1;
  result = i2c_transfer(new_client->adapter,msgs,2);
  if(result == 2)
  	{
	  copy_to_user(buf, &date, 1);
	  return 1;
	}
  else 
    return -EIO;
  return 0;
}
 
static int new_i2c_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
  unsigned char val[2];     //第一个是地址，第二个是数据
  int result;
  struct i2c_msg msgs;
  copy_from_user(val,buf,2);

  msgs.addr   = new_client->addr;
  msgs.buf    = val;
  msgs.len    = 2;
  msgs.flags  = 0;
  
  result = i2c_transfer(new_client->adapter,&msgs,1);
  if(result == 1)
  	return 2;
  else 
  return -EIO;
}


struct file_operations  new_i2c_ops = 
{ 
  .owner = THIS_MODULE,
  .read  = new_i2c_read,
  .write = new_i2c_write,
};


static struct i2c_client_address_data addr_data = {
	.normal_i2c = normal_addr,
	.probe  = ignore,
	.ignore = ignore,
	.forces  = forces,            //强制认为设备存在
};

static int new_iic_probe(struct i2c_adapter *adapter, int address, int kind)
{
	printk("new_iic_probe\n");
    new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	new_client->addr = address;
	new_client->adapter = adapter;
	new_client->driver = &new_iic;
	strlcpy(new_client->name, "new_I2C", I2C_NAME_SIZE);
    i2c_attach_client(new_client);

	//字符设备驱动函数

	major = register_chrdev(0, "new_i2c2", &new_i2c_ops);

	cls = class_create(THIS_MODULE, "new_i2c2");
	class_device_create(cls, NULL, MKDEV(major, 0), NULL, "new_i2c2");
	printk("new_class_device_create!\n");
	return 0;
}

static int new_iic_attach(struct i2c_adapter *adapter)
{
    printk("new_iic_attach\n");
	return i2c_probe(adapter, &addr_data, new_iic_probe);
}

static int new_iic_detach(struct i2c_client *client)
{
  printk("new_iic_detach\n");
  class_device_destroy(cls,MKDEV(major, 0));
  class_destroy(cls);
  unregister_chrdev(major,"new_i2c");
  
  i2c_detach_client(new_client);
  kfree(i2c_get_clientdata(new_client));
  return 0;
}

static struct i2c_driver new_iic =
{
  .driver = {
     .name	= "new_iic",
	},
  .id = I2C_DRIVERID_DS1374,
  .attach_adapter = new_iic_attach,
  .detach_client =  new_iic_detach,
};

static int new_iic_init(void)
{
  i2c_add_driver(&new_iic);
  return 0;
}

static void new_iic_exit(void)
{
  i2c_del_driver(&new_iic);
}

module_init(new_iic_init);
module_exit(new_iic_exit);

MODULE_LICENSE("GPL");

