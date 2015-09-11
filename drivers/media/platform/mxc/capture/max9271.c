
/*
 * Copyright 2004-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/ipu.h>
#include <linux/mipi_csi2.h>
#include <linux/delay.h>
#include <media/v4l2-chip-ident.h>
#include "v4l2-int-device.h"
#include "mxc_v4l2_capture.h"
#include "max9271.h"


static int max9271_remove(struct i2c_client *client);
static int max9271_probe(struct i2c_client *client,
			const struct i2c_device_id *id);
static int max9271_dump(struct i2c_client *client);
static int max9271_setaddr(int);

//static struct i2c_board_info SerialInfo = {
//	I2C_BOARD_INFO("max9271_des", 0x41),
//};

static const struct i2c_device_id max9271_id[] = {
	{"max9271_ser", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, max9271_id);

static struct i2c_driver max9271_i2c_driver = {
	.driver = {
		  .owner = THIS_MODULE,
		  .name  = "max9271_ser",
		  },
	.probe  = max9271_probe,
	.remove = max9271_remove,
	.id_table = max9271_id,
};

/*!
 * Maintains the information on the current state of the sensor.
 */
struct serializer {
	struct i2c_client *i2c_client;
	bool on;
} max9271_data;

/*!
 * max9271 init function
 * Called by insmod max9271_ser.ko.
 *
 * @return  Error code indicating success or failure
 */
static __init int max9271_init(void)
{
	u8 err;
	
	pr_err("%s:driver registration init\n",
			__func__);
	err = i2c_add_driver(&max9271_i2c_driver);
	if (err != 0)
		pr_err("%s:driver registration failed, error=%d\n",
			__func__, err);
	else
		pr_err("Registered. %d\n", err);

	return err;
}


/*!
 * function to probe MAX9271
 *
 * @return  status
 */


static int max9271_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	u8 channel_id;
	int retval;
	int rev_id;
	int ret = 0;
	u32 cvbs = true;
	struct device *dev = &client->dev;

	printk(KERN_INFO "%s\n", __func__);

	/* Set initial values for the sensor struct. */
	memset(&max9271_data, 0, sizeof(max9271_data));
	max9271_data.i2c_client = client;
	max9271_data.on = true;

	return 0;
}

static int max9271_initial_setup(void)
{
	int i2c_addr, retval, ret;

	i2c_addr = max9271_data.i2c_client->addr;
	max9271_data.i2c_client->addr = 0x40; //Default i2c address for MAX9271

	//Enable config link
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x04, 0x43);

	msleep(5);

	//Enable high threshould for reverse channel input
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x08, 0x01);

	ret = max9286_increase_rev_amplitude();

	msleep(2);

	//MAX9286 Initial Setup
	
	ret = max9286_initial_setup();

	//GMSL Link Setup Procedure

	ret = max9271_dump(max9271_data.i2c_client);	

	ret = max9286_enable_rev_channel(0);


	//Changing serializer slave address
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x00, 0x82);

	msleep(1);
	max9271_data.i2c_client->addr = i2c_addr; //Set correct i2c address for MAX9271

	//Enable DBL
	//Set Edge Select 1 = Rise / 0 = Fall
	//Enable HS/VS encoding
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x07, 0x94);

	//Unique Link 0 image sensor slave address
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x09, 0x62);

	//Link 0 image sensor slave address
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x0A, 0x60);

	//Serializer broadcast address
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x0B, 0x8A);

	//Link 0 serializer address
	retval = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x0C, 0x82);

	ret = max9271_dump(max9271_data.i2c_client);

	//ret = ov10640_dump(client);

	//Image Sensor Initialization Procedure

	//Disable auto acknowledge
	ret = max9286_disable_auto_ack();

	//Enable auto acknowledge
	//ret = max9286_enable_auto_ack();


	return 0;
}
EXPORT_SYMBOL(max9271_initial_setup);

static int max9271_enable_serial_links(void)
{
	int ret;

	//Enable all serial links
	ret = i2c_smbus_write_byte_data(max9271_data.i2c_client, 0x04, 0x83);
	msleep(5);
	
	//Poll frame sync.
	ret = max9286_check_frame_sync();

	//Enable CSI-2 output
	ret = max9286_enable_csi_output();

	return 0;

}
EXPORT_SYMBOL(max9271_enable_serial_links);


static int max9271_dep(void) // temp function to make dependency. To be removed.
{

	return 0;
}
EXPORT_SYMBOL(max9271_dep);

/*!
 * max9271 I2C dump
 *
 * @param client            struct i2c_client *
 */
static int max9271_dump(struct i2c_client *client)
{

	int ret, i;

	printk(KERN_INFO "Printing MAX9271, i2caddr: 0x%x\n", client->addr);
	printk(KERN_INFO "   ");
	for( i = 0; i < 0x10; i++)
	{
		printk("%4x ",i);
	}
	
	for( i = 0; i <= 0x1f; i++)
	{
		if((i%16) == 0)
		{
			printk("\n%x: ",i/16);
		}
		ret = i2c_smbus_read_byte_data(client, i);
		printk("0x%02x ", ret);
	}
	printk(KERN_INFO"\n");

	return 0;

}

/*!
 * max9271 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int max9271_remove(struct i2c_client *client)
{
	return 0;
}

/*!
 * Deinit max9271 driver.
 *
 * @return  Error code indicating success or failure
 */
void __exit max9271_exit(void)
{
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

module_init(max9271_init);
module_exit(max9271_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MAX9271 Camera Serializer Driver");
MODULE_LICENSE("GPL");
