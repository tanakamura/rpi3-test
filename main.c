
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <arm_neon.h>
#include <string.h>
#include "npr/printf-format.h"
#include "arm.h"
#include "console.h"


#define BUFFER_SIZE (2048*1024)
#define PERIPHERAL_BASE 0x3f000000
#define MBOX_BASE (unsigned char*)(PERIPHERAL_BASE | 0xb880)
#define TIMER_BASE ((unsigned char*)0x3f003000)

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1200

#define MBOX_OFFSET_READ 0
#define MBOX_OFFSET_CONFIG 0x1c
#define MBOX_OFFSET_STATUS 0x18
#define MBOX_OFFSET_WRITE 0x20
#define MBOX_OFFSET_STATUS1 0x38
#define MBOX_CH_ARM_TO_VC 8

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

#define CHAR_BBOX_HEIGHT 9
#define CHAR_BBOX_WIDTH 9

asm(".globl _start\n\t"
    "_start:\n\t"
    "mrs x0, MPIDR_EL1\n\t"
    "and x0, x0, 3\n\t"
    "cbnz x0, other_core\n\t"
    "adrp x0, stack+16*1024\n\t"
    "add x0, x0, :lo12:stack+16*1024\n\t"
    "mov sp, x0\n\t"
    "b start\n\t"
    "other_core: b .\n\t");

#define get_pc(v) asm volatile("adr %0, .":"=r"(v))

#define ALLOCATE_BUFFER_TAG 0x00040001
extern uint32x4_t buffer[];

static uint32_t __attribute__((aligned(16)))
fb_struct[] = {
    0,                          /* 0 size */
    0,                          /* 1 req/response */

    0x00048003,                 /* set physical display */
    8,
    8,
    SCREEN_WIDTH,               /* width */
    SCREEN_HEIGHT,              /* height */

    0x00048004,                 /* set virtual buffer */
    8,                          /* */
    8,                          /* */
    SCREEN_WIDTH,               /* width */
    SCREEN_HEIGHT,              /* height */

    0x00048009,                 /* set virtual offset */
    8,
    8,
    0,
    0,

    0x00048005,                 /* set depth */
    4,
    4,
    8,

    0x0004800b,                 /* set palette */
    16,
    16,
    0,                          /* off = 0 */
    2,                          /* 2color */
    0,                          /* black */
    0xffffffff,                 /* white */


    ALLOCATE_BUFFER_TAG,        /* allocate buffer */
    8,                          /* size */
    8,                          /* req */
    0,                          /* pointer[0] */
    0,                          /* pointer[1] */

    0,                          /* end tag */
};

#define GET_ARM_MEMORY_TAG 0x00010005
#define GET_VC_MEMORY_TAG 0x00010006
#define GET_CLOCK_RATE 0x00030002

static uint32_t __attribute__((aligned(16)))
firmware_info[] = {
    0,                          /* size */
    0,                          /* req/res */

    GET_CLOCK_RATE,
    4,
    4,                          /* req */
    3,                          /* arm */

    GET_CLOCK_RATE,
    8,
    8 | 0x80000000,             /* res */
    3,                          /* clkid */
    0,                          /* clock */

    GET_ARM_MEMORY_TAG,                 /* get arm memory */
    8,                          /* size */
    8 | 0x80000000,
    0,
    0,

    GET_VC_MEMORY_TAG,                 /* get vc memory */
    8,
    8 | 0x80000000,
    0,
    0,

    0,                          /* end */
};

static void
wait_write(int ch)
{
    while(1) {
        uint32_t st = read32(MBOX_BASE + MBOX_OFFSET_STATUS1);
        if (st & 0x80000000) {  /* FULL */
            continue;
        }
        break;
    }
}

static void
write_to_mbox(int ch, uint32_t val)
{
    wait_write(ch);
    write32(MBOX_BASE + MBOX_OFFSET_WRITE, val | ch);
    wait_write(ch);
}

static void
wait_while_zero(uint32_t *p)
{
    while(1) {
        uint32_t v = *(volatile uint32_t *)p;
        if (v != 0) {
            return;
        }
    }
}

