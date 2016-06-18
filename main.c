
#include <stdint.h>
#include <stdio.h>

#define PERIPHERAL_BASE 0x3f000000
#define MBOX_BASE (unsigned char*)(PERIPHERAL_BASE | 0xb880)

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define MBOX_OFFSET_READ 0
#define MBOX_OFFSET_CONFIG 0x1c
#define MBOX_OFFSET_STATUS 0x18
#define MBOX_OFFSET_WRITE 0x20
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
    "mov x0, stack + 4096\n\t"
    "mov sp, x0\n\t"
    "b start\n\t"
    "other_core: b .\n\t");

extern unsigned char font8x8[];

struct console {
    int cur_x;
    int cur_y;
    int screen_char_width;
    int screen_char_height;
    unsigned char *frame_buffer;
};

static void console_init(struct console *con,
                         unsigned char *frame_buffer) {
    con->cur_x = 0;
    con->cur_y = 0;

    con->screen_char_width = SCREEN_WIDTH / CHAR_BBOX_WIDTH;
    con->screen_char_height = SCREEN_HEIGHT / CHAR_BBOX_HEIGHT;

    con->frame_buffer = frame_buffer;
}


static void
disp_char(struct console *con, char c)
{
    if (c == '\n') {
        con->cur_x = 0;
        con->cur_y++;
        return;
    }

    unsigned char *f = &font8x8[c*8];

    int xpos = con->cur_x * CHAR_BBOX_WIDTH;
    int ypos = con->cur_y * CHAR_BBOX_WIDTH;

    for (int yi=0; yi<FONT_HEIGHT; yi++) {
        unsigned char fc = f[yi];
        unsigned char *out = con->frame_buffer + SCREEN_WIDTH * (yi+ypos) + xpos;
        for (int xi=0; xi<FONT_WIDTH; xi++) {
            if (fc & (1<<(7-xi))) {
                out[xi] = 1;
            } else {
                out[xi] = 0;
            }
        }

        out[8] = 0;
    }

    con->cur_x ++;
    if (con->cur_x == con->screen_char_width) {
        con->cur_x = 0;
        con->cur_y ++;
    }
}

static void
disp_str(struct console *con, const char *str)
{
    while (*str) {
        disp_char(con, *str);
        str++;
    }
}


static uint32_t read32(unsigned char *p) {
    return *(volatile uint32_t*)p;
}

static void write32(unsigned char *p, uint32_t val) {
    *(volatile uint32_t*)p = val;
}

#define ALLOCATE_BUFFER_TAG 0x00040001

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

static void
write_to_mbox(int ch, uint32_t val)
{
    while(1) {
        uint32_t st = read32(MBOX_BASE + MBOX_OFFSET_STATUS);
        if (st & 0x80000000) {  /* FULL */
            continue;
        }
        break;
    }

    write32(MBOX_BASE + MBOX_OFFSET_WRITE, val | ch);
}

void start()
{
    int fb_off;

    for (int i=0; i<sizeof(fb_struct)/sizeof(fb_struct[0]); i++) {
        if (fb_struct[i] == ALLOCATE_BUFFER_TAG) {
            fb_off = i + 3;
            break;
        }
    }

    fb_struct[0] = sizeof(fb_struct);
    write_to_mbox(MBOX_CH_ARM_TO_VC, (uint32_t)(uintptr_t)fb_struct);

    while(1) {
        uint32_t ptr = *(volatile uint32_t *)(&fb_struct[fb_off]);
        if (ptr != 0) {
            break;
        }
    }

    uintptr_t frame_buffer_addr = *(fb_struct + fb_off);
    frame_buffer_addr &= 0x3fffffff;
    unsigned char *frame_buffer = (unsigned char*)frame_buffer_addr;

    struct console con;
    console_init(&con, frame_buffer);

    disp_str(&con, "Hello World!\n");
    disp_str(&con, "This is a pen.");

    asm volatile ("" ::: "memory");

    while (1)
        ;
}

char stack[4096] __attribute__((aligned(64)));

asm(".include \"fonts.s\"\n\t");
