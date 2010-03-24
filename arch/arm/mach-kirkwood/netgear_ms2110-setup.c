/*
 * arch/arm/mach-kirkwood/netgear_ms2110-setup.c 
 *
 * Netgear MS2110 (Stora) Board Setup
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mtd/partitions.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_eth.h>
#include <linux/i2c.h>
#include <linux/i2c/at24.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/leds.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/kirkwood.h>
#include <plat/mvsdio.h>
#include "common.h"
#include "mpp.h"

static struct mtd_partition netgear_ms2110_nand_parts[] = {
	{
		.name = "u-boot",
		.offset = 0,
		.size = SZ_1M
	}, {
		.name = "uImage",
		.offset = MTDPART_OFS_NXTBLK,
		.size = SZ_4M
	}, {
		.name = "root",
		.offset = MTDPART_OFS_NXTBLK,
		.size = MTDPART_SIZ_FULL
	},
};

static struct mv643xx_eth_platform_data netgear_ms2110_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR(8),
};

static struct mv_sata_platform_data netgear_ms2110_sata_data = {
	.n_ports	= 2,
};

static struct i2c_board_info  __initdata netgear_ms2110_i2c_info[] = {
   { I2C_BOARD_INFO("pcf8563", 0x51 ) },
   { I2C_BOARD_INFO("lm75", 0x48 ) }
}; 

#define NETGEAR_MS2110_POWER_BUTTON 36
#define NETGEAR_MS2110_RESET_BUTTON 38

static struct gpio_keys_button netgear_ms2110_buttons[] = {
        [0] = {
                .code           = KEY_POWER,
                .gpio           = NETGEAR_MS2110_POWER_BUTTON,
                .desc           = "Power push button",
                .active_low     = 1,
        },
        [1] = {
                .code           = KEY_POWER2,
                .gpio           = NETGEAR_MS2110_RESET_BUTTON,
                .desc           = "Reset push button",
                .active_low     = 1,
        },
};

static struct gpio_keys_platform_data netgear_ms2110_button_data = {
        .buttons        = netgear_ms2110_buttons,
        .nbuttons       = ARRAY_SIZE(netgear_ms2110_buttons),
};

static struct platform_device netgear_ms2110_gpio_buttons = {
        .name           = "gpio-keys",
        .id             = -1,
        .dev            = {
                .platform_data  = &netgear_ms2110_button_data,
        },
};

static unsigned int netgear_ms2110_mpp_config[] __initdata = {
	MPP0_NF_IO2,
	MPP1_NF_IO3,
	MPP2_NF_IO4,
	MPP3_NF_IO5,
	MPP4_NF_IO6,
	MPP5_NF_IO7,
	MPP6_SYSRST_OUTn,
	MPP7_SPI_SCn,
	MPP8_TW_SDA,
	MPP9_TW_SCK,
	MPP10_UART0_TXD,
	MPP11_UART0_RXD,
	MPP12_SD_CLK,
	MPP13_SD_CMD,
	MPP14_SD_D0,
	MPP15_SD_D1,
	MPP16_SD_D2,
	MPP17_SD_D3,
	MPP18_NF_IO0,
	MPP19_NF_IO1,
	MPP20_SATA1_ACTn, /* green led for drive 2 */
	MPP21_SATA0_ACTn, /* green led for drive 1 */
	MPP22_GPIO,   /* red led for drive 2 */
	MPP23_GPIO,   /* red led for drive 1 */
	MPP24_GE1_4,
	MPP25_GE1_5,
	MPP26_GE1_6,
	MPP27_GE1_7,
	MPP28_GPIO,
	MPP29_GPIO,
	MPP30_GPIO, 
	MPP31_GPIO,  /* blue led for power indicator */
	MPP32_GPIO,
	MPP33_GE1_13,
	MPP34_SATA1_ACTn, /* positively retarded, unused, and nonstandard */
	MPP35_GPIO,
	MPP36_GPIO,  /* power button */
	MPP37_GPIO,  /* reset button */
	MPP38_GPIO,
	MPP39_GPIO,
	MPP40_GPIO,  /* low output powers off board */
	MPP41_GPIO,
	MPP42_GPIO,
	MPP43_GPIO,
	MPP44_GPIO,
	MPP45_TDM_PCLK,
	MPP46_TDM_FS,
	MPP47_TDM_DRX,
	MPP48_TDM_DTX,
	MPP49_GPIO,
	0
};

#define NETGEAR_MS2110_GPIO_POWER_OFF 40

static void netgear_ms2110_power_off(void)
{
        gpio_set_value(NETGEAR_MS2110_GPIO_POWER_OFF, 1);
}