static int
find_tag(uint32_t *a, int n, uint32_t tag)
{
    int ret = 0;
    for (int i=0; i<n; i++) {
        if (a[i] == tag) {
            ret = i;
        }
    }

    return ret;
}


void start()
{
    int fb_off = find_tag(fb_struct,
                          sizeof(fb_struct)/sizeof(fb_struct[0]),
                          ALLOCATE_BUFFER_TAG);
    fb_off += 3;

    fb_struct[0] = sizeof(fb_struct);
    write_to_mbox(MBOX_CH_ARM_TO_VC, (uint32_t)(uintptr_t)fb_struct);

    wait_while_zero(&fb_struct[fb_off]);

    uintptr_t frame_buffer_addr0 = *(fb_struct + fb_off);
    uintptr_t frame_buffer_addr = frame_buffer_addr0 & 0x3fffffff;
    unsigned char *frame_buffer = (unsigned char*)frame_buffer_addr;
    uintptr_t pc;

    console_init(&stdio_console, frame_buffer,
                 SCREEN_WIDTH, SCREEN_HEIGHT,
                 CHAR_BBOX_WIDTH, CHAR_BBOX_HEIGHT);

    printf("frame_buffer = %p, %p\n", frame_buffer_addr, frame_buffer_addr0);
    get_pc(pc);
    printf("pc = %p\n", pc);

    firmware_info[0] = sizeof(firmware_info);

    int nelem = sizeof(firmware_info)/sizeof(firmware_info[0]);
    int arm_mem = find_tag(firmware_info, nelem, GET_ARM_MEMORY_TAG);
    int vc_mem = find_tag(firmware_info, nelem, GET_VC_MEMORY_TAG);
    int clk_rate = find_tag(firmware_info, nelem, GET_CLOCK_RATE);

    arm_mem += 3;
    vc_mem += 3;
    clk_rate += 3;

    write_to_mbox(MBOX_CH_ARM_TO_VC, (uint32_t)(uintptr_t)firmware_info);
    wait_while_zero(&firmware_info[vc_mem]);

    printf("arm mem : base=%08x, size=%08x\n",
           firmware_info[arm_mem],
           firmware_info[arm_mem+1]);
    printf("vc mem : base=%08x, size=%08x\n",
           firmware_info[vc_mem],
           firmware_info[vc_mem+1]);
    printf("arm clk : clkid=%d, clk=%d, %d\n",
           firmware_info[clk_rate],
           firmware_info[clk_rate+1], clk_rate);

    printf("el = %llx\n",get_currentel());
    printf("cntfrq = %llx\n",get_cntfrq_el0());
    printf("cntpct = %llx\n",get_cntpct_el0());
    uint64_t clk = firmware_info[clk_rate+1];
    clk = 10000;

    {
        struct console *con = &stdio_console;
        unsigned char *dst = con->frame_buffer;
        unsigned char *src = con->frame_buffer + con->char_bbox_height * con->screen_width;
        size_t len = con->screen_width * (con->screen_height - con->char_bbox_height);

        ptrdiff_t dptr = dst - src;
        unsigned char *d = dst;
        const unsigned char *s = src;
        intptr_t i;

        if (dptr > 0) {
            for (i=len-1; i>=0; i--) {
                d[i] = s[i];
            }
        } else {
            uintptr_t d_addr = (uintptr_t)d;
            uintptr_t s_addr = (uintptr_t)s;

            printf("%p %p %08x %08x\n",
                   d_addr, s_addr, (int)len, (int)dptr);

            if (((d_addr & 15) == 0) &&
                ((s_addr & 15) == 0) &&
                ((len & 15) == 0) &&
                ((dptr <= -16)))
            {
                puts("xx");
            } else {
                puts("yy");
            }
        }
    }

    uint64_t t0 = clk;

    while (1) {
        uint64_t now = get_cntpct_el0();
        if (now >= t0) {
            uint32_t tv = read32(TIMER_BASE + 0x4); /* CLO */

            printf("now = %d, tv=%d\n", now, tv);

            t0 += clk;
        }

        asm volatile ("" ::: "memory");

    }


    asm volatile ("" ::: "memory");

    while (1)
        ;
}

char stack[16384] __attribute__((aligned(64)));
unsigned char buffer[BUFFER_SIZE] __attribute__((aligned(64)));