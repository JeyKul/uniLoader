// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022, Ivaylo Ivanov <ivo.ivanov.ivanov1@gmail.com>
 * Copyright (c) 2026, Igor Belwon <igor.belwon@mentallysanemainliners.org>
 */

#include <lib/debug.h>
#include <main/boot.h>
#include <string.h>

#define GTS4L_DTB_ENTRY        0xa9000000UL
#define GTS4L_TRAMPOLINE_ENTRY 0xa0f00000UL
#define FDT_MAGIC_LE           0xedfe0dd0U

static unsigned int be32_to_cpu(unsigned int value)
{
    return ((value & 0x000000ffU) << 24) |
           ((value & 0x0000ff00U) << 8) |
           ((value & 0x00ff0000U) >> 8) |
           ((value & 0xff000000U) >> 24);
}

static void sync_instruction_word(unsigned long addr)
{
    asm volatile(
        "dc cvau, %0\n"
        "dsb ish\n"
        "ic ivau, %0\n"
        "dsb ish\n"
        "isb\n"
        :
        : "r"(addr)
        : "memory"
    );
}


void arch_load_kernel(void *kernel, void *dt, void *ramdisk)
{
	volatile unsigned int *dtb_header;
	volatile unsigned int *dtb_dest;
	volatile unsigned int *tramp_test;
	unsigned long kernel_len;
	unsigned int kernel_first_src;
	unsigned int kernel_first_dst;
	unsigned int kernel_last_src;
	unsigned int kernel_last_dst;
	unsigned int dtb_magic;
	unsigned int dtb_size;
	unsigned long *image_header;
	unsigned long current_el;
	unsigned long sctlr_el1;
	unsigned long tcr_el1;
	unsigned long ttbr0_el1;
	unsigned long ttbr1_el1;

	printk(KERN_INFO, "\n\n[UL-6]\n");

	kernel_len = (unsigned long)&kernel_size;

printk(KERN_INFO, "\n[UL-7]\n");

memcpy((void *)CONFIG_PAYLOAD_ENTRY, kernel, kernel_len);

kernel_first_src = *(volatile unsigned int *)kernel;
kernel_first_dst = *(volatile unsigned int *)CONFIG_PAYLOAD_ENTRY;

kernel_last_src = *(volatile unsigned int *)
	((unsigned long)kernel + kernel_len - sizeof(unsigned int));
kernel_last_dst = *(volatile unsigned int *)
	((unsigned long)CONFIG_PAYLOAD_ENTRY + kernel_len -
	 sizeof(unsigned int));

if (kernel_first_src != kernel_first_dst ||
    kernel_last_src != kernel_last_dst) {
	printk(KERN_ERR, "\n[IMG-COPY-BAD]\n");
	for (;;)
		;
}

printk(KERN_INFO,
       "[IMG-COPY-OK L=%lx H=%x T=%x]\n",
       kernel_len, kernel_first_dst, kernel_last_dst);

printk(KERN_INFO, "\n[UL-8]\n");

#ifndef CONFIG_RAMDISK_NO_COPY
	printk(KERN_INFO, "\n[UL-9]\n");
	__optimized_memcpy((void *)CONFIG_RAMDISK_ENTRY, ramdisk,
			   (unsigned long)&ramdisk_size);
	printk(KERN_INFO, "\n[UL-10]\n");
#else
	printk(KERN_INFO, "\n[UL-RAMDISK-NO-COPY]\n");
#endif

	/*
	 * A DTB begins with big-endian 0xd00dfeed. Reading it as a native
	 * little-endian u32 on this CPU produces 0xedfe0dd0.
	 */
	dtb_header = (volatile unsigned int *)dt;
	dtb_magic = dtb_header[0];
	dtb_size = be32_to_cpu(dtb_header[1]);

	if (dtb_magic == FDT_MAGIC_LE)
		printk(KERN_INFO, "\n[DTB-OK]\n");
	else
		printk(KERN_ERR, "\n[DTB-BAD]\n");

	printk(KERN_INFO, "[DTB-SZ=%x]\n", dtb_size);

	if (dtb_magic != FDT_MAGIC_LE ||
	    dtb_size < 40 ||
	    dtb_size > 0x00200000U) {
		printk(KERN_ERR, "\n[DTB-INVALID]\n");
		for (;;)
			;
	}

	/*
	 * Verify the chosen low-RAM destination is writable before attempting
	 * a ~500 KiB copy. The original value is not important: it will be
	 * overwritten by the real DTB immediately afterward.
	 */
	dtb_dest = (volatile unsigned int *)GTS4L_DTB_ENTRY;

	printk(KERN_INFO, "\n[DTB-DST-TEST]\n");
	dtb_dest[0] = 0xa5a55a5aU;

	if (dtb_dest[0] != 0xa5a55a5aU) {
		printk(KERN_ERR, "\n[DTB-DST-FAIL]\n");
		for (;;)
			;
	}

	printk(KERN_INFO, "\n[DTB-DST-OK]\n");

	printk(KERN_INFO, "\n[DTB-COPY]\n");
	memcpy((void *)GTS4L_DTB_ENTRY, dt, dtb_size);

	if (dtb_dest[0] != FDT_MAGIC_LE) {
		printk(KERN_ERR, "\n[DTB-COPY-BAD]\n");
		for (;;)
			;
	}

	printk(KERN_INFO, "\n[DTB-COPY-OK]\n");

	tramp_test = (volatile unsigned int *)GTS4L_TRAMPOLINE_ENTRY;

printk(KERN_INFO, "\n[TRAMP-DST-TEST]\n");

tramp_test[0] = 0xa55a5aa5U;

if (tramp_test[0] != 0xa55a5aa5U) {
	printk(KERN_ERR, "\n[TRAMP-DST-FAIL]\n");
	for (;;)
		;
}

printk(KERN_INFO, "\n[TRAMP-DST-OK]\n");

	image_header = (unsigned long *)CONFIG_PAYLOAD_ENTRY;

	printk(KERN_INFO, "\n[IMG-I=%lx]\n", image_header[0]);
	printk(KERN_INFO, "[IMG-O=%lx]\n", image_header[1]);
	printk(KERN_INFO, "[IMG-S=%lx]\n", image_header[2]);
	printk(KERN_INFO, "[IMG-F=%lx]\n", image_header[3]);

	printk(KERN_INFO,
	       "[ADDR D=%p K=%p P=%p R=%p]\n",
	       dt, kernel, (void *)CONFIG_PAYLOAD_ENTRY,
	       (void *)CONFIG_RAMDISK_ENTRY);

	asm volatile("mrs %0, CurrentEL" : "=r"(current_el));
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr_el1));
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr_el1));
    asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0_el1));
    asm volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1_el1));

    printk(KERN_INFO, "[EL=%lu]\n", current_el >> 2);
    printk(KERN_INFO, "[SCTLR=%lx]\n", sctlr_el1);
    printk(KERN_INFO, "[TCR=%lx]\n", tcr_el1);
    printk(KERN_INFO, "[TTBR0=%lx]\n", ttbr0_el1);
    printk(KERN_INFO, "[TTBR1=%lx]\n", ttbr1_el1);


	printk(KERN_INFO,
       "[KTEST size=%lx first=%08x last=%08x]\n",
       (unsigned long)&kernel_size,
       *(volatile unsigned int *)CONFIG_PAYLOAD_ENTRY,
       *(volatile unsigned int *)
       (CONFIG_PAYLOAD_ENTRY +
        (unsigned long)&kernel_size -
        sizeof(unsigned int)));
	
/*
 * DIAGNOSTIC ONLY:
 *
 * Place AArch64 `ret` at the payload entry. Synchronize that data write
 * with instruction fetch before start.S executes BLR to this address.
 */
((volatile unsigned int *)CONFIG_PAYLOAD_ENTRY)[0] = 0xd65f03c0U;
sync_instruction_word(CONFIG_PAYLOAD_ENTRY);

if (*(volatile unsigned int *)CONFIG_PAYLOAD_ENTRY != 0xd65f03c0U) {
    printk(KERN_ERR, "\n[ENTRY-STUB-BAD]\n");
    for (;;)
        ;
}

printk(KERN_INFO, "\n[ENTRY-STUB-READY]\n");

	printk(KERN_INFO, "\n[UL-11]\n");

	/*
	 * Linux receives the low staging location in x0 rather than the
	 * high relocated uniLoader pointer.
	 */
	load_kernel_and_jump((void *)GTS4L_DTB_ENTRY, 0, 0, 0,
			     (void *)CONFIG_PAYLOAD_ENTRY,
			     (void *)(CONFIG_PAYLOAD_ENTRY +
				      (unsigned long)&kernel_size));

	printk(KERN_INFO, "\n[STUB-RETURNED]\n");

	for (;;)
		;
}