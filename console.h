#ifndef CONSOLE_H
#define CONSOLE_H

struct console {
    int cur_x;
    int cur_y;
    int screen_char_width;
    int screen_char_height;
    int screen_width;
    int screen_height;
    int char_bbox_width;
    int char_bbox_height;
    unsigned char *frame_buffer;
};

void fatal(void);
void console_init(struct console *con,
                  unsigned char *frame_buffer,
                  int screen_width,
                  int screen_height,
                  int char_bbox_width,
                  int char_bbox_height);

void disp_char(struct console *con, char c);
void disp_chars(struct console *con, const char *str, int len);

extern struct console stdio_console;

#endif