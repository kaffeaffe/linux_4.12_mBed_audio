/*
 * ASoC driver for mbed AudioCODEC (with a TLV320AIC23b)
 * connected to a Raspberry Pi
 *
 * Author:      Florian Meier, <koalo@koalo.de>, Alfred Samuelson (Device Tree Support) <alfredsa@kth.se>
 *	      Copyright 2013
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "../codecs/tlv320aic23.h"

static int snd_rpi_mbed_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

static int snd_rpi_mbed_hw_params(struct snd_pcm_substream *substream,
				       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int sysclk = 12000000; /* this is fixed on this board */
	
	/* Set mbed bclk */
	int ret = snd_soc_dai_set_bclk_ratio(cpu_dai,32*2);
	if (ret < 0){
		dev_err(codec->dev,
				"Failed to set BCLK ratio %d\n", ret);
		return ret;
	}
	
	/* set mbed sysclk */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, sysclk, 0);
	
	if (ret < 0) {
		dev_err(codec->dev,
				"Failed to set tlv320aic23 SYSCLK: %d\n", ret);
		return ret;
	}

	return 0;
}

/* machine stream operations */
static struct snd_soc_ops snd_rpi_mbed_ops = {
	.hw_params = snd_rpi_mbed_hw_params,
};

static struct snd_soc_dai_link snd_rpi_mbed_dai[] = {
{
	.name		= "TLV320AIC23",
	.stream_name	= "TLV320AIC23 HiFi",
	.cpu_dai_name	= "bcm2708-i2s.0",
	.codec_dai_name	= "tlv320aic23-hifi",
	.platform_name	= "bcm2708-i2s.0",
	.codec_name	= "tlv320aic23-codec.1-001b",
	.dai_fmt	= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
				SND_SOC_DAIFMT_CBS_CFS,
	.ops		= &snd_rpi_mbed_ops,
	.init		= snd_rpi_mbed_init,
},
};

/* audio machine driver */
static struct snd_soc_card snd_rpi_mbed = {
	.name	 = "snd_rpi_mbed",
	.dai_link     = snd_rpi_mbed_dai,
	.num_links    = ARRAY_SIZE(snd_rpi_mbed_dai),
};

static int snd_rpi_mbed_probe(struct platform_device *pdev)
{
	int ret = 0;

	snd_rpi_mbed.dev = &pdev->dev;
	
	if (pdev->dev.of_node) {
		struct device_node *i2s_node;
		struct snd_soc_dai_link *dai = &snd_rpi_mbed_dai[0];
		i2s_node = of_parse_phandle(pdev->dev.of_node, "i2s-controller", 0);

		if (i2s_node) {
			dai->cpu_dai_name = NULL;
			dai->cpu_of_node = i2s_node;
			dai->platform_name = NULL;
			dai->platform_of_node = i2s_node;
		}
	}
	ret = snd_soc_register_card(&snd_rpi_mbed);
	if (ret) {
		dev_err(&pdev->dev,
				"snd_soc_register_card() failed: %d\n", ret);
	}

	return ret;
}


static int snd_rpi_mbed_remove(struct platform_device *pdev)
{
	return snd_soc_unregister_card(&snd_rpi_mbed);
}

static const struct of_device_id snd_rpi_mbed_of_match[] = {
	{ .compatible = "rpi,rpi-mbed", },
	{},
};
MODULE_DEVICE_TABLE(of, snd_rpi_mbed_of_match);

static struct platform_driver snd_rpi_mbed_driver = {
	.driver = {
		.name   = "snd-rpi-mbed",
		.owner  = THIS_MODULE,
		.of_match_table = snd_rpi_mbed_of_match,
	},
	.probe	  = snd_rpi_mbed_probe,
	.remove	 = snd_rpi_mbed_remove,
};

module_platform_driver(snd_rpi_mbed_driver);

MODULE_AUTHOR("Florian Meier, Alfred Samuelson");
MODULE_DESCRIPTION("ASoC Driver for Raspberry Pi connected to mbed AudioCODEC");
MODULE_LICENSE("GPL");
