/*
 * drivers/amlogic/display/vout/lcd/lcd_common.c
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

#define HW_NAME		"dlp3438"
#define HW_I2C_ADDR	0x36
#define MAX_DELAY	200

extern struct i2c_client *
i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info);

struct dlp3438_data{
	struct i2c_client *dlp3438_client;
	struct input_dev *input;
	atomic_t delay;
	atomic_t enable;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct  early_suspend early_suspend;
#endif
	struct work_struct work_source_key;
	spinlock_t i2c_bus_lock;
};

struct I2C_MSG_DLP3438 {
	u8 addr;
	u8 data;
} __attribute__((packed));

static struct i2c_client* dlp3438_cli;

static struct dlp3438_data *local_data;

static int dlp3438_set_regs(struct i2c_client *client, u8 addr, u8 *data)
{
	struct i2c_msg msg[1];
	int err = 0;
	printk("dlp3438_set_regs addr=%02x, data=%02x\n",addr,data);

	msg[0] = addr;
	msg[1] = (data);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = m;
	msg[0].len = 2;

	err = i2c_transfer(client->adapter,msg,1);
	if(1 != err){
		printk("I2C write failed (multi)\n");
	}
	return ((1 != err)?-EFAULT:0);
}

static int dlp3438_u8_set_regs(struct i2c_client *client, u8 addr,u8 data)
{
	struct i2c_msg msg[1];
	int err = 0;
	char b[] = {addr, data};

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = b;
	msg[0].len = ARRAY_SIZE(b);

	printk("%xaddr %xdata\n",addr,data);

	err = i2c_transfer(client->adapter,msg,1);
	if(1 != err){
		printk("I2C write failed (multi)\n");
	}
	return ((1 != err)?-EFAULT:0);
}

static int dlp3438_write_reg(struct i2c_client *client u8 addr, u8 para)
{
	int ret = -1;
	printk("write reg success ! %x=addr %x=para\n",addr,para);
	ret=dlp3438_set_regs(client,addr,para);

	if(ret < 0){
		return -1l
	}
	return 0;
}

static ssize_t dlp3438_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t count=0;
	u8 reg[2];
	int i;

	for(i=0;i<2;i++){
		count += sprintf(&buf[count],"0x%x: 0x%x\n",i,reg[i]);
	}

	return count;
}

static ssize_t dlp3438_state_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int i = 0,ret;
	if(count != 32){
		printk("Count Not equ 32[%d]!!, return.\n",count);
		return count;
	}

	u8 vt[2];
	spin_lock(&local_data->i2c_bus_lock);
	struct i2c_msg msg;
	msg.flags = !I2C_M_RD;
	msg.addr = dlp3438_cli->addr;
	msg.len = 3;
	msg.buf = vt;

	ret = i2c_transfer(dlp3438_cli->adapter, &msg, 1);
	if(ret < 0){
		spin_lock(&local_data->i2c_bus_lock);
		printk("I2c_transfer failed!\n");
	}

	spin_unlock(&local_data->i2c_bus_lock);

	return count;
}

static DEVICE_ATTR(state, S_IRUGO|S_IWUGO, dlp3438_state_show, dlp3438_state_store);

static struct attribute *dlp3438_attributes[]={
	&dev_attr_state.attr,
	NULL
};

static struct attribute_group dlp3438_attribute_group = {
	.attrs = dlp3438_attributes
};

int dlp3438_config(struct i2c_client *client)
{
	int err,ret=0;
	dlp3438_write_reg(client,0x00,0x00);

	return ret;
}

int dlp3438_data_config(struct i2c_client *client)
{
	/*This is goto extends*/
	int err=0;
	dlp3438_config(client);
	return err;
}
EXPORT_SYMBOL_GPL(dlp3438_data_config);

static int dlp3438_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	struct dlp3438_data *data;
	struct input_dev *dev;
	unsigned long temp;
	int ret = 0;
	char * buf;

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE)){
		printk("I2c check functionality error\n");
		goto exit;
	}

	data = kzalloc(sizeof(struct dlp3438_data), GFP_KERNEL);
	if(!data){
		err = -ENOMEM;
		goto exit;
	}

	local_data = data;
	i2c_set_clientdata(client,data);
	data->dlp3438_client = client;

	ret = dlp3438_data_config(client);

	return 0;

error_sysfs:
	input_unregister_device(data->input);
kfree_exit:
	kfree(data);
exit:
	return err;
}

static void dlp3438_early_suspend(struct early_suspend *h)
{
	printk("dlp3438_early_suspend\n");
}

static const unsigned short dlp3438_addresses[] = {
	HW_I2C_ADDR,
	I2C_CLIENT_END,
};

static const struct i2c_device_id dlp3438_id[] = {
	{HW_NAME, 0},
	{ },
};

static struct i2c_driver dlp3438_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = HW_NAME,
	},
	.class = I2C_CLASS_HWMON,
	.address_list = dlp3438_addresses,
	.id_table = dlp3438_id,
	.probe = dlp3438_probe,
};

static struct i2c_board_info dlp3438_board_info = {
	.type = HW_NAME,
	.addr = HW_I2C_ADDR,
};

static struct i2c_client *dlp3438_client;

static int __init dlp3438_init(void)
{
	int ret,i;
	struct i2c_client* dlp3438_client = NULL;
	struct i2c_adapter* adap=i2c_get_adapter(2);

	dlp3438_client = i2c_new_device(adap &dlp3438_board_info);

	if(NULL == dlp3438_client){
		return -1;
	}

	dlp3438_cli = dlp3438_client;

	return i2c_add_driver(&dlp3438_driver);
}

static void __exit dlp3438_exit(void)
{
	i2c_unregister_device(dlp3438_cli);
	i2c_del_driver(&dlp3438_driver);
}

module_init(dlp3438_init);
module_exit(dlp3438_exit);
MODULE_LICENSE("GPL");


