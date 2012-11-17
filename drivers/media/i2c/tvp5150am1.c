/*
 * tvp5150 - Texas Instruments TVP5150A/AM1 video decoder driver
 *
 * Copyright (c) 2012 Mike Dyer <mike.dyer@md-soft.co.uk>
 * This code is placed under the terms of the GNU General Public License v2
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>

#include "tvp5150_reg.h"

MODULE_DESCRIPTION("TVP5150AM1 Simplified Driver");
MODULE_AUTHOR("Mike Dyer");
MODULE_LICENSE("GPL");

#define MODULE_NAME "tvp5150"

extern const unsigned char tvp5150am1_patch[];
extern const unsigned int tvp5150am1_patch_size;

static int debug;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Debug level (0-2)");

struct tvp5150 {
	struct v4l2_subdev sd;
	struct media_pad pad;
};

static inline struct tvp5150 *to_tvp5150(struct v4l2_subdev *sd)
{
	return container_of(sd, struct tvp5150, sd);
}

static int tvp5150_read(struct v4l2_subdev *sd, unsigned char addr)
{
	struct i2c_client *c = v4l2_get_subdevdata(sd);
	unsigned char buffer[1];
	int rc;
	int tries = 0;

	while (tries < 10) {
		buffer[0] = addr;
		if (1 != (rc = i2c_master_send(c, buffer, 1))) {
			v4l2_dbg(0, debug, sd, "i2c i/o error (rd tx): rc == %d (should be 1)\n", rc);
			tries ++;
			continue;
		}

		msleep(10);

		if (1 != (rc = i2c_master_recv(c, buffer, 1))) {
			v4l2_dbg(0, debug, sd, "i2c i/o error (rd rx): rc == %d (should be 1)\n", rc);
			tries ++;
		} else {
			tries = 20;
		}
	}
	v4l2_dbg(3, debug, sd, "tvp5150: read 0x%02x = 0x%02x\n", addr, buffer[0]);

	if (tries != 20)
		v4l2_dbg(0, debug, sd, "i2c i/o error (rd): tries exceeded\n");

	return (buffer[0]);
}

static inline void tvp5150_write(struct v4l2_subdev *sd, unsigned char addr,
				 unsigned char value)
{
	struct i2c_client *c = v4l2_get_subdevdata(sd);
	unsigned char buffer[2];
	int rc;

	buffer[0] = addr;
	buffer[1] = value;
	v4l2_dbg(3, debug, sd, "tvp5150: writing 0x%02x 0x%02x\n", buffer[0], buffer[1]);
	if (2 != (rc = i2c_master_send(c, buffer, 2)))
		v4l2_dbg(0, debug, sd, "i2c i/o error (wr tx): rc == %d (should be 2)\n", rc);
}

static void dump_reg_range(struct v4l2_subdev *sd, char *s, u8 init,
				const u8 end, int max_line)
{
	int i = 0;

	while (init != (u8)(end + 1)) {
		if ((i % max_line) == 0) {
			if (i > 0)
				printk("\n");
			printk("tvp5150: %s reg 0x%02x = ", s, init);
		}
		printk("%02x ", tvp5150_read(sd, init));

		init++;
		i++;
	}
	printk("\n");
}

static int tvp5150_log_status(struct v4l2_subdev *sd)
{
	printk("tvp5150: Video input source selection #1 = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VD_IN_SRC_SEL_1));
	printk("tvp5150: Analog channel controls = 0x%02x\n",
			tvp5150_read(sd, TVP5150_ANAL_CHL_CTL));
	printk("tvp5150: Operation mode controls = 0x%02x\n",
			tvp5150_read(sd, TVP5150_OP_MODE_CTL));
	printk("tvp5150: Miscellaneous controls = 0x%02x\n",
			tvp5150_read(sd, TVP5150_MISC_CTL));
	printk("tvp5150: Autoswitch mask= 0x%02x\n",
			tvp5150_read(sd, TVP5150_AUTOSW_MSK));
	printk("tvp5150: Color killer threshold control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_COLOR_KIL_THSH_CTL));
	printk("tvp5150: Luminance processing controls #1 #2 and #3 = %02x %02x %02x\n",
			tvp5150_read(sd, TVP5150_LUMA_PROC_CTL_1),
			tvp5150_read(sd, TVP5150_LUMA_PROC_CTL_2),
			tvp5150_read(sd, TVP5150_LUMA_PROC_CTL_3));
	printk("tvp5150: Brightness control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_BRIGHT_CTL));
	printk("tvp5150: Color saturation control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_SATURATION_CTL));
	printk("tvp5150: Hue control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_HUE_CTL));
	printk("tvp5150: Contrast control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_CONTRAST_CTL));
	printk("tvp5150: Outputs and data rates select = 0x%02x\n",
			tvp5150_read(sd, TVP5150_DATA_RATE_SEL));
	printk("tvp5150: Configuration shared pins = 0x%02x\n",
			tvp5150_read(sd, TVP5150_CONF_SHARED_PIN));
	printk("tvp5150: Active video cropping start = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_ST_MSB),
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_ST_LSB));
	printk("tvp5150: Active video cropping stop  = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_STP_MSB),
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_STP_LSB));
	printk("tvp5150: Genlock/RTC = 0x%02x\n",
			tvp5150_read(sd, TVP5150_GENLOCK));
	printk("tvp5150: Horizontal sync start = 0x%02x\n",
			tvp5150_read(sd, TVP5150_HORIZ_SYNC_START));
	printk("tvp5150: Vertical blanking start = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VERT_BLANKING_START));
	printk("tvp5150: Vertical blanking stop = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VERT_BLANKING_STOP));
	printk("tvp5150: Chrominance processing control #1 and #2 = %02x %02x\n",
			tvp5150_read(sd, TVP5150_CHROMA_PROC_CTL_1),
			tvp5150_read(sd, TVP5150_CHROMA_PROC_CTL_2));
	printk("tvp5150: Interrupt reset register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_RESET_REG_B));
	printk("tvp5150: Interrupt enable register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_ENABLE_REG_B));
	printk("tvp5150: Interrupt configuration register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INTT_CONFIG_REG_B));
	printk("tvp5150: Video standard = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VIDEO_STD));
	printk("tvp5150: Chroma gain factor: Cb=0x%02x Cr=0x%02x\n",
			tvp5150_read(sd, TVP5150_CB_GAIN_FACT),
			tvp5150_read(sd, TVP5150_CR_GAIN_FACTOR));
	printk("tvp5150: Macrovision on counter = 0x%02x\n",
			tvp5150_read(sd, TVP5150_MACROVISION_ON_CTR));
	printk("tvp5150: Macrovision off counter = 0x%02x\n",
			tvp5150_read(sd, TVP5150_MACROVISION_OFF_CTR));
	printk("tvp5150: ITU-R BT.656.%d timing(TVP5150AM1 only)\n",
			(tvp5150_read(sd, TVP5150_REV_SELECT) & 1) ? 3 : 4);
	printk("tvp5150: Device ID = %02x%02x\n",
			tvp5150_read(sd, TVP5150_MSB_DEV_ID),
			tvp5150_read(sd, TVP5150_LSB_DEV_ID));
	printk("tvp5150: ROM version = (hex) %02x.%02x\n",
			tvp5150_read(sd, TVP5150_ROM_MAJOR_VER),
			tvp5150_read(sd, TVP5150_ROM_MINOR_VER));
	printk("tvp5150: Vertical line count = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_VERT_LN_COUNT_MSB),
			tvp5150_read(sd, TVP5150_VERT_LN_COUNT_LSB));
	printk("tvp5150: Interrupt status register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_STATUS_REG_B));
	printk("tvp5150: Interrupt active register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_ACTIVE_REG_B));
	printk("tvp5150: Status regs #1 to #5 = %02x %02x %02x %02x %02x\n",
			tvp5150_read(sd, TVP5150_STATUS_REG_1),
			tvp5150_read(sd, TVP5150_STATUS_REG_2),
			tvp5150_read(sd, TVP5150_STATUS_REG_3),
			tvp5150_read(sd, TVP5150_STATUS_REG_4),
			tvp5150_read(sd, TVP5150_STATUS_REG_5));

	dump_reg_range(sd, "Teletext filter 1",   TVP5150_TELETEXT_FIL1_INI,
			TVP5150_TELETEXT_FIL1_END, 8);
	dump_reg_range(sd, "Teletext filter 2",   TVP5150_TELETEXT_FIL2_INI,
			TVP5150_TELETEXT_FIL2_END, 8);

	printk("tvp5150: Teletext filter enable = 0x%02x\n",
			tvp5150_read(sd, TVP5150_TELETEXT_FIL_ENA));
	printk("tvp5150: Interrupt status register A = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_STATUS_REG_A));
	printk("tvp5150: Interrupt enable register A = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_ENABLE_REG_A));
	printk("tvp5150: Interrupt configuration = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_CONF));
	printk("tvp5150: VDP status register = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VDP_STATUS_REG));
	printk("tvp5150: FIFO word count = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_WORD_COUNT));
	printk("tvp5150: FIFO interrupt threshold = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_INT_THRESHOLD));
	printk("tvp5150: FIFO reset = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_RESET));
	printk("tvp5150: Line number interrupt = 0x%02x\n",
			tvp5150_read(sd, TVP5150_LINE_NUMBER_INT));
	printk("tvp5150: Pixel alignment register = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_PIX_ALIGN_REG_HIGH),
			tvp5150_read(sd, TVP5150_PIX_ALIGN_REG_LOW));
	printk("tvp5150: FIFO output control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_OUT_CTRL));
	printk("tvp5150: Full field enable = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FULL_FIELD_ENA));
	printk("tvp5150: Full field mode register = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FULL_FIELD_MODE_REG));

	dump_reg_range(sd, "CC   data",   TVP5150_CC_DATA_INI,
			TVP5150_CC_DATA_END, 8);

	dump_reg_range(sd, "WSS  data",   TVP5150_WSS_DATA_INI,
			TVP5150_WSS_DATA_END, 8);

	dump_reg_range(sd, "VPS  data",   TVP5150_VPS_DATA_INI,
			TVP5150_VPS_DATA_END, 8);

	dump_reg_range(sd, "VITC data",   TVP5150_VITC_DATA_INI,
			TVP5150_VITC_DATA_END, 10);

	dump_reg_range(sd, "Line mode",   TVP5150_LINE_MODE_INI,
			TVP5150_LINE_MODE_END, 8);
	return 0;
}

static void tvp5150_patch(struct v4l2_subdev *sd) {
	int i;

	/* Reset, enable */
	tvp5150_write(sd, TVP5150_PATCH_EXEC, 0x00);
	tvp5150_write(sd, TVP5150_MISC_CTL, 0x69);

	/* Unlock */
	tvp5150_write(sd, TVP5150_IDR_DATA_LSB, 0x51);
	tvp5150_write(sd, TVP5150_IDR_DATA_MSB, 0x50);
	tvp5150_write(sd, TVP5150_IDR_ADDR, 0xFF);
	tvp5150_write(sd, TVP5150_IDR_STROBE, 0x04);

	/* Write Patch */
	v4l2_dbg(2, debug, sd, "tvp5150: patching %d bytes from %p\n", tvp5150am1_patch_size, tvp5150am1_patch);
	for (i = 0; i < tvp5150am1_patch_size; i++)
		tvp5150_write(sd, TVP5150_PATCH_WRITE, tvp5150am1_patch[i]);

	/* Restart */
	tvp5150_write(sd, TVP5150_PATCH_EXEC, 0x00); 

	/* Lock */
	tvp5150_write(sd, TVP5150_IDR_DATA_LSB, 0x00);
	tvp5150_write(sd, TVP5150_IDR_DATA_MSB, 0x00);
	tvp5150_write(sd, TVP5150_IDR_ADDR, 0xFF);
	tvp5150_write(sd, TVP5150_IDR_STROBE, 0x04);
}

