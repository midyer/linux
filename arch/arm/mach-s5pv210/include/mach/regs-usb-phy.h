/*
 * Copyright (C) 2012 MDSoft Ltd.
 * Author: Mike Dyer <mike.dyer@md-soft.co.uk>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __PLAT_S5PV210_REGS_USB_PHY_H
#define __PLAT_S5PV210_REGS_USB_PHY_H

#define S5P_USB_PHYREG(x)		((x) + S3C_VA_USB_HSPHY)

#define S5P_PHYPWR			S5P_USB_PHYREG(0x00)
#define PHY1_STD_NORMAL_MASK		(0x3 << 6)
#define PHY1_STD_ANALOG_POWERDOWN	(1 << 7)
#define PHY1_STD_FORCE_SUSPEND		(1 << 6)

#define PHY0_NORMAL_MASK		(0x19 << 0)
#define PHY0_OTG_DISABLE		(1 << 4)
#define PHY0_ANALOG_POWERDOWN		(1 << 3)
#define PHY0_FORCE_SUSPEND		(1 << 0)

#define S5P_PHYCLK			S5P_USB_PHYREG(0x04)
#define PHY1_COMMON_ON_N		(1 << 7)
#define PHY0_COMMON_ON_N		(1 << 4)
#define PHY0_ID_PULLUP			(1 << 2)
#define CLKSEL_MASK			(0x3 << 0)
#define CLKSEL_SHIFT			(0)
#define CLKSEL_48M			(0x0 << 0)
#define CLKSEL_12M			(0x2 << 0)
#define CLKSEL_24M			(0x3 << 0)

#define S5P_RSTCON			S5P_USB_PHYREG(0x08)
#define PHY1_SWRST_MASK			(0x3 << 3)
#define PHY1_STD_SWRST			(1 << 4)
#define PHY1_ALL_SWRST			(1 << 3)

#define PHY0_SWRST_MASK			(0x7 << 0)
#define PHY0_PHYLINK_SWRST		(1 << 2)
#define PHY0_HLINK_SWRST		(1 << 1)
#define PHY0_SWRST			(1 << 0)


#endif

