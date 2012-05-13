/*
 * Copyright (C) 2012 MDSoft Ltd
 * Author: Mike Dyer <mike.dyer@md-soft.co.uk>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <mach/map.h>
#include <mach/regs-sys.h>
#include <mach/regs-clock.h>
#include <mach/regs-usb-phy.h>
#include <plat/cpu.h>

static atomic_t phy[2];

static int s5p_usb_host_phy_is_on(int n)
{
	if (n == 1)
		return (readl(S5P_PHYPWR) & PHY1_STD_ANALOG_POWERDOWN) ? 0 : 1;
	else
		return (readl(S5P_PHYPWR) & PHY0_ANALOG_POWERDOWN) ? 0 : 1;
}

void set_phy_clk(struct platform_device *pdev) {
	struct clk *xusbxti_clk;
	u32 phyclk;

	/* set clock frequency for PLL */
	phyclk = readl(S5P_PHYCLK) & ~CLKSEL_MASK;

	xusbxti_clk = clk_get(&pdev->dev, "xusbxti");
	if (xusbxti_clk && !IS_ERR(xusbxti_clk)) {
		printk(KERN_ERR "USB Clock %luHZ\n", clk_get_rate(xusbxti_clk));
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
	

	writel(phyclk, S5P_PHYCLK);
}

static void enable_phy1(void)
{
	u32 rstcon;

	printk(KERN_ERR "USB Enable Phy[1]\n");

	/* set to normal standard USB of PHY1 */
	writel((readl(S5P_PHYPWR) & ~PHY1_STD_NORMAL_MASK), S5P_PHYPWR);

	/* reset all ports of both PHY and Link */
	rstcon = readl(S5P_RSTCON) | PHY1_SWRST_MASK;
	writel(rstcon, S5P_RSTCON);
	udelay(10);

	rstcon &= ~(PHY1_SWRST_MASK);
	writel(rstcon, S5P_RSTCON);
	udelay(80);
}

static void enable_phy0(void)
{
	u32 rstcon;

	printk(KERN_ERR "USB Enable Phy[0]\n");

	/* set to normal standard USB of PHY0 */
	writel((readl(S5P_PHYPWR) & ~PHY0_NORMAL_MASK), S5P_PHYPWR);

	/* reset all ports of both PHY and Link */
	rstcon = readl(S5P_RSTCON) | PHY0_SWRST_MASK;
	writel(rstcon, S5P_RSTCON);
	udelay(10);

	rstcon &= ~(PHY0_SWRST_MASK);
	writel(rstcon, S5P_RSTCON);
	udelay(80);
}

int s5p_usb_phy_init(struct platform_device *pdev, int type)
{
	struct clk *otg_clk;
	int err;
	int n = 0;

	if (strcmp(pdev->name, "s5p-ehci") == 0)
		n = 1;

	atomic_inc(&phy[n]);

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

	if (s5p_usb_host_phy_is_on(n))
		return 0;

	writel(readl(S5P_USB_PHY_CONTROL) | (1 << n),
			S5P_USB_PHY_CONTROL);

	set_phy_clk(pdev);

	if (n == 0)
		enable_phy0(); 
	else
		enable_phy1();

	clk_disable(otg_clk);
	clk_put(otg_clk);

	return 0;
}

static void disable_phy0(void)
{
	writel((readl(S5P_PHYPWR) | PHY0_ANALOG_POWERDOWN),
			S5P_PHYPWR);
}

static void disable_phy1(void)
{
	writel((readl(S5P_PHYPWR) | PHY1_STD_ANALOG_POWERDOWN),
			S5P_PHYPWR);
}

int s5p_usb_phy_exit(struct platform_device *pdev, int type)
{
	struct clk *otg_clk;
	int err;
	int n = 1;

	if (strcmp(pdev->name, "s5p-ehci") == 0)
		n = 0;

	if (atomic_dec_return(&phy[n]) > 0)
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

	if (n == 0)
		disable_phy0();
	else
		disable_phy1();

	writel(readl(S5P_USB_PHY_CONTROL) & ~(1 << n),
			S5P_USB_PHY_CONTROL);

	return 0;
}