static void detect_format(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
	u16 lines;

	lines = tvp5150_read(sd, TVP5150_VERT_LN_COUNT_MSB);
	lines <<= 8;
	lines |= tvp5150_read(sd, TVP5150_VERT_LN_COUNT_LSB);

	v4l2_info(sd, "tvp5150: lines %d\n", lines);

	mf->width = 720; /* always */
	mf->height = lines == 525 ? 480 : 576;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field = V4L2_FIELD_ALTERNATE;
	mf->code = V4L2_MBUS_FMT_UYVY8_2X8;
}

static int tvp5150_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct v4l2_mbus_framefmt *mf = v4l2_subdev_get_try_format(fh, 0);

	printk(KERN_ERR "%s\n", __func__);

	detect_format(sd, mf);

	return 0;
}

static const struct v4l2_subdev_internal_ops tvp5150_subdev_internal_ops = {
	.open = tvp5150_open,
};

static int tvp5150_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_fh *fh,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	printk(KERN_ERR "%s\n", __func__);

	if (code->index > 0)
		return -EINVAL;

	code->code = V4L2_MBUS_FMT_UYVY8_2X8;

	return 0;
}

static int tvp5150_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			   struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *mf;

	printk(KERN_ERR "%s\n", __func__);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		if (fh) {
			mf = v4l2_subdev_get_try_format(fh, 0);
			fmt->format = *mf;
		}
		return 0;
	}
	mf = &fmt->format;

	detect_format(sd, mf);

	return 0;
}

