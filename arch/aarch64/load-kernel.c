/*
 * Copyright (c) 2022, Ivaylo Ivanov <ivo.ivanov.ivanov1@gmail.com>
 * Copyright (c) 2026, Igor Belwon <igor.belwon@mentallysanemainliners.org>
 */

#include <lib/debug.h>
#include <main/boot.h>
#include <string.h>

#define GTS4L_DTB_ENTRY 0xa9000000UL
#define FDT_MAGIC_LE    0xedfe0dd0U
#define ARM64_IMAGE_MAGIC 0x644d5241U

static unsigned int be32_to_cpu(unsigned int value)
{
    return ((value & 0x000000ffU) << 24) |
           ((value & 0x0000ff00U) << 8) |
           ((value & 0x00ff0000U) >> 8) |
           ((value & 0xff000000U) >> 24);
}

static void clean_to_poc(unsigned long addr, unsigned long len)
{
    unsigned long ctr, line, p, end;

    if (!len)
        return;

    asm volatile("mrs %0, ctr_el0" : "=r"(ctr));
    line = 4UL << ((ctr >> 16) & 0xf);
    end = addr + len;

    for (p = addr & ~(line - 1); p < end; p += line)
        asm volatile("dc cvac, %0" : : "r"(p) : "memory");

    asm volatile("dsb sy" : : : "memory");
}

