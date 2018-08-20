
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct net_device *new_virt;

static int new_virt_net_init(void)
{
  new_virt = alloc_netdev(0, "vent%d", ether_setup);    //ether_setup 默认设置函数

  register_netdev(new_virt);
  return 0;
}

static void new_virt_exit(void)
{
  unregister_netdev(new_virt);
  free_netdev(new_virt);
}

module_init(new_virt_net_init);
module_exit(new_virt_exit);

MODULE_LICENSE("GPL");



