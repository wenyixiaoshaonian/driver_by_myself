#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

static struct input_dev *input_dev;
static char *usb_buf;
static dma_addr_t *usb_buf_phys;
static struct urb *new_urb;
static int len;
void new_usb_irq(struct urb *urb)
{
  static unsigned char pre_val;
#if 0	

  int i;
  static int cnt=0;
  printk("data ,cnt %d: ",++cnt);
  for(i = 0;i<len;i++)
  	{
  	  printk("%02x",usb_buf[i]);
  	}
  printk("\n");
#endif
	/*
	  data[0]: bit0-左键。1-按下，0-松开 
	  data[0]: bit1-右键。1-按下，0-松开 
	  data[0]: bit2-中键。1-按下，0-松开 
	*/ 
  if((pre_val & (1<<0)) !=(usb_buf[0]& (1<<0)))
  	{
  	  input_event(input_dev,EV_KEY,KEY_L,(usb_buf[0] & (1<<0))?1:0);
	  input_sync(input_dev);
  	}
  if((pre_val & (1<<1)) !=(usb_buf [0]& (1<<1)))
  	{
  	  input_event(input_dev,EV_KEY,KEY_S,(usb_buf[0] & (1<<1))?1:0);
	  input_sync(input_dev);
  	}
  if((pre_val & (1<<2)) !=(usb_buf [0]& (1<<2)))
  	{
  	  input_event(input_dev,EV_KEY,KEY_ENTER,(usb_buf[0] & (1<<2))?1:0);
	  input_sync(input_dev);
  	}
  pre_val = usb_buf[0];
  /* 重新提交URB */
   usb_submit_urb(new_urb, GFP_KERNEL);
}

static int new_usb_probe (struct usb_interface *intf,const struct usb_device_id *id)
{

  struct usb_device *dev = interface_to_usbdev(intf);
  struct usb_host_interface *interface;
  struct usb_endpoint_descriptor *endpoint;
  
  interface = intf->cur_altsetting;
  endpoint = &interface->endpoint[0].desc;
  
  int pipe;

  input_dev =input_allocate_device();
  
  set_bit(EV_KEY, input_dev->evbit);
  set_bit(EV_REP, input_dev->evbit);
  
  set_bit(KEY_L, input_dev->keybit);
  set_bit(KEY_S, input_dev->keybit);
  set_bit(KEY_ENTER, input_dev->keybit);
  
  input_register_device(input_dev);

  pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);  //源 USB设备的端点

  len = endpoint->wMaxPacketSize;   //长度 端点描述符内的长度

  usb_buf = usb_buffer_alloc(dev, len, GFP_ATOMIC, &usb_buf_phys);  //目的 分配一块内存

  new_urb = usb_alloc_urb(0, GFP_KERNEL);

  usb_fill_int_urb (new_urb,dev,pipe,usb_buf,len,new_usb_irq,NULL, endpoint->bInterval);      //internal: 频率 context:给完成函数用的 不需要 
  new_urb->transfer_dma = usb_buf_phys;       //物理地址
  new_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;	 //设置标记

  usb_submit_urb(new_urb, GFP_KERNEL);
  return 0;
}

void new_usb_disconnect (struct usb_interface *intf)
{
 // printk("disconnent the usb!");
 struct usb_device *dev = interface_to_usbdev(intf);
 
 usb_kill_urb(new_urb);
 usb_free_urb(new_urb);
 usb_buffer_free(dev,len,usb_buf,usb_buf_phys);
 input_unregister_device(input_dev);
 input_free_device(input_dev); 
}


static struct usb_device_id new_usb_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
    { }	/* Terminating entry */
};

static struct usb_driver usb_mouse_driver = {
	.name		= "new_usb",
	.probe		= new_usb_probe,
	.disconnect	= new_usb_disconnect,
	.id_table	= new_usb_id_table,
};

static int new_usb_init(void)
{
  usb_register(&usb_mouse_driver);
  return 0;
}

static void new_usb_exit(void)
{
  usb_deregister(&usb_mouse_driver);
}

module_init(new_usb_init);
module_exit(new_usb_exit);

MODULE_LICENSE("GPL");


