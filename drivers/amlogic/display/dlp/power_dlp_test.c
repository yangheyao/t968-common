#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/reset.h>
#include <dt-bindings/gpio/gxtvbb.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#define HW_NAME "dlp3438_test"
#define DLP_ONOFF     ((GPIOY_7)+135)
#define DLP_RESET     ((GPIOY_8)+135)

int request_io(void)
{
  int err=0;

  gpio_free(DLP_ONOFF);
  gpio_free(DLP_RESET);
  err = gpio_request(DLP_RESET,HW_NAME);
  if(err){
    printk("request pin GPIOY_8 is ERROR!![%d]\n",err);
    goto err_out;
  }
  err = gpio_direction_output(DLP_RESET,1);
  if(err){
    printk("SET:direction_output DLP_RESET ERROR!![%d]\n",err);
    goto err_out;
  }
  
  gpio_set_value(DLP_RESET,0);
  mdelay(100);
  gpio_set_value(DLP_RESET,1);

  err = gpio_request(DLP_ONOFF,HW_NAME);
  if(err){
    printk("request pin GPIOY_7 88 is ERROR!![%d]\n",err);
    goto err_out;
  }
  err = gpio_direction_output(DLP_ONOFF,1);
  if(err){
    printk("SET: direction_output DLP_ONOFF ERROR!![%d]\n",err);
    goto err_out;
  }

  gpio_set_value(DLP_ONOFF,1);

  gpio_set_value(DLP_RESET,0);
  mdelay(100);
  gpio_set_value(DLP_RESET,1);


 err_out:
  return  err;
}

int freeio(void)
{
  gpio_free(DLP_ONOFF);
  gpio_free(DLP_RESET);
  return 0;
}

static int __init test_io_init(void)
{
  int rvl;

  rvl = request_io();
  return 0;
}

static void __exit test_io_exit(void)
{
  freeio();
  printk("=========Test IO Exit!! %s ========<%d>\n",HW_NAME,2);
}


MODULE_AUTHOR("Jonah-Ynag <yangheyao@cloudesteem.cn");
MODULE_DESCRIPTION("Test-GPIO function(read)");
MODULE_LICENSE("GPL");
module_init(test_io_init);
module_exit(test_io_exit);
  
