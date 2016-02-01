/*
 * Copyright (C) 2011-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/fsl_devices.h>
#include <linux/mipi_csi2.h>
#include <media/v4l2-chip-ident.h>
#include "v4l2-int-device.h"
#include "mxc_v4l2_capture.h"

#include "max9286.h"
#include "max9286_config3.h"

#define max9286_VOLTAGE_ANALOG               2800000
#define max9286_VOLTAGE_DIGITAL_CORE         1500000
#define max9286_VOLTAGE_DIGITAL_IO           1800000

#define MIN_FPS 15
#define MAX_FPS 30
#define DEFAULT_FPS 30

#define max9286_XCLK_MIN 6000000
#define max9286_XCLK_MAX 24000000

#define max9286_CHIP_ID_HIGH_BYTE	0x300A
#define max9286_CHIP_ID_LOW_BYTE	0x300B

static int max9286_probe(struct i2c_client *adapter,
				const struct i2c_device_id *device_id);
static int max9286_remove(struct i2c_client *client);
static const struct i2c_device_id max9286_id[] = {
	{"max9286_des", 0},
	{},
};

static int max9286_read(struct i2c_client *client, u8 reg);
static int max9286_write(struct i2c_client *client, u8 reg, char value);
static int max9286_dump(struct i2c_client *client);
static int max9286_reset(void);

static struct i2c_board_info gOvDeserialInfo = {
	I2C_BOARD_INFO("max9286_des", 0x6a),
};

MODULE_DEVICE_TABLE(i2c, max9286_id);

static struct i2c_driver max9286_i2c_driver = {
	.driver = {
		  .owner = THIS_MODULE,
		  .name  = "max9286_des",
		  },
	.probe  = max9286_probe,
	.remove = max9286_remove,
	.id_table = max9286_id,
};

/*!
 * Maintains the information on the current state of the sensor.
 */
struct deserializer {
	struct i2c_client *i2c_client;
	bool on;
} max9286_data;



/*!
 * max9286 I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */

static int max9286_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	//struct device *dev = &client->dev;
	int retval, buf;

	/* Set initial values for the deserializer struct. */
	memset(&max9286_data, 0, sizeof(max9286_data));
	max9286_data.i2c_client = client;

	max9286_reset();

	//u8 chip_id_high, chip_id_low;

	pr_err("%s:probe deserializer\n", __func__);

	retval = i2c_smbus_read_byte_data(client, 0x1E);
	if (retval!=0x40)
		pr_err("Deserializer not recognized 0x%x.\n", retval);
	else
		pr_err("Deserializer recognized 0x%x.\n", retval);

	//Enable Custom Reverse Channel & First Pulse Lenght *
	retval = i2c_smbus_write_byte_data(client, 0x3F, 0x4F);

	msleep(2);

	//Reverse channel pulse lenght (oscillator clock cycles)
	retval = i2c_smbus_write_byte_data(client, 0x3B, 0x1E);

	msleep(2);

	return 0;
}

static int max9286_initial_setup(void)
{

	int ret;

//	ret = max9286_dump(max9286_data.i2c_client);

	// PART 2 - MAX9286 Initial Setup

	//Disable CSI output
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x15, 0x03);

	//Enable CSI-2 Lanes D0, D1, D2, D3
	//Enable CSI-2 DBL
	//Enable GMSL DBL for Rawx2
	//Enable RAW8 data type
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x12, 0xf7); //0x77 0xe5);

	//Enable frame sync
	//Enable semi-auto frame sync
	//Use semi-auto for row reset on frame sync sensors
	//Use auto for row/column reset on frame sync sensors
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x01, 0xC1);//0x01);

	//Force master link or select auto. First GMSL link
	//to lock will be master link with auto select
	//Disable internal VSYNC generation. Use free running
	//VSYNC from image sensor
	//Enable GMSL links ////e1
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x00, 0xe1);

	return 0;

}
EXPORT_SYMBOL(max9286_initial_setup);

static int max9286_reset(void)
{
	int ret, i;

	int reg[] = {
	0xef,0x22,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0xd4,0xff,0xe4,0x99,0xf3,0x50,0x0b,
	0x00,0x00,0xf4,0x0f,0xe4,0x0b,0x00,0x01,0x00,0x00,0x60,0x00,0x04,0xff,0x40,0x02,
	0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x99,0x99,0xb6,0x00,0x00,0x00,0x00,0x00,0x00,0x2e,0x24,0x54,0xc8,0x22, 
	};

	printk("Reseting max9286 registers\n");

	for( i = 0; i <= 0x40; i++)
	{
		if((i%16) == 0)
		{
			printk("\n%x: ",i/16);
		}
		ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, i, reg[i]);
		printk("0x%02x ", reg[i]);
	}

	return 0;
}
EXPORT_SYMBOL(max9286_reset);

