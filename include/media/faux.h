/*
 * Driver header for faux camera sensor chip.
 *
 * Copyright (c) 2012 MDSoft Ltd
 * Contact: Mike Dyer <mike.dyer@md-soft.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef FAUXVIDEO_H
#define FAUXVIDEO_H

#include <media/v4l2-mediabus.h>

/**
 * @width: image width of fake sensor
 * @height: image height of fake sensor
 * @interlaced: image is interlaced
 * @code: color pixel code
 */

struct faux_video_platform_data {
	u32 width;
	u32 height;
	enum v4l2_field field;
	enum v4l2_mbus_pixelcode code;
};

#endif /* FAUXVIDEO_H */