void arch_load_kernel(void *kernel, void *dt, void *ramdisk)
{
    volatile unsigned int *dtb_header;
    volatile unsigned int *dtb_dest;
    unsigned long kernel_len;
    unsigned int kernel_first_src;
    unsigned int kernel_first_dst;
    unsigned int kernel_last_src;
    unsigned int kernel_last_dst;
    unsigned int dtb_magic;
    unsigned int dtb_size;
    unsigned long *image_header;
    unsigned long text_offset;
    unsigned long image_size;
    unsigned long current_el;
    unsigned long sctlr_el1;
    unsigned long tcr_el1;
    unsigned long ttbr0_el1;
    unsigned long ttbr1_el1;
#ifndef CONFIG_RAMDISK_NO_COPY
    unsigned long ramdisk_len;
#endif

    printk(KERN_INFO, "\n\n[UL-6]\n");

    kernel_len = (unsigned long)&kernel_size;
    if (kernel_len < 64 ||
        kernel_len > CONFIG_RAMDISK_ENTRY - CONFIG_PAYLOAD_ENTRY) {
        printk(KERN_ERR, "\n[IMAGE-LENGTH-BAD]\n");
        for (;;)
            ;
    }

    printk(KERN_INFO, "\n[UL-7]\n");
    memcpy((void *)CONFIG_PAYLOAD_ENTRY, kernel, kernel_len);

    kernel_first_src = *(volatile unsigned int *)kernel;
    kernel_first_dst = *(volatile unsigned int *)CONFIG_PAYLOAD_ENTRY;
    kernel_last_src = *(volatile unsigned int *)
        ((unsigned long)kernel + kernel_len - sizeof(unsigned int));
    kernel_last_dst = *(volatile unsigned int *)
        (CONFIG_PAYLOAD_ENTRY + kernel_len - sizeof(unsigned int));

    if (kernel_first_src != kernel_first_dst ||
        kernel_last_src != kernel_last_dst) {
        printk(KERN_ERR, "\n[IMG-COPY-BAD]\n");
        for (;;)
            ;
    }

    printk(KERN_INFO, "[IMG-COPY-OK L=%lx H=%x T=%x]\n",
           kernel_len, kernel_first_dst, kernel_last_dst);
    printk(KERN_INFO, "\n[UL-8]\n");

#ifndef CONFIG_RAMDISK_NO_COPY
    ramdisk_len = (unsigned long)&ramdisk_size;
    if (ramdisk_len > GTS4L_DTB_ENTRY - CONFIG_RAMDISK_ENTRY) {
        printk(KERN_ERR, "\n[RAMDISK-RANGE-BAD]\n");
        for (;;)
            ;
    }
    printk(KERN_INFO, "\n[UL-9]\n");
    __optimized_memcpy((void *)CONFIG_RAMDISK_ENTRY, ramdisk, ramdisk_len);
    printk(KERN_INFO, "\n[UL-10]\n");
#else
    printk(KERN_INFO, "\n[UL-RAMDISK-NO-COPY]\n");
#endif

    dtb_header = (volatile unsigned int *)dt;
    dtb_magic = dtb_header[0];
    dtb_size = be32_to_cpu(dtb_header[1]);

    if (dtb_magic == FDT_MAGIC_LE)
        printk(KERN_INFO, "\n[DTB-OK]\n");
    else
        printk(KERN_ERR, "\n[DTB-BAD]\n");
    printk(KERN_INFO, "[DTB-SZ=%x]\n", dtb_size);

    if (dtb_magic != FDT_MAGIC_LE ||
        dtb_size < 40 || dtb_size > 0x00200000U) {
        printk(KERN_ERR, "\n[DTB-INVALID]\n");
        for (;;)
            ;
    }

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

    image_header = (unsigned long *)CONFIG_PAYLOAD_ENTRY;
    text_offset = image_header[1];
    image_size = image_header[2];

    printk(KERN_INFO, "\n[IMG-I=%lx]\n", image_header[0]);
    printk(KERN_INFO, "[IMG-O=%lx]\n", text_offset);
    printk(KERN_INFO, "[IMG-S=%lx]\n", image_size);
    printk(KERN_INFO, "[IMG-F=%lx]\n", image_header[3]);
    printk(KERN_INFO, "[IMG-MAGIC=%08x]\n",
           *(volatile unsigned int *)(CONFIG_PAYLOAD_ENTRY + 0x38UL));
    printk(KERN_INFO, "[ADDR D=%p K=%p P=%p R=%p]\n",
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

    printk(KERN_INFO, "[KTEST size=%lx first=%08x last=%08x]\n",
           kernel_len,
           *(volatile unsigned int *)CONFIG_PAYLOAD_ENTRY,
           *(volatile unsigned int *)
           (CONFIG_PAYLOAD_ENTRY + kernel_len - sizeof(unsigned int)));

    if (*(volatile unsigned int *)(CONFIG_PAYLOAD_ENTRY + 0x38UL)
            != ARM64_IMAGE_MAGIC ||
        text_offset > CONFIG_PAYLOAD_ENTRY ||
        ((CONFIG_PAYLOAD_ENTRY - text_offset) & 0x001fffffUL) != 0 ||
        image_size == 0 ||
        image_size > CONFIG_RAMDISK_ENTRY - CONFIG_PAYLOAD_ENTRY) {
        printk(KERN_ERR, "\n[IMAGE-HEADER-OR-RANGE-BAD]\n");
        for (;;)
            ;
    }

    if ((sctlr_el1 & 1UL) != 0 || (current_el >> 2) != 1) {
        printk(KERN_ERR, "\n[EL1-MMU-STATE-BAD]\n");
        for (;;)
            ;
    }

    printk(KERN_INFO, "\n[CLEAN-START]\n");
    clean_to_poc(CONFIG_PAYLOAD_ENTRY, kernel_len);
    clean_to_poc(GTS4L_DTB_ENTRY, dtb_size);
#ifndef CONFIG_RAMDISK_NO_COPY
    clean_to_poc(CONFIG_RAMDISK_ENTRY, ramdisk_len);
#endif
    asm volatile("ic iallu\n dsb sy\n isb" : : : "memory");
    printk(KERN_INFO, "\n[REAL-JUMP]\n");
    printk(KERN_INFO, "\n[UL-11]\n");

    load_kernel_and_jump((void *)GTS4L_DTB_ENTRY, 0, 0, 0,
                         (void *)CONFIG_PAYLOAD_ENTRY,
                         (void *)(CONFIG_PAYLOAD_ENTRY + kernel_len));

    printk(KERN_ERR, "\n[UL-JUMP-RETURNED]\n");
    for (;;)
        ;
}
