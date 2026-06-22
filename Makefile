CC = gcc
AS = as
LD = ld
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pie -no-pie -Wall -Wextra -I src/kernel
LDFLAGS = -m elf_i386 -T src/linker.ld
OBJS = boot.o kernel.o ramfs.o keyboard.o libc.o

all: check yayca.iso

boot.o: src/boot/boot.s
	$(AS) --32 src/boot/boot.s -o boot.o

kernel.o: src/kernel/kernel.c
	$(CC) $(CFLAGS) -c src/kernel/kernel.c -o kernel.o

ramfs.o: src/kernel/ramfs.c
	$(CC) $(CFLAGS) -c src/kernel/ramfs.c -o ramfs.o

keyboard.o: src/kernel/keyboard.c
	$(CC) $(CFLAGS) -c src/kernel/keyboard.c -o keyboard.o

libc.o: src/kernel/libc.c
	$(CC) $(CFLAGS) -c src/kernel/libc.c -o libc.o

kernel.bin: $(OBJS) src/linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o kernel.bin

check: kernel.bin
	@echo "=== PROVERKA MULTIBOOT ==="
	@grub2-file --is-x86-multiboot kernel.bin && echo "MULTIBOOT OK" || (echo "MULTIBOOT HUY"; exit 1)
	@file kernel.bin

yayca.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "HeavyYaycaOS" { multiboot /boot/kernel.bin }' >> iso/boot/grub/grub.cfg
	/usr/sbin/grub2-mkrescue -o yayca.iso iso

clean:
	rm -rf *.o *.bin *.iso iso

qemu: yayca.iso
	qemu-system-i386 -cdrom yayca.iso -m 256M -vga std

qemu-kernel: kernel.bin
	qemu-system-i386 -kernel kernel.bin -m 256M -vga std
