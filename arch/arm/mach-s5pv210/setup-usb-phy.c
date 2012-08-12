/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundationr
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/map.h>
#include <mach/regs-sys.h>
#include <mach/regs-usb-phy.h>
#include <plat/cpu.h>
#include <plat/regs-usb-hsotg-phy.h>
#include <plat/usb-phy.h>

static atomic_t host_usage;

static int s5pv210_usb_host_phy_is_on(void)
{
	return (readl(S5PV210_PHYPWR) & PHY1_STD_ANALOG_POWERDOWN) ? 0 : 1;
}

static void s5pv210_usb_phy_clkset(struct platform_device *pdev)
{
	struct clk *xusbxti_clk;
	u32 phyclk;

	/* set clock frequency for PLL */
	phyclk = readl(S3C_PHYCLK) & ~CLKSEL_MASK;

	xusbxti_clk = clk_get(&pdev->dev, "xusbxti");
	if (xusbxti_clk && !IS_ERR(xusbxti_clk)) {
		switch (clk_get_rate(xusbxti_clk)) {
		case 12 * MHZ:
			phyclk |= CLKSEL_12M;
			break;
		case 24 * MHZ:
			phyclk |= CLKSEL_24M;
			break;
		default:
		case 48 * MHZ:
			/* default reference clock */
			break;
		}
		clk_put(xusbxti_clk);
	}

	writel(phyclk, S3C_PHYCLK);
}

static int s5pv210_usb_phy0_init(struct platform_device *pdev)
{
	u32 rstcon;

	dev_info(&pdev->dev, "phy[0] init\n");

	writel(readl(S5PV210_USB_PHY_CON) | S5PV210_USB_PHY0_EN,
			S5PV210_USB_PHY_CON);

	s5pv210_usb_phy_clkset(pdev);

	/* set to normal PHY0 */
	writel((readl(S5PV210_PHYPWR) & ~PHY0_NORMAL_MASK), S5PV210_PHYPWR);

	/* reset PHY0 and Link */
	rstcon = readl(S5PV210_PHYPWR) | PHY0_SWRST_MASK;
	writel(rstcon, S5PV210_PHYPWR);
	udelay(10);

	rstcon &= ~PHY0_SWRST_MASK;
	writel(rstcon, S5PV210_PHYPWR);

	return 0;
}

static int s5pv210_usb_phy0_exit(struct platform_device *pdev)
{
	writel((readl(S5PV210_PHYPWR) | PHY0_ANALOG_POWERDOWN |
				PHY0_OTG_DISABLE), S5PV210_PHYPWR);

	writel(readl(S5PV210_USB_PHY_CON) & ~S5PV210_USB_PHY0_EN,
			S5PV210_USB_PHY_CON);

	return 0;
}

static int s5pv210_usb_phy1_init(struct platform_device *pdev)
{
	struct clk *otg_clk;
	u32 rstcon;
	int err;

	dev_info(&pdev->dev, "phy[1] init\n");

	atomic_inc(&host_usage);

	otg_clk = clk_get(&pdev->dev, "otg");
	if (IS_ERR(otg_clk)) {
		dev_err(&pdev->dev, "Failed to get otg clock\n");
		return PTR_ERR(otg_clk);
	}

	err = clk_enable(otg_clk);
	if (err) {
		clk_put(otg_clk);
		return err;
	}

	if (s5pv210_usb_host_phy_is_on())
		return 0;

	writel(readl(S5PV210_USB_PHY_CON) | S5PV210_USB_PHY1_EN,
			S5PV210_USB_PHY_CON);

	s5pv210_usb_phy_clkset(pdev);

	/* set to normal standard USB of PHY1 */
	writel((readl(S5PV210_PHYPWR) & ~PHY1_STD_NORMAL_MASK), S5PV210_PHYPWR);

	/* reset all ports of both PHY and Link */
	rstcon = readl(S5PV210_RSTCON) | PHY1_SWRST_MASK;
	writel(rstcon, S5PV210_RSTCON);
	udelay(10);

	rstcon &= ~PHY1_SWRST_MASK;
	writel(rstcon, S5PV210_RSTCON);
	udelay(80);

	clk_disable(otg_clk);
	clk_put(otg_clk);

	return 0;
}

static int s5pv210_usb_phy1_exit(struct platform_device *pdev)
{
	struct clk *otg_clk;
	int err;

	if (atomic_dec_return(&host_usage) > 0)
		return 0;

	otg_clk = clk_get(&pdev->dev, "otg");
	if (IS_ERR(otg_clk)) {
		dev_err(&pdev->dev, "Failed to get otg clock\n");
		return PTR_ERR(otg_clk);
	}

	err = clk_enable(otg_clk);
	if (err) {
		clk_put(otg_clk);
		return err;
	}

	writel((readl(S5PV210_PHYPWR) | PHY1_STD_ANALOG_POWERDOWN),
			S5PV210_PHYPWR);

	writel(readl(S5PV210_USB_PHY_CON) & ~S5PV210_USB_PHY1_EN,
			S5PV210_USB_PHY_CON);

	clk_disable(otg_clk);
	clk_put(otg_clk);

	return 0;
}

int s5p_usb_phy_init(struct platform_device *pdev, int type)
{
	if (type == S5P_USB_PHY_DEVICE)
		return s5pv210_usb_phy0_init(pdev);
	else if (type == S5P_USB_PHY_HOST)
		return s5pv210_usb_phy1_init(pdev);
	return -EINVAL;
}

int s5p_usb_phy_exit(struct platform_device *pdev, int type)
{
	if (type == S5P_USB_PHY_DEVICE)
		return s5pv210_usb_phy0_exit(pdev);
	else if (type == S5P_USB_PHY_HOST)
		return s5pv210_usb_phy1_exit(pdev);

	return -EINVAL;
}
