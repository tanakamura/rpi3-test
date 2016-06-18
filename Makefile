all: kernel8.img

CROSS=aarch64-linux-gnu-

kernel8.img: kernel8.elf
	$(CROSS)objcopy -j .text -j .data -j .rodata -O binary $< $@

kernel8.elf: main.c
	$(CROSS)gcc -Ttext=0 -nostdlib -nostartfiles -o $@ $< -std=gnu99 -Wall -static

clean:
	rm -f kernel8.elf kernel8.img