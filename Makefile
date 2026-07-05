CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
NASM    = nasm
QEMU    = qemu-system-x86_64

CFLAGS  = -std=c11 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
          -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -O2 \
          -Ikernel/include
ASFLAGS = -f elf64
LDFLAGS = -T linker.ld -nostdlib -z max-page-size=0x1000
# GridBASIC (kernel/basic.c) uses 64-bit fixed-point math with __int128
# intermediates for mul/div/sqrt. The 128-bit division helpers (__divti3,
# __udivti3, __modti3) live in libgcc, which is pure integer code with no
# FPU dependency. We pull in the cross-compiler's libgcc.a at link time.
LIBGCC := $(shell $(CC) -print-libgcc-file-name)

DISK_IMAGE = build/grid.img
DISK_TEST_IMAGE = build/grid-test.img
DISK_MB    = 16

BOOT_OBJS = build/boot.o build/gdt_load.o build/interrupts.o
USER_PROGS = gridprog discinfo gridsh lightcycle gridloop
USER_EMBED = $(USER_PROGS:%=build/%_embed.o)

KERNEL_OBJS = build/kernel.o build/console.o build/security.o build/iso.o \
               build/memory.o build/gdt.o build/idt.o build/syscall.o \
               build/program.o build/serial.o build/disk.o build/pci.o \
               build/virtio_blk.o build/storage.o build/log.o build/gridfs.o \
               build/gfs.o build/elf.o build/ide.o build/mouse.o build/sched.o \
               build/timer.o build/link.o build/net.o build/tcp.o build/irc.o \
               build/basic.o build/basic_ide.o build/ai.o build/shell.o $(USER_EMBED)
TARGET = build/grid-os.bin

QEMU_MACHINE  = q35,acpi=off
QEMU_CPU      = qemu64
QEMU_RAM      = 128M
QEMU_DRIVE    = -drive if=none,id=grid0,file=$(DISK_IMAGE),format=raw
QEMU_DRIVE_TEST = -drive if=none,id=grid0,file=$(DISK_TEST_IMAGE),format=raw
QEMU_VIRTIO   = -device virtio-blk-pci,drive=grid0
QEMU_NET      = -netdev user,id=net0 -device virtio-net-pci,netdev=net0
QEMU_SERIAL   = -serial stdio
# zoom-to-fit lets the VGA text console scale when the window is resized
QEMU_DISPLAY  = -display cocoa,zoom-to-fit=on
# -no-shutdown would make QEMU ignore isa-debug-exit, breaking `poweroff`.
QEMU_COMMON   = -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=0x04

.PHONY: all run run-vga run-headless run-legacy test test-host test-qemu-smoke test-e2e disk seed-disk install-prog ai-bridge clean

all: $(TARGET)

build:
	mkdir -p build

$(DISK_IMAGE): | build
	dd if=/dev/zero of=$(DISK_IMAGE) bs=1M count=$(DISK_MB) status=none

disk: $(DISK_IMAGE)

seed-disk: $(TARGET) $(DISK_IMAGE)
	python3 tools/gfs_seed.py

PROG ?= gridsh
install-prog: $(DISK_IMAGE) build/$(PROG).elf
	python3 tools/gfs_install.py /programs/$(PROG) build/$(PROG).elf

$(DISK_TEST_IMAGE): $(DISK_IMAGE) | build
	cp $(DISK_IMAGE) $(DISK_TEST_IMAGE)

build/boot.o: boot/boot.s | build
	$(NASM) $(ASFLAGS) boot/boot.s -o $@

build/gdt_load.o: boot/gdt_load.s | build
	$(NASM) $(ASFLAGS) boot/gdt_load.s -o $@

build/interrupts.o: boot/interrupts.s | build
	$(NASM) $(ASFLAGS) boot/interrupts.s -o $@

build/%.o: kernel/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# GridBASIC uses pure 64-bit fixed-point arithmetic (no hardware FP at all),
# so basic.c/basic_ide.c build with the standard kernel CFLAGS (-mno-sse).
# -O2 is safe now that no doubles/XMM are involved.
build/basic.o: kernel/basic.c | build
	$(CC) $(CFLAGS) -c $< -o $@
build/basic_ide.o: kernel/basic_ide.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# serial.c and console.c string-output loops miscompile at -O2 (GCC inlines
# UART/mirror helpers and walks past terminating NUL into .rodata, spewing
# megabytes on COM1). Keep these at -O0.
build/serial.o: kernel/serial.c | build
	$(CC) $(CFLAGS) -O0 -c $< -o $@
build/console.o: kernel/console.c | build
	$(CC) $(CFLAGS) -O0 -c $< -o $@

define USER_PROG_RULE
build/$(1).elf: user/$(1).c user/linker.ld user/usys.h | build
	$(CC) $(CFLAGS) -Iuser -T user/linker.ld user/$(1).c -o $$@ -nostdlib -Wl,-e,_start

build/$(1).bin: build/$(1).elf
	$(OBJCOPY) -O binary $$< $$@

