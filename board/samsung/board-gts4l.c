// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026 Ryo "evilMyQueen" Yamada
 * <evilMyQueen@mainlining.org>
 */

#include <board.h>
#include <util.h>
#include <drivers/framework.h>
#include <lib/simplefb.h>

#define UART_BASE                       0x0c1b0000UL

#define UARTDM_SR                       0x00a4
#define UARTDM_CR                       0x00a8
#define UARTDM_ISR                      0x00b4
#define UARTDM_NCF_TX                   0x0040
#define UARTDM_TF                       0x0100

#define UARTDM_SR_TX_EMPTY              (1U << 3)
#define UARTDM_ISR_TX_READY             (1U << 7)
#define UARTDM_CR_CMD_RESET_TX_READY    (3U << 8)

static inline unsigned int mmio_read32(unsigned long addr)
{
    return *(volatile unsigned int *)addr;
}

static inline void mmio_write32(unsigned long addr, unsigned int value)
{
    *(volatile unsigned int *)addr = value;
}

#ifdef CONFIG_EARLYCON
static void uart_putc(char ch)
{
    unsigned int timeout = 1000000;
    unsigned int sr;
    unsigned int isr;

    while (timeout--) {
        sr = mmio_read32(UART_BASE + UARTDM_SR);
        isr = mmio_read32(UART_BASE + UARTDM_ISR);

        if ((sr & UARTDM_SR_TX_EMPTY) ||
            (isr & UARTDM_ISR_TX_READY))
            break;
    }

    /*
     * Do not trap uniLoader if the UART is not initialized, clocked,
     * physically exposed, or inherited from Samsung firmware.
     */
    if (!timeout)
        return;

    mmio_write32(UART_BASE + UARTDM_CR,
                 UARTDM_CR_CMD_RESET_TX_READY);
    mmio_write32(UART_BASE + UARTDM_NCF_TX, 1);
    mmio_write32(UART_BASE + UARTDM_TF,
                 (unsigned int)(unsigned char)ch);
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');

        uart_putc(*s++);
    }
}
#endif

static struct video_info gts4l_fb = {
    .format = FB_FORMAT_ARGB8888,
    .width = 2560,
    .height = 3200,
    .stride = 4,
    .scale = 2,
    .address = (void *)0x9d400000,
};

static const struct device gts4l_devices[] = {
    { "simplefb", &gts4l_fb, "fb" },
};

struct board_data board_ops = {
    .name = "samsung-gts4l",
    .ops = {
    },
    .devices = gts4l_devices,
    .num_devices = ARRAY_SIZE(gts4l_devices),
    .quirks = 0,
};