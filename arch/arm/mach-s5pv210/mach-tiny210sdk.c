/* linux/arch/arm/mach-s5pv210/mach-tiny210sdk.c
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
#include <sound/wm8960.h>

#include "common.h"

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define TINY210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define TINY210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define TINY210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg tiny210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= TINY210_UCON_DEFAULT,
		.ulcon		= TINY210_ULCON_DEFAULT,
		.ufcon		= TINY210_UFCON_DEFAULT,
	} ,
	/*
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= TINY210_UCON_DEFAULT,
		.ulcon		= TINY210_ULCON_DEFAULT,
		.ufcon		= TINY210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= TINY210_UCON_DEFAULT,
		.ulcon		= TINY210_ULCON_DEFAULT,
		.ufcon		= TINY210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= TINY210_UCON_DEFAULT,
		.ulcon		= TINY210_ULCON_DEFAULT,
		.ufcon		= TINY210_UFCON_DEFAULT,
	},
	*/
};

#define S5PV210_PA_DM9000_A     (0x88001000)
#define S5PV210_PA_DM9000_F     (S5PV210_PA_DM9000_A + 0x300C)

static struct resource dm9000_resources[] = {
	[0] = {
		.start	= S5PV210_PA_DM9000_A,
		.end	= S5PV210_PA_DM9000_A + SZ_1K * 4 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= S5PV210_PA_DM9000_F,
		.end	= S5PV210_PA_DM9000_F + SZ_1K * 4 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= IRQ_EINT(7),
		.end	= IRQ_EINT(7),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct dm9000_plat_data dm9000_platdata = {
	.flags		= DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM,
	.dev_addr	= { 0x08, 0x90, 0x00, 0xa0, 0x02, 0x10 },
};

struct platform_device tiny210_device_dm9000 = {
	.name		= "dm9000",
	.id			= -1,
	.num_resources	= ARRAY_SIZE(dm9000_resources),
	.resource	= dm9000_resources,
	.dev		= {
		.platform_data	= &dm9000_platdata,
	},
};

static int __init dm9000_set_mac(char *str) {

	unsigned int val;
	int idx = 0;
	char *p = str, *end;

	while (*p && idx < 6) {
		val = simple_strtoul(p, &end, 16);
		if (end <= p) {
			/* convert failed */
			break;
		} else {
			dm9000_platdata.dev_addr[idx++] = val;
			p = end;
			if (*p == ':'|| *p == '-') {
				p++;
			} else {
				break;
			}
		}
	}

	return 1;
}

__setup("ethmac=", dm9000_set_mac);

static void __init tiny210_dm9000_set(void)
{
	unsigned int tmp;

	tmp = ((0<<28)|(0<<24)|(5<<16)|(0<<12)|(0<<8)|(0<<4)|(0<<0));
	__raw_writel(tmp, (S5P_SROM_BW+0x08));

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~(0xf << 4);
	tmp |= (0x1 << 4); /* dm9000 16bit */
	__raw_writel(tmp, S5P_SROM_BW);

	gpio_request(S5PV210_MP01(1), "nCS1");
	s3c_gpio_cfgpin(S5PV210_MP01(1), S3C_GPIO_SFN(2));
	gpio_free(S5PV210_MP01(1));
}

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
		.default_trigger	= "none",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},{
		.name			= "led3",
		.gpio			= S5PV210_GPJ2(2),
		.active_low		= 1,
		.default_trigger	= "mmc0",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},{
		.name			= "led4",
		.gpio			= S5PV210_GPJ2(3),
		.active_low		= 1,
		.default_trigger	= "nand-disk",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device tiny210_leds = {
	.name	= "leds-gpio",
	.id	= 0,
	.dev	= {
		.platform_data	= &gpio_led_info,
	}
};

static struct mtd_partition tiny210_nand_part[] = {
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

static struct s3c2410_nand_set tiny210_nand_sets[] = {
	[0] = {
		.name		= "nand",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(tiny210_nand_part),
		.partitions	= tiny210_nand_part,
	},
};

static struct s3c2410_platform_nand tiny210_nand_info = {
	.tacls		= 25,
	.twrph0		= 55,
	.twrph1		= 40,
	.nr_sets	= ARRAY_SIZE(tiny210_nand_sets),
	.sets		= tiny210_nand_sets,
};

static struct s3c_fb_pd_win tiny210_fb_win0 = {
	.xres		= 800,
	.yres		= 480,
	.max_bpp	= 32,
	.default_bpp	= 24,
};

static struct fb_videomode tiny210_fb_timing = {
	.left_margin	= 40,
	.right_margin	= 40,
	.upper_margin	= 29,
	.lower_margin	= 13,
	.hsync_len	= 48,
	.vsync_len	= 3,
	.xres		= 800,
	.yres		= 480,
};

static struct s3c_fb_platdata tiny210_fb_pdata __initdata = {
	.win[0]		= &tiny210_fb_win0,
	.vtiming	= &tiny210_fb_timing,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
	.setup_gpio	= s5pv210_fb_gpio_setup_24bpp,
};

static void lcd_gpio_init(void)
{
	int err;

	err = gpio_request(S5PV210_GPH1(2), "GPH1_2");
	if (err)
		printk(KERN_ERR "#### failed to request GPH1(2) \n");

	s3c_gpio_setpull(S5PV210_GPH1(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPH1(2), 1);
	gpio_free(S5PV210_GPH1(2));

	err = gpio_request(S5PV210_GPD0(1), "GPD0_1");
	if (err)
		printk(KERN_ERR "#### failed to request GPD0(1) \n");

	s3c_gpio_setpull(S5PV210_GPD0(1), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPD0(1), 1);
	gpio_free(S5PV210_GPD0(1));
}

static void cam_a_init(void)
{
	int err;
	err = gpio_request(S5PV210_GPJ2(6), "GPJ2_6");
	if (err)
		printk(KERN_ERR "#### failed to request GPJ2(6) \n");

	s3c_gpio_setpull(S5PV210_GPJ2(6), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPJ2(6), 1);
	gpio_free(S5PV210_GPJ2(6));

	err = gpio_request(S5PV210_GPJ3(1), "GPJ3_1");
	if (err)
		printk(KERN_ERR "#### failed to request GPJ3(1) \n");

	s3c_gpio_setpull(S5PV210_GPJ3(1), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPJ3(1), 0);

	msleep(10);

	gpio_set_value(S5PV210_GPJ3(1), 1);

	gpio_free(S5PV210_GPJ2(6));
}

static void __init tiny210_map_io(void)
{
	s5pv210_init_io(NULL, 0);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(tiny210_uartcfgs, ARRAY_SIZE(tiny210_uartcfgs));
	s5p_set_timer_source(S5P_PWM3, S5P_PWM4);
}

static void __init tiny210_reserve(void)
{
	s5p_mfc_reserve_mem(0x3EC00000, 10 << 20, 0x3F600000, 10 << 20);
}

#ifdef CONFIG_VIDEO_TVP5150
static struct i2c_board_info tvp5150_board_info = {
	I2C_BOARD_INFO("tvp5150", 0x5c),
};

static struct s5p_fimc_isp_info tiny210_video_capture_devs[] = {
	{
		.mux_id		= 0,
		.bus_type	= FIMC_ITU_656,
		.board_info	= &tvp5150_board_info,
		.i2c_bus_num	= 0,
		.clk_frequency	= 27000000UL,
		.flags		= 0
	},
};

static struct s5p_platform_fimc tiny210_fimc_md_platdata __initdata = {
	.isp_info	= tiny210_video_capture_devs,
	.num_clients	= ARRAY_SIZE(tiny210_video_capture_devs),
};
#endif

static struct i2c_board_info __initdata i2c0_devices[] = {
	{ 	I2C_BOARD_INFO("wm8960", 0x1a),
		.platform_data = &(struct wm8960_data) {
			.capless = false,
			.shared_lrclk = true,
			},
	},
};

static struct platform_device tiny210_audio = {
	.name	= "tiny210sdk-audio",
	.id	= 0,
};

static void tiny210_set_audio_clk(void)
{
	u32 *regs;
	regs = ioremap(S5PV210_PA_ACLK, 0xc);
	if (regs) {
		printk("ASS CLK SRC: 0x%08x\n", regs[0]);
		printk("ASS CLK DIV: 0x%08x\n", regs[1]);
		printk("ASS CLK GTE: 0x%08x\n", regs[2]);
		iounmap(regs);
	} else {
		printk("Can't ioremap audio clock controller\n");
	}
}

static struct s5p_ehci_platdata tiny210_ehci_pdata;

static struct platform_device *tiny210_devices[] __initdata = {
	&s3c_device_fb,
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc_md,
	&s3c_device_hsmmc0,
	&s3c_device_i2c0,
	&s5pv210_device_iis0,
	&s5p_device_mfc,
	&s5p_device_mfc_l,
	&s5p_device_mfc_r,
	&s5p_device_jpeg,
	&s3c_device_rtc,
	&s3c_device_wdt,
	&s3c_device_nand,
	&s5p_device_ehci,
	&samsung_asoc_dma,
	&samsung_asoc_idma,
	&tiny210_device_dm9000,
	&tiny210_leds,
	&tiny210_audio,
};

static void __init tiny210_machine_init(void)
{
	s3c_pm_init();

	lcd_gpio_init();
	cam_a_init();

	s3c_nand_set_platdata(&tiny210_nand_info);

	s3c_fb_set_platdata(&tiny210_fb_pdata);

	tiny210_dm9000_set();

	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c0_devices, ARRAY_SIZE(i2c0_devices));

#ifdef CONFIG_VIDEO_TVP5150
	/* FIMC */
	s5pv210_fimc_setup_gpio(S5P_CAMPORT_A);
	s3c_set_platdata(&tiny210_fimc_md_platdata, sizeof(tiny210_fimc_md_platdata),
			 &s5p_device_fimc_md);
#endif
	s5p_ehci_set_platdata(&tiny210_ehci_pdata);

	tiny210_set_audio_clk();

	platform_add_devices(tiny210_devices, ARRAY_SIZE(tiny210_devices));
}

MACHINE_START(TINY210SDK, "TINY210 SDK")
	/* Maintainer: Mike Dyer <mike.dyer@md-soft.co.uk> */
	.atag_offset	= 0x100,
	.init_irq	= s5pv210_init_irq,
	.handle_irq	= vic_handle_irq,
	.map_io		= tiny210_map_io,
	.init_machine	= tiny210_machine_init,
	.timer		= &s5p_timer,
	.restart	= s5pv210_restart,
	.reserve	= tiny210_reserve
MACHINE_END
