// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026 Ryo "evilMyQueen" Yamada <evilMyQueen@mainlining.org>
 */

#include <board.h>
#include <util.h>
#include <drivers/framework.h>
#include <lib/simplefb.h>

static struct video_info gta4l_fb = {
    .format = FB_FORMAT_ARGB8888,
    .width = 2560,
    .height = 1600,
    .stride = 20,
    .scale = 1,
    .address = (void *)0x9d600000,
};

static const struct device gta4l_devices[] = {
	{ "simplefb", &gta4l_fb, "fb" },
};

struct board_data board_ops = {
	.name = "samsung-gta4l",
	.ops = {
	},
	.devices = gta4l_devices,
	.num_devices = ARRAY_SIZE(gta4l_devices),
	.quirks = 0
};