GCCPARAMS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
LDPARAMS = -melf_i386

objects = loader.o gdt.o port.o kernel.o

all: mykernel.iso

%.o: %.cpp
	gcc $(GCCPARAMS) -o $@ -c $<

%.o: %.s
	gcc $(GCCPARAMS) -o $@ -c $<

mykernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)

install: mykernel.bin
	sudo cp $< /boot/mykernel.bin

mykernel.iso: mykernel.bin
	mkdir -p iso/boot/grub
	cp $< iso/boot/

	echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "My OS" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.bin' >> iso/boot/grub/grub.cfg
	echo '  boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg

	grub-mkrescue --output=$@ iso
	rm -rf iso

run: mykernel.iso
	# qemu-system-i386 -cdrom $<
	# qemu-system-i386 -cdrom $< -d cpu_reset
	# qemu-system-i386 -cdrom $< -boot d -display curses -m 64M
	qemu-system-i386 -cdrom $< -boot d -m 64M -vga std

.PHONY: clean

clean:
	rm -f $(objects) mykernel.bin mykernel.iso
	rm -rf iso
