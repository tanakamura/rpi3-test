#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "npr/printf-format.h"
#include "console.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

struct console stdio_console;

void fatal(void) {
    while (1);
}

#define BUFSIZE 256

int
vprintf(const char *format,
	va_list ap)
{
	char buffer[BUFSIZE];
	struct npr_printf_format fmt[32];
	struct npr_printf_arg args[32];
	int num_fmt;
	struct npr_printf_state st;
	int is_fini, out_size;
	struct npr_printf_build_format_error error;

	num_fmt = npr_printf_build_format(fmt, 32, format, strlen(format), &error);

	if (num_fmt < 0) {
		switch (error.code) {
		case NPR_PRINTF_BUILD_FORMAT_TOO_MANY_FORMATS:
			puts("too many format for printf");
			break;

		case NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT:
			printf("printf format error @ %c(%d)(%s)\n",
			       format[error.idx],
			       error.idx,
			       format);
			break;
		}
		fatal();
	} else if (num_fmt >= 32) {
		fatal();
	}
	npr_printf_build_varg(args, fmt, num_fmt, ap);

	npr_sprintf_start(&st);
	out_size = npr_sprintf(&st, buffer, BUFSIZE, fmt, num_fmt, args, &is_fini);

	if (! is_fini) {
		puts("printf error");
		return -1;
	}

        disp_chars(&stdio_console, buffer, out_size);

	return out_size;
}


int puts(const char *s)
{
    size_t len = strlen(s);
    disp_chars(&stdio_console, s, len);
    disp_char(&stdio_console, '\n');

    return len;
}

void console_init(struct console *con,
                  unsigned char *frame_buffer,
                  int screen_width,
                  int screen_height,
                  int char_bbox_width,
                  int char_bbox_height)
{
    con->cur_x = 0;
    con->cur_y = 0;

    con->screen_char_width = screen_width / char_bbox_width;
    con->screen_char_height = screen_height / char_bbox_height;

    con->frame_buffer = frame_buffer;

    con->screen_width = screen_width;
    con->screen_height = screen_height;
    con->char_bbox_width = char_bbox_width;
    con->char_bbox_height = char_bbox_height;
}

extern unsigned char font8x8[];

static void
next_line(struct console *con)
{
        con->cur_x = 0;
        if (con->cur_y == con->screen_char_height-1) {
            unsigned char *dst = con->frame_buffer;
            unsigned char *src = con->frame_buffer + con->char_bbox_height * con->screen_width;

            memmove(dst, src, con->screen_width * (con->screen_height - con->char_bbox_height));
        } else {
            con->cur_y++;
        }
}

void
disp_char(struct console *con, char c)
{
    if (c == '\n') {
        next_line(con);
        return;
    }

    unsigned char *f = &font8x8[c*8];

    int xpos = con->cur_x * con->char_bbox_width;
    int ypos = con->cur_y * con->char_bbox_height;

    for (int yi=0; yi<FONT_HEIGHT; yi++) {
        unsigned char fc = f[yi];
        unsigned char *out = con->frame_buffer + con->screen_width * (yi+ypos) + xpos;
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
        next_line(con);
    }
}

void
disp_chars(struct console *con, const char *str, int len)
{
    for (int i=0; i<len; i++) {
        disp_char(con, str[i]);
    }
}

asm(".include \"fonts.s\"\n\t");
