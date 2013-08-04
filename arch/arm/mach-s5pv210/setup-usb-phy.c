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

#include <plat/cpu.h>
#include <plat/regs-usb-hsotg-phy.h>
#include <plat/usb-phy.h>

#define S5PV210_USB_PHY_CON	(S3C_VA_SYS + 0xE80C)
#define S5PV210_USB_PHY0_EN	(1 << 0)
#define S5PV210_USB_PHY1_EN	(1 << 1)

static int s5pv210_usb_host_phy_is_on(void)
{
	return (readl(S3C_PHYPWR) & S5P_PHYPWR_ANALOG_POWERDOWN_1) ? 0 : 1;
}

static void s5pv210_usb_phy_clkset(struct platform_device *pdev)
{
	struct clk *xusbxti_clk;
	u32 phyclk;

	/* set clock frequency for PLL */
	phyclk = readl(S3C_PHYCLK) & ~S3C_PHYCLK_CLKSEL_MASK;

	xusbxti_clk = clk_get(&pdev->dev, "xusbxti");
	if (xusbxti_clk && !IS_ERR(xusbxti_clk)) {
		switch (clk_get_rate(xusbxti_clk)) {
		case 12 * MHZ:
			phyclk |= S3C_PHYCLK_CLKSEL_12M;
			break;
		case 24 * MHZ:
			phyclk |= S3C_PHYCLK_CLKSEL_24M;
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

static int s5pv210_usb_otgphy_init(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "usb-otg phy init\n");

	writel(readl(S5PV210_USB_PHY_CON) | S5PV210_USB_PHY0_EN,
			S5PV210_USB_PHY_CON);

	s5pv210_usb_phy_clkset(pdev);

	/* Force clock on */
	writel(readl(S3C_PHYCLK) | S3C_PHYCLK_CLK_FORCE, S3C_PHYCLK);

	/* set to normal OTG PHY */
	writel(readl(S3C_PHYPWR) & ~S5P_PHYPWR_NORMAL_MASK_0, S3C_PHYPWR);
	mdelay(1);

	/* reset OTG PHY and Link */
	writel(readl(S3C_RSTCON) | S5P_RSTCON_MSK_0, S3C_RSTCON);

	udelay(20);	/* at-least 10uS */
	writel(readl(S3C_RSTCON) & ~S5P_RSTCON_MSK_0, S3C_RSTCON);

	return 0;
}

static int s5pv210_usb_otgphy_exit(struct platform_device *pdev)
{
	writel((readl(S3C_PHYPWR) | S3C_PHYPWR_ANALOG_POWERDOWN |
				S3C_PHYPWR_OTG_DISABLE), S3C_PHYPWR);

	writel(readl(S5PV210_USB_PHY_CON) & ~S5PV210_USB_PHY0_EN,
			S5PV210_USB_PHY_CON);

	return 0;
}

static int s5pv210_usb_host_init(struct platform_device *pdev)
{
	struct clk *otg_clk;
	int err;

	dev_info(&pdev->dev, "usb-host phy init\n");

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

	/* Force clock on */
	writel(readl(S3C_PHYCLK) | S5P_PHYCLK_CLK_FORCE_1, S3C_PHYCLK);

	/* set to normal standard USB of PHY1 */
	writel((readl(S3C_PHYPWR) & ~S5P_PHYPWR_NORMAL_MASK_1), S3C_PHYPWR);

	/* reset phy */
	writel(readl(S3C_RSTCON) | S5P_RSTCON_MSK_1, S3C_RSTCON);
	udelay(20);
	writel(readl(S3C_RSTCON) & ~S5P_RSTCON_MSK_1, S3C_RSTCON);

	udelay(80);

	clk_disable(otg_clk);
	clk_put(otg_clk);

	return 0;
}

static int s5pv210_usb_host_exit(struct platform_device *pdev)
{
	struct clk *otg_clk;
	int err;


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

	writel((readl(S3C_PHYPWR) | S5P_PHYPWR_ANALOG_POWERDOWN_1), S3C_PHYPWR);

	writel(readl(S5PV210_USB_PHY_CON) & ~S5PV210_USB_PHY1_EN,
			S5PV210_USB_PHY_CON);

	clk_disable(otg_clk);
	clk_put(otg_clk);

	return 0;
}

int s5p_usb_phy_init(struct platform_device *pdev, int type)
{
	if (type == USB_PHY_TYPE_DEVICE)
		return s5pv210_usb_otgphy_init(pdev);
	else if (type == USB_PHY_TYPE_HOST)
		return s5pv210_usb_host_init(pdev);

	return -EINVAL;
}

int s5p_usb_phy_exit(struct platform_device *pdev, int type)
{
	if (type == USB_PHY_TYPE_DEVICE)
		return s5pv210_usb_otgphy_exit(pdev);
	else if (type == USB_PHY_TYPE_HOST)
		return s5pv210_usb_host_exit(pdev);

	return -EINVAL;
}
