/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Ivaylo Ivanov <ivo.ivanov1@gmail.com>
 */

#include "asmdefs.h"

#ifdef CONFIG_LINUX_KRNL_HEADER_IMG
#include "linux-kernel-image-header.h"
#endif

.section .text

#if defined(CONFIG_LINUX_KRNL_HEADER_IMG) && !defined(CONFIG_POSITION_INDEPENDENT)
_head:
        linux_image_header _start

_stack_base:
        .quad 0x0

_stack_size:
        .quad _stack_end

        .align 4
#endif

ENTRY(_start)
        /* Set up stack pointer. */
        adrp    x0, _stack_end
        add     x0, x0, #:lo12:_stack_end
        mov     sp, x0

        /* Zero BSS. */
        adrp    x0, __bss_start
        add     x0, x0, #:lo12:__bss_start
        adrp    x1, __bss_end
        add     x1, x1, #:lo12:__bss_end

1:
        cmp     x0, x1
        b.ge    2f
        str     xzr, [x0], #8
        b       1b

2:
        /*
         * Pass embedded DTB, raw Image and ramdisk pointers into main().
         */
        adrp    x0, dtb
        add     x0, x0, #:lo12:dtb
        adrp    x1, kernel
        add     x1, x1, #:lo12:kernel

#ifndef CONFIG_RAMDISK_NO_COPY
        adrp    x2, ramdisk
        add     x2, x2, #:lo12:ramdisk
#endif

        bl      main
END(_start)

ENTRY(load_kernel_and_jump)
	msr	daifset, #0xf
	dsb	sy
	isb
	br	x4
END(load_kernel_and_jump)