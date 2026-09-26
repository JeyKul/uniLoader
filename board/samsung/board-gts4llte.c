// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026 JeyKul
 *
 */

#include <board.h>
#include <util.h>
#include <drivers/framework.h>
#include <lib/simplefb.h>

static struct video_info gts4llte_fb = {
    .format = FB_FORMAT_ARGB8888,
    .width = 2560,
    .height = 3200,
    .stride = 4,
    .scale = 1,
    .rotate = 1,
    .address = (void *)0x9d400000,
};

static const struct device gts4llte_devices[] = {
    { "simplefb", &gts4llte_fb, "fb" },
};

struct board_data board_ops = {
    .name = "samsung-gts4llte",
    .ops = {
    },
    .devices = gts4llte_devices,
    .num_devices = ARRAY_SIZE(gts4llte_devices),
    .quirks = 0,
};