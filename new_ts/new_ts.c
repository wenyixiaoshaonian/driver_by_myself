#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/plat-s3c24xx/ts.h>

#include <asm/arch/regs-adc.h>
#include <asm/arch/regs-gpio.h>

#define ERR_LIMIT 10

struct ts_reg 
{
 unsigned long adccon;
 unsigned long adctsc;
 unsigned long adcdly;
 unsigned long adcdat0;
 unsigned long adcdat1;
 unsigned long adcupdn;
};


static struct input_dev *ts_info ;
static volatile struct ts_reg* ts_regs;
static struct timer_list    ts_time;

static void wait_down_action(void)
{
	ts_regs->adctsc = 0xd3;
}


static void wait_up_action(void)
{
	ts_regs->adctsc = 0x1d3;
}

static void start_adc(void)
{
  ts_regs->adccon |= 1 ;     //使能ADC
}

static void measure_mode(void)
{
  ts_regs->adctsc |= (1<<3) | (1<<2);    //断开上拉电阻，开启自动转换模式
}

static int new_filter (int x[],int y[])
{
  int arv_x,arv_y;
  int det_x,det_y;

  arv_x = (x[0]+x[1])/2 ;
  arv_y = (y[0]+y[1])/2 ;

  det_x = (arv_x > x[2]) ? (arv_x - x[2]):(x[2] - arv_x);
  det_y = (arv_y > y[2]) ? (arv_y - y[2]):(y[2] - arv_y);
  if((det_x > ERR_LIMIT) || (det_y > ERR_LIMIT))
  	return 0;

  arv_x = (x[1]+x[2])/2 ;
  arv_y = (y[1]+y[2])/2 ;

  det_x = (arv_x > x[3]) ? (arv_x - x[3]):(x[3] - arv_x);
  det_y = (arv_y > y[3]) ? (arv_y - y[3]):(y[3] - arv_y);
  if((det_x > ERR_LIMIT) || (det_y > ERR_LIMIT))
  	return 0;

  return 1;
}

static void ts_timer_irq(unsigned long data)
{
  if(ts_regs->adcdat0 & (1<<15))
  	{
      input_report_abs(ts_info, ABS_PRESSURE, 0);
	  input_report_key(ts_info, BTN_TOUCH, 0);
	  input_sync(ts_info);
	  wait_down_action();
  	}
  else 
  	{
  	  measure_mode();
	  start_adc();
  	}

}

static irqreturn_t new_adc_irq(int irq, void *dev_id)
{
  static int cnt = 0;
  int x,y;
  static int x_val[4],y_val[4];
  
  x = ts_regs->adcdat0;
  y = ts_regs->adcdat1;
  
  if(ts_regs->adcdat0 & (1<<15))     //防止过早松开，未完成转换
  	{
      cnt = 0;  	
	  input_report_abs(ts_info, ABS_PRESSURE, 0);
	  input_report_key(ts_info, BTN_TOUCH, 0);
	  input_sync(ts_info);
	  wait_down_action();
  	}
  else
  	{
		  x_val[cnt] = x & 0x3ff;
		  y_val[cnt] = y & 0x3ff;
          ++cnt;
		  
		  if(cnt == 4)
		  	{
		  	  if(new_filter(x_val,y_val))
		  	  	{
					input_report_abs(ts_info, ABS_X, (x_val[0]+x_val[1]+x_val[2]+x_val[3])/4);
				    input_report_abs(ts_info, ABS_Y, (y_val[0]+y_val[1]+y_val[2]+y_val[3])/4);
				    input_report_abs(ts_info, ABS_PRESSURE, 1);
				    input_report_key(ts_info, BTN_TOUCH, 1);
				    input_sync(ts_info);
		          //  printk("x:%d, y:%d\n",(x_val[0]+x_val[1]+x_val[2]+x_val[3])/4,(y_val[0]+y_val[1]+y_val[2]+y_val[3])/4); //多次测量 取平均值
		  	  	}
			  cnt = 0;
              wait_up_action();
		      mod_timer(&ts_time, jiffies + HZ/100 );
		  	}
		  else
		  	{
		  	  measure_mode();
	          start_adc();  	
		  	}
  	}
      return IRQ_HANDLED;
}

static irqreturn_t ts_down_action(int irq, void *dev_id)
{
   if(ts_regs->adcdat0 & (1<<15))
  	{      
  	  input_report_abs(ts_info, ABS_PRESSURE, 0);
	  input_report_key(ts_info, BTN_TOUCH, 0);
	  input_sync(ts_info);
	  wait_down_action();
  	}
  else
  	{	  
  	  input_report_abs(ts_info, ABS_X, 2);
	  measure_mode();
	  start_adc();
  	}

  return IRQ_HANDLED;
}


static int new_ts_init(void)
{
 struct clk	*adc_clock;
 int error;
 ts_info = input_allocate_device();


   
 set_bit(EV_KEY, ts_info->evbit);
 set_bit(EV_ABS, ts_info->evbit);
 
 set_bit(BTN_TOUCH, ts_info->keybit);

 input_set_abs_params(ts_info, ABS_X, 0, 0x3FF, 0, 0);
 input_set_abs_params(ts_info, ABS_Y, 0, 0x3FF, 0, 0);
 input_set_abs_params(ts_info, ABS_PRESSURE, 0, 1, 0, 0);

 input_register_device(ts_info);


 //硬件方面的设置
 adc_clock = clk_get(NULL, "adc");
 clk_enable(adc_clock);
 
 ts_regs = ioremap(0x58000000,sizeof(struct ts_reg));


 ts_regs->adccon = (1<<14) | (49<<6) | (0<<0);
 //使能converter prescaler;PCLK/(prescaler value+1) :49 ; ADC使能 先关闭，使用时开启
 
 request_irq (IRQ_TC, ts_down_action,IRQF_SAMPLE_RANDOM,"touch_pen",NULL);
 request_irq (IRQ_ADC, new_adc_irq,IRQF_SAMPLE_RANDOM,"adc",NULL);

 ts_regs->adcdly = 0xffff;    //增加一段延迟，使按下电压稳定后再执行中断

 init_timer(&ts_time);
 ts_time.function =  ts_timer_irq;
 add_timer(&ts_time);
 wait_down_action();

 return 0;
}

static void new_ts_exit(void)
{
  iounmap(ts_regs);
  input_unregister_device(ts_info);
  input_free_device(ts_info); 
  del_timer(&ts_time);
  free_irq(IRQ_TC,NULL);
  free_irq(IRQ_ADC, NULL);
}

module_init(new_ts_init);
module_exit(new_ts_exit);

MODULE_LICENSE("GPL");



