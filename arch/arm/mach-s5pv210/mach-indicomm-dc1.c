/* linux/arch/arm/mach-s5pv210/mach-idc1sdk.c
 *
 * Copyright (c) 2012 MDSoft LTd
 * Mike Dyer <mike.dyer@md-soft.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/dm9000.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/hardware/vic.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/map.h>
#include <mach/regs-clock.h>

#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/ehci.h>
#include <plat/fb.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>
#include <plat/mfc.h>
#include <plat/nand.h>
#include <plat/pm.h>
#include <plat/s5p-time.h>

#include <plat/regs-fb-v4.h>
#include <plat/regs-serial.h>
#include <plat/regs-srom.h>

#include <media/v4l2-mediabus.h>
#include <media/s5p_fimc.h>
#include <plat/camport.h>

#include "common.h"

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define IDC1_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define IDC1_ULCON_DEFAULT	S3C2410_LCON_CS8

#define IDC1_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg idc1_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= IDC1_UCON_DEFAULT,
		.ulcon		= IDC1_ULCON_DEFAULT,
		.ufcon		= IDC1_UFCON_DEFAULT,
	} ,
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= IDC1_UCON_DEFAULT,
		.ulcon		= IDC1_ULCON_DEFAULT,
		.ufcon		= IDC1_UFCON_DEFAULT,
	},
/*
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= IDC1_UCON_DEFAULT,
		.ulcon		= IDC1_ULCON_DEFAULT,
		.ufcon		= IDC1_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= IDC1_UCON_DEFAULT,
		.ulcon		= IDC1_ULCON_DEFAULT,
		.ufcon		= IDC1_UFCON_DEFAULT,
	},
	*/
};


//__setup("ethmac=", dm9000_set_mac);

static struct gpio_led gpio_leds[] = {
	{
		.name			= "led1",
		.gpio			= S5PV210_GPJ2(0),
		.active_low		= 1,
		.default_trigger	= "heartbeat",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},{
		.name			= "led2",
		.gpio			= S5PV210_GPJ2(1),
		.active_low		= 1,
		.default_trigger	= "nand-disk",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},{
		.name			= "led3",
		.gpio			= S5PV210_GPJ2(2),
		.active_low		= 1,
		.default_trigger	= "none",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},{
		.name			= "led4",
		.gpio			= S5PV210_GPJ2(3),
		.active_low		= 1,
		.default_trigger	= "none",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device idc1_leds = {
	.name	= "leds-gpio",
	.id	= 0,
	.dev	= {
		.platform_data	= &gpio_led_info,
	}
};

static struct mtd_partition idc1_nand_part[] = {
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
	[2] = {
		.name	= "kernel",
		.size	= SZ_1M + SZ_2M,
		.offset	= SZ_128K + SZ_128K + SZ_256K,
	},
	[3] = {
		.name	= "rootfs",
		.size	= MTDPART_SIZ_FULL,
		.offset	= SZ_1M + SZ_2M + SZ_128K + SZ_128K + SZ_256K,
	},
};

static struct s3c2410_nand_set idc1_nand_sets[] = {
	[0] = {
		.name		= "nand",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(idc1_nand_part),
		.partitions	= idc1_nand_part,
	},
};

static struct s3c2410_platform_nand idc1_nand_info = {
	.tacls		= 25,
	.twrph0		= 55,
	.twrph1		= 40,
	.nr_sets	= ARRAY_SIZE(idc1_nand_sets),
	.sets		= idc1_nand_sets,
};


static void __init idc1_map_io(void)
{
	s5pv210_init_io(NULL, 0);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(idc1_uartcfgs, ARRAY_SIZE(idc1_uartcfgs));
	s5p_set_timer_source(S5P_PWM3, S5P_PWM4);
}

static void __init idc1_reserve(void)
{
	s5p_mfc_reserve_mem(0x3EC00000, 8 << 20, 0x3F400000, 8 << 20);
}

};

};

	},
};

};

static struct s5p_ehci_platdata idc1_ehci_pdata;

static struct platform_device *idc1_devices[] __initdata = {
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc_md,
	&s3c_device_i2c0,
	&s5p_device_mfc,
	&s5p_device_mfc_l,
	&s5p_device_mfc_r,
	&s5p_device_jpeg,
	&s3c_device_rtc,
	&s3c_device_wdt,
	&s3c_device_nand,
	&s5p_device_ehci,
	&idc1_leds,
};

static void __init idc1_machine_init(void)
{
	s3c_pm_init();

	s3c_nand_set_platdata(&idc1_nand_info);

	s5p_ehci_set_platdata(&idc1_ehci_pdata);

	platform_add_devices(idc1_devices, ARRAY_SIZE(idc1_devices));
}

MACHINE_START(IDCM_DC1, "IDC1 SDK")
	/* Maintainer: Mike Dyer <mike.dyer@md-soft.co.uk> */
	.atag_offset	= 0x100,
	.init_irq	= s5pv210_init_irq,
	.handle_irq	= vic_handle_irq,
	.map_io		= idc1_map_io,
	.init_machine	= idc1_machine_init,
	.timer		= &s5p_timer,
	.restart	= s5pv210_restart,
	.reserve	= idc1_reserve
MACHINE_END