#define NETGEAR_MS2110_GPIO_BLUE_LED     31
#define NETGEAR_MS2110_GPIO_RED1_LED     23
#define NETGEAR_MS2110_GPIO_RED2_LED     22

int gpio_blink_set(unsigned gpio, unsigned long *delay_on, unsigned long *delay_off) {
	if (*delay_on == 0 && *delay_off == 1) { 
		/* this special case turns on hardware blinking */
		orion_gpio_set_blink(gpio,1);
		return 0;
	}
	orion_gpio_set_blink(gpio,0); /* turn off hardware blinking */
	if (*delay_off == 0 && *delay_off == 0)
		return 0; /* we're done */
	else
		return 1; /* fall back to the software blinking */
}

static struct gpio_led netgear_ms2110_gpio_led_pins[] = {
        {
                .name   = "blue",
                .gpio   = NETGEAR_MS2110_GPIO_BLUE_LED,
                .active_low     = 1,
		.default_trigger = "heartbeat",
		.default_state = LEDS_GPIO_DEFSTATE_KEEP,
        },
        {
                .name   = "red1",
                .gpio   = NETGEAR_MS2110_GPIO_RED1_LED,
                .active_low     = 1,
		.default_trigger = "none",
		.default_state = LEDS_GPIO_DEFSTATE_KEEP,
        },
        {
                .name   = "red2",
                .gpio   = NETGEAR_MS2110_GPIO_RED2_LED,
                .active_low     = 1,
		.default_trigger = "none",
		.default_state = LEDS_GPIO_DEFSTATE_KEEP,
        },
};

static struct gpio_led_platform_data netgear_ms2110_gpio_leds_data = {
        .num_leds       = ARRAY_SIZE(netgear_ms2110_gpio_led_pins),
        .leds           = netgear_ms2110_gpio_led_pins,
	.gpio_blink_set = &gpio_blink_set,
};


static struct platform_device netgear_ms2110_gpio_leds = {
        .name           = "leds-gpio",
        .id             = -1,
        .dev            = {
                .platform_data  = &netgear_ms2110_gpio_leds_data,
        },
};

static void __init netgear_ms2110_init(void)
{
	/*
	 * Basic setup. Needs to be called early.
	 */
	kirkwood_init();
	kirkwood_mpp_conf(netgear_ms2110_mpp_config);

	kirkwood_uart0_init();
	kirkwood_nand_init(ARRAY_AND_SIZE(netgear_ms2110_nand_parts), 25);

	kirkwood_ehci_init();

	kirkwood_ge00_init(&netgear_ms2110_ge00_data);
	kirkwood_sata_init(&netgear_ms2110_sata_data);

	kirkwood_i2c_init();
	i2c_register_board_info(0, netgear_ms2110_i2c_info,
                                ARRAY_SIZE(netgear_ms2110_i2c_info));
        
	platform_device_register(&netgear_ms2110_gpio_buttons);

	if (gpio_request(NETGEAR_MS2110_GPIO_POWER_OFF, "power-off") == 0 &&
            gpio_direction_output(NETGEAR_MS2110_GPIO_POWER_OFF, 0) == 0)
                pm_power_off = netgear_ms2110_power_off;
        else
                pr_err("netgear_ms2110: failed to configure power-off GPIO\n");

	if (gpio_request(NETGEAR_MS2110_GPIO_BLUE_LED,"power-light") == 0 &&
	    gpio_direction_output(NETGEAR_MS2110_GPIO_BLUE_LED, 0) == 0) {
	        gpio_set_value(NETGEAR_MS2110_GPIO_BLUE_LED,0);
	        orion_gpio_set_blink(NETGEAR_MS2110_GPIO_BLUE_LED,0);
            } else
	        pr_err("netgear_ms2110: failed to configure blue LED\n");
	gpio_free(NETGEAR_MS2110_GPIO_BLUE_LED);
        platform_device_register(&netgear_ms2110_gpio_leds);

}

static int __init netgear_ms2110_pci_init(void)
{
	if (machine_is_netgear_ms2110())
		kirkwood_pcie_init();

	return 0;
 }
subsys_initcall(netgear_ms2110_pci_init);


MACHINE_START(NETGEAR_MS2110, "Netgear MS2110")
	.phys_io	= KIRKWOOD_REGS_PHYS_BASE,
	.io_pg_offst	= ((KIRKWOOD_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= netgear_ms2110_init,
	.map_io		= kirkwood_map_io,
	.init_irq	= kirkwood_init_irq,
	.timer		= &kirkwood_timer,
MACHINE_END
