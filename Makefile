all: kernel8.img

CROSS=aarch64-linux-gnu-

kernel8.img: kernel8.elf
	$(CROSS)objcopy -j .text -j .data -j .rodata -O binary $< $@

OBJS=main.o console.o npr/printf.o libc/stdio.o libc/string.o libc/strlen.o npr/printf-format.o
kernel8.elf: $(OBJS)
	$(CROSS)gcc -Ttext=0x0 -nostdlib -nostartfiles -o $@ $^ -std=gnu99 -Wall -static -Wl,-Map,kernel8.map

CFLAGS=-Inpr -Ilibc -O2 -fno-builtin

%.o: %.c
	$(CROSS)gcc -c $(CFLAGS) -nostdlib -nostartfiles -o $@ $< -std=gnu99 -Wall


clean:
	rm -f kernel8.elf kernel8.img *~