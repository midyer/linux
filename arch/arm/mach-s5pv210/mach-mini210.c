/* linux/arch/arm/mach-s5pv210/mach-mini210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/sysdev.h>
#include <linux/dm9000.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pwm_backlight.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <video/platform_lcd.h>

#include <mach/map.h>
#include <mach/regs-clock.h>

#include <plat/regs-serial.h>
#include <plat/regs-srom.h>
#include <plat/gpio-cfg.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/ata.h>
#include <plat/iic.h>
#include <plat/keypad.h>
#include <plat/pm.h>
#include <plat/fb.h>
#include <plat/s5p-time.h>
#include <plat/backlight.h>
#include <plat/regs-fb-v4.h>
#include <plat/nand.h>
#include <plat/sdhci.h>

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define MINI210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define MINI210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define MINI210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg mini210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= MINI210_UCON_DEFAULT,
		.ulcon		= MINI210_ULCON_DEFAULT,
		.ufcon		= MINI210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= MINI210_UCON_DEFAULT,
		.ulcon		= MINI210_ULCON_DEFAULT,
		.ufcon		= MINI210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= MINI210_UCON_DEFAULT,
		.ulcon		= MINI210_ULCON_DEFAULT,
		.ufcon		= MINI210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= MINI210_UCON_DEFAULT,
		.ulcon		= MINI210_ULCON_DEFAULT,
		.ufcon		= MINI210_UFCON_DEFAULT,
	},
};

static struct mtd_partition mini210_nand_part[] = {
	[0] = {
		.name	= "uboot",
		.size	= SZ_128K + SZ_256K,
		.offset	= 0,
	},
	[1] = {
		.name	= "uboot-env",
		.size	= SZ_128K,
		.offset	= SZ_128K + SZ_256K,
	},
	[1] = {
		.name	= "kernel",
		.size	= SZ_512K,
		.offset	= SZ_4M,
	},
	[2] = {
		.name	= "rootfs",
		.size	= MTDPART_SIZ_FULL,
		.offset	= SZ_4M + SZ_512K,
	},
};

static struct s3c2410_nand_set mini210_nand_sets[] = {
	[0] = {
		.name		= "nand",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(mini210_nand_part),
		.partitions	= mini210_nand_part,
	},
};

static struct s3c2410_platform_nand mini210_nand_info = {
	.tacls		= 25,
	.twrph0		= 55,
	.twrph1		= 40,
	.nr_sets	= ARRAY_SIZE(mini210_nand_sets),
	.sets		= mini210_nand_sets,
};

static struct platform_device *mini210_devices[] __initdata = {
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1
};

static void __init mini210_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(mini210_uartcfgs, ARRAY_SIZE(mini210_uartcfgs));
	s5p_set_timer_source(S5P_PWM3, S5P_PWM4);
}


static void __init mini210_machine_init(void)
{
	s3c_pm_init();

	/*s5pv210_default_sdhci0();
	s5pv210_default_sdhci1();*/

	platform_add_devices(mini210_devices, ARRAY_SIZE(mini210_devices));
}

MACHINE_START(MINI210, "MINI110")
	/* Maintainer: Mike Dyer <mike.dyer@md-soft.co.uk> */
	.atag_offset	= 0x100,
	.init_irq	= s5pv210_init_irq,
	.map_io		= mini210_map_io,
	.init_machine	= mini210_machine_init,
	.timer		= &s5p_timer,
MACHINE_END
