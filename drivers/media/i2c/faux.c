/*
 * Fake sensor driver
 *
 * Copyright (C) 2012 MDSoft Ltd
 * Contact: Mike Dyer <mike.dyer@md-soft.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/faux.h>

#define MODULE_NAME "faux-sensor"

struct faux_info {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct faux_video_platform_data data;
};

static inline struct faux_info *to_faux(struct v4l2_subdev *sd)
{
	return container_of(sd, struct faux_info, sd);
}

static int faux_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct faux_info *info = to_faux(sd);
	struct v4l2_mbus_framefmt *mf = v4l2_subdev_get_try_format(fh, 0);

	//printk(KERN_ERR "%s\n", __func__);

	mf->width = info->data.width;
	mf->height = info->data.height;
	mf->code = info->data.code;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field = info->data.field;
	return 0;
}

static const struct v4l2_subdev_internal_ops faux_subdev_internal_ops = {
	.open = faux_open,
};

static int faux_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_fh *fh,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	struct faux_info *info = to_faux(sd);

	//printk(KERN_ERR "%s\n", __func__);

	if (code->index > 0)
		return -EINVAL;

	code->code = info->data.code;

	return 0;
}

static int faux_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			   struct v4l2_subdev_format *fmt)
{
	struct faux_info *info = to_faux(sd);
	struct v4l2_mbus_framefmt *mf;

	//printk(KERN_ERR "%s\n", __func__);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		if (fh) {
			mf = v4l2_subdev_get_try_format(fh, 0);
			fmt->format = *mf;
		}
		return 0;
	}
	mf = &fmt->format;

	mf->width = info->data.width;
	mf->height = info->data.height;
	mf->code = info->data.code;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field = info->data.field;

	return 0;
}

static int faux_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			   struct v4l2_subdev_format *fmt)
{
	struct faux_info *info = to_faux(sd);
	struct v4l2_mbus_framefmt *mf;
	int ret = 0;

	//printk(KERN_ERR "%s\n", __func__);

	fmt->format.colorspace = V4L2_COLORSPACE_JPEG;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		if (fh) {
			mf = v4l2_subdev_get_try_format(fh, 0);
			*mf = fmt->format;
		}
		return 0;
	}
	mf = &fmt->format;

	mf->width = info->data.width;
	mf->height = info->data.height;
	mf->code = info->data.code;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field = info->data.field;

	return ret;
}

static struct v4l2_subdev_pad_ops faux_pad_ops = {
	.enum_mbus_code	= faux_enum_mbus_code,
	.get_fmt	= faux_get_fmt,
	.set_fmt	= faux_set_fmt,
};

static int faux_s_stream(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static struct v4l2_subdev_video_ops faux_video_ops = {
	.s_stream	= faux_s_stream,
};

static const struct v4l2_subdev_ops faux_ops = {
	.pad	= &faux_pad_ops,
	.video	= &faux_video_ops,
};

static int faux_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct faux_info *info;
	int ret;
	const struct faux_video_platform_data *pdata
		= client->dev.platform_data;

	//printk(KERN_ERR "%s\n", __func__);

	if (!pdata) {
		dev_err(&client->dev, "No platform data!\n");
		return -EIO;
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memcpy(&info->data, pdata, sizeof(info->data));

	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &faux_ops);

	strlcpy(sd->name, MODULE_NAME, sizeof(sd->name));

	sd->internal_ops = &faux_subdev_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	info->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&sd->entity, 1, &info->pad, 0);
	if (ret < 0)
		goto err;

	return 0;

err:
	v4l2_device_unregister_subdev(sd);
	kfree(info);
	return ret;
}

static int faux_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct faux_info *info = to_faux(sd);

	//printk(KERN_ERR "%s\n", __func__);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	kfree(info);

	return 0;
}

static const struct i2c_device_id faux_id[] = {
	{ MODULE_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, faux_id);

static struct i2c_driver faux_i2c_driver = {
	.driver = {
		.name = MODULE_NAME
	},
	.probe		= faux_probe,
	.remove		= faux_remove,
	.id_table	= faux_id,
};

module_i2c_driver(faux_i2c_driver);

MODULE_DESCRIPTION("Faux sensor");
MODULE_AUTHOR("Mike Dyer <mike.dyer@md-soft.co.uk>");
MODULE_LICENSE("GPL");
