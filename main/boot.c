// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022, Ivaylo Ivanov <ivo.ivanov.ivanov1@gmail.com>
 * Copyright (c) 2026, Igor Belwon <igor.belwon@mentallysanemainliners.org>
 */

#include <lib/debug.h>
#include <main/boot.h>
#include <main/boot-fdt.h>
#include <string.h>

void boot_kernel(void *dt, void *kernel, void *ramdisk)
{
	printk(KERN_INFO, "\n\n[UL-1: boot_kernel]\n");

#ifdef CONFIG_LIBFDT
	printk(KERN_INFO, "\n\n[UL-2: patch_dtb begin]\n");
	patch_dtb(&dt);
	printk(KERN_INFO, "\n\n[UL-3: patch_dtb done]\n");
#else
	printk(KERN_INFO, "\n\n[UL-2: no libfdt]\n");
#endif

	printk(KERN_INFO, "\n\n[UL-4: before arch_load_kernel]\n");
	printk(KERN_INFO, "Booting kernel...\n");
	printk(KERN_INFO, "\n\n[UL-5: calling arch_load_kernel]\n");

	arch_load_kernel(kernel, dt, ramdisk);

	/*
	 * A correct architecture handoff does not return. Seeing this marker
	 * means arch_load_kernel returned unexpectedly.
	 */
	printk(KERN_ERR, "\n\n[UL-ERROR: arch_load_kernel returned]\n");

	for (;;)
		;
}