static int max9286_increase_rev_amplitude(void)
{
	int ret;
	//Increase reverse amplitude from 100mV to 170mV
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x3B, 0x19);

	//max9286_write_reg(0x0D, 0xF7);
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x0D, 0xF7);

	msleep(2);

	return 0;
}
EXPORT_SYMBOL(max9286_increase_rev_amplitude);

static int max9286_enable_rev_channel(int cam_id)
{
	int ret, link_ctrl = 0;

	link_ctrl = (0x01 << cam_id) | 0xf0;

	// Enabling only one reverse channel 
	printk (KERN_INFO "Enabling only one reverse channel @0x0A: %x\n", link_ctrl);
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x0A, link_ctrl);

	return 0;
}
EXPORT_SYMBOL(max9286_enable_rev_channel);

static int max9286_disable_auto_ack(void)
{
	int ret;

	// Disable auto acknowledge
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x34, 0x36);

	return 0;
}
EXPORT_SYMBOL(max9286_disable_auto_ack);

static int max9286_enable_auto_ack(void)
{
	int ret;

	// Enable auto acknowledge
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x34, 0xB6);

	return 0;
}
EXPORT_SYMBOL(max9286_enable_auto_ack);

static int max9286_enable_csi_output(void)
{
	int ret;

	// Enable CSI-2 output
	//ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x15, 0x0B);
	ret = i2c_smbus_write_byte_data(max9286_data.i2c_client, 0x15, 0x9B);
	msleep(10);

	return 0;
}
EXPORT_SYMBOL(max9286_enable_csi_output);

static int max9286_check_frame_sync(void)
{
	int ret;

	// Check frame sync bit
	ret = i2c_smbus_read_byte_data(max9286_data.i2c_client, 0x31);
	if (ret && 0x40) {
		printk(KERN_INFO "Frame sync is locked\n");
		return 0;
	}
	else {
		printk(KERN_INFO "Frame sync is not locked\n");
		return -1;
	}
}
EXPORT_SYMBOL(max9286_check_frame_sync);

/*!
 * max9286 I2C dump
 *
 * @param client            struct i2c_client *
 */
static int max9286_dump(struct i2c_client *client)
{

	struct device *dev = &client->dev;
	int ret, i;

	printk(KERN_INFO "Printing MAX9286, i2caddr: 0x%x\n", client->addr);
	printk(KERN_INFO "   ");
	for( i = 0; i < 0x10; i++)
	{
		printk("%4x ",i);
	}
	
	for( i = 0; i <= 0x71; i++)
	{
		if((i%16) == 0)
		{
			printk("\n%x: ",i/16);
		}
//		ret = i2c_smbus_read_byte_data(client, i);
		printk("0x%02x ", ret);
	}
	printk(KERN_INFO"\n");

	return 0;

}

/***********************************************************************
 * I2C transfer.
 ***********************************************************************/

static int max9286_read(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

/*!
 * This function do one mag3110 register write.
 */
static int max9286_write(struct i2c_client *client, u8 reg, char value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);
	if (ret < 0)
		dev_err(&client->dev, "i2c write failed\n");
	return ret;
}


/*!
 * max9286 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int max9286_remove(struct i2c_client *client)
{
	//v4l2_int_device_unregister(&max9286_int_device);

/*	if (gpo_regulator)
		regulator_disable(gpo_regulator);

	if (analog_regulator)
		regulator_disable(analog_regulator);

	if (core_regulator)
		regulator_disable(core_regulator);

	if (io_regulator)
		regulator_disable(io_regulator);
*/
	return 0;
}

/*!
 * max9286 init function
 * Called by insmod max9286_camera_mipi_maxim_2.ko.
 *
 * @return  Error code indicating success or failure
 */
static __init int max9286_init(void)
{
	u8 err;
	
	pr_err("%s:driver registration init\n",
			__func__);
	err = i2c_add_driver(&max9286_i2c_driver);
	if (err != 0)
		pr_err("%s:driver registration failed, error=%d\n",
			__func__, err);
	else
		pr_err("Registered. %d\n", err);

	return err;
}

/*!
 * max9286 cleanup function
 * Called on rmmod max9286_camera_mipi_maxim_2.ko
 *
 * @return  Error code indicating success or failure
 */
static void __exit max9286_clean(void)
{
	i2c_del_driver(&max9286_i2c_driver);
}

module_init(max9286_init);
module_exit(max9286_clean);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("max9286 MIPI Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("CSI");


