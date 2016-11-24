

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

//#define 

int gpio_init(void)
{
	/*int ret = 0;
	ret = set_proj_on();
	if(ret)
	{*/
		printk("gpio_init proj is ok\n");
	//}
	return 0;
}
void gpio_exit(void)
{
	/*int ret = 0;
	ret = remove_proj_on();
	if(ret)
	{*/
		printk("gpio_exit proj is remove\n");
	//}
}
module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");
