
/*
 * drivers/amlogic/display/dlp/power_dlp3438.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
*/

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

#define GPIO_SZIE 0x4
#define GPIO_MAJOR 254

//IOCTL 命令定义
#define ALL_MEM_CLEAR 0x1  /*清0全部内存*/
#define SET_MEM_ADDR 0x2 /*设置操作的GPIO地址*/
#define WRITE_DATA 0x3 /*写IO端口*/
#define READ_DATA 0x4 /*读IO端口*/


/*gpio 设备结构体，定义自己使用的变量，并要包含一个cdev成员，与系统字符设备接口*/

struct gpio_dev
{
	struct cdev cdev;/*cdev结构体*/
	unsigned char mem[GPIO_SIZE]; //全局内存
	int addr; //gpio地址，操作哪一个gpio
};


//全局变量定义
static int gpio_major = GPIO_MAJOR; //保留申请的主设备号
struct gpio_dev *gpio_devp; //设备结构体指针
/*文件打开函数，将gpio_devp 传递给file 结构的私有数据*/

int gpio_open(struct inode *inode, struct file *filp)
{
	/*将设备结构体指针赋值给文件私有数据指针*/
	filp->private_data = gpio_devp;
	return 0;
}

/*文件释放函数*/
int gpio_release(struct inode *inode, struct file *filp)
{
	return 0;
}

//ioctl设备控制函数
//对于简单的gpio, 也许ioctl就已经足够了，而不需要read, write 接口
//这里为了完整，任然写了read,write，等完成批量内存操作
long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gpio_dev *pDev = filp->private_data;//获得设备结构体指针

	switch(cmd)
	{
		case ALL_MEM_CLEAR:
			memset(pDev->mem, 0, GPIO_SIZE);
			printk(KERN_INFO "all gpio is set to zeo\n");
			break;
		case SET_MEM_ADDR:
			pDev->addr = arg;
			printk(KERN_INFO "addr is %d\n",pDev->addr);
		case WRITE_DATA:
			pDev->mem[pDev->addr]=arg;
			GPIOSetData(pDev->addr;arg);
			printk(KERN_INFO "Data Write: %d\n", (int)arg);
			break;
		case READ_DATA:
			arg=pDev->mem[pDev->addr];
			printk(KERN_INFO "Data Read: %d\n",(int)arg);
			break;

		default:
			return -EINVAL;
	}
	return 0;
}

//读函数，可以一次读多个gpio的值，似乎有些多余，但体现read 的能力
static ssize_t gpio_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct gpio_dev *pDev = filp->private_data; //获得设备结构体指针

	printk("need size:%ld, offset:%ld\n",size,offset);

	/*分析和获取有效的写长度*/

	if(offset >GPIO_SIZE)
	{
		return count ? -ENXIO:0;
	}
	else if(offset == GPIO_SIZE)
	{
		return 0; //防止测试cat /dev/gpio 时文件尾部出现错误提示
	}
	if(count > GPIO_SIZE - offset)
	{
		count = GPIO_SIZE - offset;
	}

	/*内核空间->用户空间*/
	if(!copy_to_user(buf, (void*)(pDev->mem + offset), count))
	{
		*ppos += count;
		printk(KERN_INFO "Read %d bytes(s) from %ld addr\n", count, offset);
		ret = count;
	}
	else
	{
		ret = -EFAULT;
	}

	return ret;
}

/*写函数*/
static ssize_t gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned long offset = *ppos;
	unsigned int count = size;
	int ret = 0;
	int i;
	struct gpio_dev *pDev = filp->private_data;//获得设备结构体指针

	/*分析和获取有效的写长度*/
	if(offset >= GPIO_SIZE)
	{
		return count ? -ENXIO:0;
	}
	if(count > GPIO_SIZE - offset)
	{
		count = GPIO_SIZE - offset;
	}

	/*用户空间->内核空间*/
	if(!copy_from_user(pDev->mem + offset, buf, count))
	{
		*ppos += count;
		for(i=0; i<count;i++)
		{
			GPIOSetData(offset+i ,pDev->mem[offset+i]);
		}
		printk(KERN_INFO "Written %d bytes(s) to %ld addr\n", count, offset);
		ret = count;
	}
	else 
	{
		ret = -EFAULT;
	}
	return ret;
}
		
/*seek文件定位函数*/
static loff_t gpio_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret = 0;
	switch(orig)
	{
		case 0:
			if(offset < 0)
			{
				ret = -EINVAL;
				break;
			}
			if((unsigned int)offset > GPIO_SIZE)
			{
				ret = -EINVAL;
				break;
			}
			filp->f_ops = (unsigned int)offset;
			ret = filp->f_ops;
			break;
		case 1:
			if((filp->f_pos + offset) > GPIO_SIZE)
			{
				ret = -EINVAL;
				break;
			}
			if ((filp->f_pos + offset) < 0)
			{
				ret = -EINVAL;
				break;
			}
			filp->f_pos += offset;
			ret = filp->f_pos;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

/*文件操作结构体*/
static const struct file_operations gpio_fops = 
{
	.owner = THIS_MODULE,
	.llseek = gpio_llseek,
	.read = gpio_read,
	.write = gpio_write,
	.compat_ioctl = gpio_ioctl,
	.open = gpio_open,
	.release = gpio_release,
};

/*向系统注册设备*/
static void gpio_setup_cdev(struct gpio_dev *pDev, int index)
{
	int err,devno = MKDEV(gpio_major, index);

	cdev_init(&pDev->cdev, &gpio_fops);
	pDev->cdev.owner = THIS_MODULE;
	pDev->cdev.ops = &gpio_fops;
	err = cdev_add(&pDev->cdev, devno, 1);
	if(err)
		printk(KERN_NOTICE "Error %d adding CDEV %d", err,index);
}


/*模块加载函数*/
int gpio_init(void)
{
	int result = -1;
	dev_t devno = MKDEV(gpio_major, 0);

	/*申请设备号*/
	if(gpio_major)
	{
		result = register_chrdev_region(devno,1,"gpio");
	}
	if(result < 0)//设备号已被占用等
	{
		/*动态申请设备号*/
		result = alloc_chrdev_region(&devno,0,1,"gpio");
		gpio_major = MAJOR(devno);
	}
	if(result <0)
	{
		printk("gpio module register devno failed!, result:%d\n",result);
		return result;
	}

	/*动态申请设备结构体的内存*/
	gpio_devp = kmalloc(sizeof(struct gpio_dev),GFP_KERNEL);
	if(!gpio_devp)
	{
		result = -ENOMEM;
		goto fail_malloc;
	}
	memset(gpio_devp,0,sizeof(struct gpio_dev));

	gpio_setup_cdev(gpio_devp,0);
	//调用硬件层初始化
	GPIOInit(NULL, GPIO_SIZE);
	printk("gpio module installed!\n");
	return 0;

fail_malloc:unregister_chrdev_region(devno,1);
			return result;
}

/*模块卸载函数*/
void gpio_exit(void)
{
	if(gpio_devp)
	{
		cdev_del(&gpio_devp->cdev);/*注销cdev*/
		kfree(gpio_devp);/*释放设备结构体内存*/
		unregister_chrdev_region(MKDEV(gpio_major,0),1);/*释放设备号*/
	}
	gpio_devp = 0;
	printk(KERN_INFO "gpio module released!\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_AUTHOR("yangheyao");
MODULE_LICENSE("GPL");