build/$(1)_embed.o: user/$(1)_embed.s build/$(1).bin
	$(NASM) $(ASFLAGS) user/$(1)_embed.s -o $$@
endef

$(foreach prog,$(USER_PROGS),$(eval $(call USER_PROG_RULE,$(prog))))

$(TARGET): $(BOOT_OBJS) $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $(BOOT_OBJS) $(KERNEL_OBJS) $(LIBGCC) -o build/grid-os.elf
	$(OBJCOPY) -O binary build/grid-os.elf $@

run: $(TARGET) $(DISK_IMAGE)
	$(QEMU) -machine $(QEMU_MACHINE) -cpu $(QEMU_CPU) -m $(QEMU_RAM) \
		-kernel build/grid-os.elf $(QEMU_DRIVE) $(QEMU_VIRTIO) \
		$(QEMU_NET) $(QEMU_SERIAL) $(QEMU_DISPLAY) $(QEMU_COMMON)

run-vga: $(TARGET) $(DISK_IMAGE)
	$(QEMU) -machine $(QEMU_MACHINE) -cpu $(QEMU_CPU) -m $(QEMU_RAM) \
		-kernel build/grid-os.elf $(QEMU_DRIVE) $(QEMU_VIRTIO) \
		$(QEMU_NET) $(QEMU_DISPLAY) $(QEMU_COMMON)

run-headless: $(TARGET) $(DISK_IMAGE)
	$(QEMU) -machine $(QEMU_MACHINE) -cpu $(QEMU_CPU) -m $(QEMU_RAM) \
		-kernel build/grid-os.elf $(QEMU_DRIVE) $(QEMU_VIRTIO) \
		$(QEMU_NET) $(QEMU_SERIAL) -display none $(QEMU_COMMON)

run-legacy: $(TARGET) $(DISK_IMAGE)
	$(QEMU) -kernel build/grid-os.elf -drive file=$(DISK_IMAGE),if=ide,format=raw \
		$(QEMU_SERIAL) -no-reboot -no-shutdown

test-host:
	@cc -std=c11 -Ikernel/include -O2 -o build/basic_host tools/basic_host_test.c kernel/basic.c
	@printf '10 PRINT 7/2\n20 END\n' | build/basic_host | grep -qx '3.5'

test-qemu-smoke: $(TARGET) $(DISK_TEST_IMAGE)
	@$(QEMU) -machine $(QEMU_MACHINE) -cpu $(QEMU_CPU) -m $(QEMU_RAM) \
		-kernel build/grid-os.elf $(QEMU_DRIVE_TEST) $(QEMU_VIRTIO) \
		$(QEMU_NET) -display none -serial none $(QEMU_COMMON) & \
	e=$$!; \
	sleep 8; \
	if kill -0 $$e 2>/dev/null; then kill $$e 2>/dev/null; wait $$e 2>/dev/null || true; else echo "QEMU died during smoke test"; exit 1; fi

# Full end-to-end: boot into the GridBASIC IDE, open the grid> prompt over
# serial, run the interpreter self-test, spawn the ring-3 gridsh sandbox and
# exit it cleanly, then poweroff (isa-debug-exit -> 3).
test-e2e: $(TARGET) $(DISK_TEST_IMAGE)
	@(sleep 5; printf '\033'; sleep 1; printf 'basictest\n'; sleep 3; printf '\n'; \
	  sleep 1; printf '\033'; sleep 1; printf 'spawn gridsh\n'; sleep 2; \
	  printf 'disc\n'; sleep 1; printf 'exit\n'; sleep 2; printf '\n'; \
	  sleep 1; printf '\033'; sleep 1; printf 'poweroff\n'; sleep 5) | \
	$(QEMU) -machine $(QEMU_MACHINE) -cpu $(QEMU_CPU) -m $(QEMU_RAM) \
		-kernel build/grid-os.elf $(QEMU_DRIVE_TEST) $(QEMU_VIRTIO) \
		$(QEMU_NET) $(QEMU_SERIAL) -display none $(QEMU_COMMON) \
		> build/test-e2e.log 2>&1; \
	rc=$$?; \
	if [ $$rc -ne 3 ]; then echo "expected debug-exit code 3, got $$rc"; exit 1; fi; \
	grep -q 'OK15' build/test-e2e.log || { echo "basictest output missing"; exit 1; }; \
	grep -q 'Disc:' build/test-e2e.log || { echo "gridsh disc output missing"; exit 1; }; \
	grep -q 'End of line' build/test-e2e.log || { echo "gridsh clean exit missing"; exit 1; }; \
	if grep -q 'Program fault' build/test-e2e.log; then echo "unexpected program fault"; exit 1; fi; \
	grep -q 'Derezzing' build/test-e2e.log || { echo "poweroff banner missing"; exit 1; }; \
	echo "e2e OK"

test: test-host test-qemu-smoke test-e2e

ai-bridge:
	python3 tools/gridai_bridge.py

clean:
	rm -rf build