static int tvp5150_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			   struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *mf;

	printk(KERN_ERR "%s\n", __func__);

	fmt->format.colorspace = V4L2_COLORSPACE_JPEG;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		if (fh) {
			mf = v4l2_subdev_get_try_format(fh, 0);
			*mf = fmt->format;
		}
		return 0;
	}
	mf = &fmt->format;

	detect_format(sd, mf);

	return 0;
}

static struct v4l2_subdev_pad_ops tvp5150_pad_ops = {
	.enum_mbus_code	= tvp5150_enum_mbus_code,
	.get_fmt	= tvp5150_get_fmt,
	.set_fmt	= tvp5150_set_fmt,
};

static int tvp5150_s_stream(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static struct v4l2_subdev_video_ops tvp5150_video_ops = {
	.s_stream	= tvp5150_s_stream,
};

static const struct v4l2_subdev_ops tvp5150_ops = {
	.pad	= &tvp5150_pad_ops,
	.video	= &tvp5150_video_ops,
};

static int tvp5150_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct tvp5150 *core;
	struct v4l2_subdev *sd;
	u8 msb_id, lsb_id, msb_rom, lsb_rom;
	int ret;

	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter,
	     I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		return -EIO;

	core = kzalloc(sizeof(struct tvp5150), GFP_KERNEL);
	if (!core) {
		return -ENOMEM;
	}

	sd = &core->sd;
	v4l2_i2c_subdev_init(sd, client, &tvp5150_ops);

	msb_id = tvp5150_read(sd, TVP5150_MSB_DEV_ID);
	lsb_id = tvp5150_read(sd, TVP5150_LSB_DEV_ID);
	msb_rom = tvp5150_read(sd, TVP5150_ROM_MAJOR_VER);
	lsb_rom = tvp5150_read(sd, TVP5150_ROM_MINOR_VER);

	if (msb_id == 0x51 && lsb_id == 0x50) {
		/* Is TVP5150AM1 */
		tvp5150_patch(sd);

		/* Re-read */
		msb_id = tvp5150_read(sd, TVP5150_MSB_DEV_ID);
		lsb_id = tvp5150_read(sd, TVP5150_LSB_DEV_ID);
		msb_rom = tvp5150_read(sd, TVP5150_ROM_MAJOR_VER);
		lsb_rom = tvp5150_read(sd, TVP5150_ROM_MINOR_VER);

		v4l2_info(sd, "tvp%02x%02xam1 detected - rom %02x.%02x\n", msb_id, lsb_id, msb_rom, lsb_rom);

		/* ITU-T BT.656.4 timing */
		tvp5150_write(sd, TVP5150_REV_SELECT, 0);

	} else {
		v4l2_err(sd, "tvp5150am1: Unknown device - %02x%02x rom %02x.%02x\n", msb_id, lsb_id, msb_rom, lsb_rom);
		return -ENODEV;
	}

	tvp5150_log_status(sd);

	strlcpy(sd->name, MODULE_NAME, sizeof(sd->name));

	sd->internal_ops = &tvp5150_subdev_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	core->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&sd->entity, 1, &core->pad, 0);
	if (ret < 0)
		goto err;

	return 0;

err:
	v4l2_device_unregister_subdev(sd);
	kfree(core);
	return ret;
}

static int tvp5150_remove(struct i2c_client *c)
{
	return 0;
}

static const struct i2c_device_id tvp5150_id[] = {
	{ "tvp5150", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tvp5150_id);

static struct i2c_driver tvp5150_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= MODULE_NAME,
	},
	.probe		= tvp5150_probe,
	.remove		= tvp5150_remove,
	.id_table	= tvp5150_id,
};

module_i2c_driver(tvp5150_driver);
