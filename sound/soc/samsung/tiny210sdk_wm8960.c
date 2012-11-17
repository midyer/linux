/*
 * tiny210sdk_wm8960.c
 *
 * Copyright (C) 2012 MDSoft Ltd
 * Author: Mike Dyer <mike.dyer@md-soft.co.uk>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <sound/jack.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "i2s.h"
#include "../codecs/wm8960.h"

static int tiny210_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_nc_pin(dapm, "LINPUT2");
	snd_soc_dapm_nc_pin(dapm, "LINPUT3");
	snd_soc_dapm_nc_pin(dapm, "RINPUT1");
	snd_soc_dapm_nc_pin(dapm, "RINPUT2");
	snd_soc_dapm_nc_pin(dapm, "RINPUT3");

	return 0;
}
static int tiny210_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_codec *codec = codec_dai->codec;

	int ret = 0, i = 0;
	int mclk = 24000000;
	int pll = 0;
	int adiv = 0;

	/* set the system clock */
	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
			256, SND_SOC_CLOCK_OUT);
	if (ret != 0) {
		pr_err("%s: couldn't set i2s mclk output\n", __func__);
		return ret;
	}

	/* set the codec pll */
	switch(params_rate(params)) {
	case 44100:
		pll = 11289600;
		adiv = 0;
		break;
	case 22050:
		adiv = 2;
		pll = 11289600;
		break;
	case 11025:
		adiv = 4;
		pll = 11289600;
		break;
	case 8018:
		adiv = 5;
		pll = 11289600;
		break;
	case 48000:
		adiv = 0;
		pll = 12288000;
		break;
	case 32000:
		adiv = 1;
		pll = 12288000;
		break;
	case 24000:
		adiv = 2;
		pll = 12288000;
		break;
	case 16000:
		adiv = 3;
		pll = 12288000;
		break;
	case 12000:
		adiv = 4;
		pll = 12288000;
	case 8000:
		adiv = 6;
		pll = 12288000;
		break;

	default:
		pr_err("%s: unsupported rate\n", __func__);
		return -EINVAL;
	}
	pr_info("%s: setting pll to %d\n", __func__, pll);
	ret = snd_soc_dai_set_pll(codec_dai, WM8960_SYSCLK_PLL, WM8960_SYSCLK_MCLK, mclk, pll);
	if (ret != 0)
		pr_err("%s: couldn't set PLL\n", __func__);

	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_DACDIV, adiv << 3);
	if (ret != 0)
		pr_err("%s: couldn't set DAC divider\n", __func__);

	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8960_ADCDIV, adiv << 6);
	if (ret != 0)
		pr_err("%s: couldn't set DAC divider\n", __func__);

	for (i = 0; i <= 0x37; i++) {
		printk(KERN_ERR "0x%02x == 0x%03x\n", i, snd_soc_read(codec, i));
	}
	return ret;
}

static const struct snd_soc_dapm_widget tiny210_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker Hdr", NULL),
	SND_SOC_DAPM_MIC("Microphone", NULL),
};

static const struct snd_soc_dapm_route tiny210_audio_map[] = {
	{"Headphone Jack", NULL, "HP_R"},
	{"Headphone Jack", NULL, "HP_L"},
	{"Speaker Hdr", NULL, "SPK_RP"},
	{"Speaker Hdr", NULL, "SPK_RN"},
	{"Speaker Hdr", NULL, "SPK_LP"},
	{"Speaker Hdr", NULL, "SPK_LN"},
	{"Microphone", NULL, "LINPUT1"},
};

static struct snd_soc_ops tiny210_ops = {
	//.startup = tiny210_startup,
	.hw_params = tiny210_hw_params,
	//.shutdown = tiny210_shutdown,
};

static struct snd_soc_dai_link tiny210_dai = {
	.name = "WM8960",
	.stream_name = "WM8960",
	.cpu_dai_name = "samsung-i2s.0",
	.codec_dai_name = "wm8960-hifi",
	.platform_name = "samsung-idma",
	.codec_name = "wm8960.0-001a",
	.init = tiny210_init,
	.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		   SND_SOC_DAIFMT_CBM_CFM,
	.ops = &tiny210_ops,
};

static struct snd_soc_card tiny210_card = {
	.name = "tiny210",
	.owner = THIS_MODULE,
	.dai_link = &tiny210_dai,
	.num_links = 1,
	.dapm_widgets = tiny210_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(tiny210_dapm_widgets),
	.dapm_routes = tiny210_audio_map,
	.num_dapm_routes = ARRAY_SIZE(tiny210_audio_map),
};

static int __devinit tiny210_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &tiny210_card;
	int ret;

	card->dev = &pdev->dev;

	ret = snd_soc_register_card(card);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n",
			ret);
	return ret;
}

static int __devexit tiny210_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	snd_soc_unregister_card(card);
	return 0;
}

static struct platform_driver tiny210_driver = {
	.driver		= {
		.name	= "tiny210sdk-audio",
		.owner	= THIS_MODULE,
	},
	.probe		= tiny210_probe,
	.remove		= __devexit_p(tiny210_remove),
};

module_platform_driver(tiny210_driver);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC WM8960 TINY210SDK(S5PV210)");
MODULE_AUTHOR("Mike Dyer <mike.dyer@md-soft.co.uk");
MODULE_LICENSE("GPL");
