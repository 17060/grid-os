# Prefer the osdev cross toolchain; fall back to host gcc/ld on Linux CI.
ifeq ($(shell command -v x86_64-elf-gcc 2>/dev/null),)
  CC      = gcc
  LD      = ld
  OBJCOPY = objcopy
  HOST_GCC = 1
else
  CC      = x86_64-elf-gcc
  LD      = x86_64-elf-ld
  OBJCOPY = x86_64-elf-objcopy
endif
NASM    = nasm
QEMU    = qemu-system-x86_64

CFLAGS  = -std=c11 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-red-zone \
          -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -O2 \
          -Ikernel/include
ifdef GRIDOS_VBE_4K
CFLAGS  += -DGRIDOS_VBE_4K=1
endif
ifdef HOST_GCC
  LDFLAGS += -no-pie
  USER_CFLAGS = -fno-pie
endif
ASFLAGS = -f elf64
LDFLAGS += -T linker.ld -nostdlib -z max-page-size=0x1000
# GridBASIC (kernel/basic.c) uses 64-bit fixed-point math with __int128
# intermediates for mul/div/sqrt. The 128-bit division helpers (__divti3,
# __udivti3, __modti3) live in libgcc, which is pure integer code with no
# FPU dependency. We pull in the cross-compiler's libgcc.a at link time.
LIBGCC := $(shell $(CC) -print-libgcc-file-name)

DISK_IMAGE = build/grid.img
DISK_TEST_IMAGE = build/grid-test.img
DISK_MB    = 128

BOOT_OBJS = build/boot.o build/gdt_load.o build/interrupts.o
USER_PROGS = gridprog discinfo gridsh lightcycle gridloop
USER_EMBED = $(USER_PROGS:%=build/%_embed.o)

KERNEL_OBJS = build/kernel.o build/console.o build/security.o build/iso.o \
               build/memory.o build/string.o build/gdt.o build/idt.o build/syscall.o \
               build/program.o build/serial.o build/disk.o build/pci.o \
               build/virtio_blk.o build/storage.o build/log.o build/gridfs.o \
               build/gfs.o build/elf.o build/ide.o build/mouse.o build/sched.o \
               build/timer.o build/link.o build/net.o build/dns.o build/tcp.o build/irc.o build/http.o \
               build/basic.o build/basic_pp.o build/basic_ide.o build/speaker.o \
               build/graphics.o build/recognizer.o build/disc.o build/pkg.o \
               $(VBE_OBJS) build/ai.o build/btc.o build/shell.o $(USER_EMBED)
TARGET = build/grid-os.bin

ifdef GRIDOS_VBE_4K
VBE_OBJS = build/vbe.o build/font8x16.o
else
VBE_OBJS =
endif

QEMU_MACHINE  = q35,acpi=off
QEMU_CPU      = qemu64
QEMU_RAM      = 128M
QEMU_DRIVE    = -drive if=none,id=grid0,file=$(DISK_IMAGE),format=raw
QEMU_DRIVE_TEST = -drive if=none,id=grid0,file=$(DISK_TEST_IMAGE),format=raw
QEMU_VIRTIO   = -device virtio-blk-pci,drive=grid0
QEMU_NET      = -netdev user,id=net0 -device virtio-net-pci,netdev=net0
QEMU_SERIAL   = -serial stdio
# zoom-to-fit lets the VGA text console scale when the window is resized (macOS cocoa)
ifeq ($(shell uname -s 2>/dev/null),Darwin)
  QEMU_DISPLAY  = -display cocoa,zoom-to-fit=on
  QEMU_DISPLAY_4K = -display cocoa,zoom-to-fit=on
else
  QEMU_DISPLAY  = -display gtk,zoom-to-fit=on
  QEMU_DISPLAY_4K = -display gtk,zoom-to-fit=on
endif
# HDMI 4K: EDID reports 3840x2160; cocoa zoom-to-fit scales 80x25 VGA text to fill
# the window. tools/qemu_hdmi_4k.sh also tries to resize the window on macOS.
QEMU_VGA_4K     = -device VGA,xres=3840,yres=2160,edid=on
QEMU_NAME_4K    = -name "Grid OS — HDMI 4K (3840x2160)"
QEMU_VGA_HD     = -device VGA,xres=1920,yres=1080,edid=on
QEMU_NAME_HD    = -name "Grid OS — HDMI HD (1920x1080)"
# -no-shutdown would make QEMU ignore isa-debug-exit, breaking `poweroff`.
QEMU_COMMON   = -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=0x04

.PHONY: all vbe-profile-check run run-hd run-4k run-vga run-headless run-legacy test test-host test-host-basic test-host-pp test-host-vault test-host-vault-disk test-host-tcp test-host-net test-host-spawn test-host-sched test-qemu-smoke test-e2e disk seed-disk gen-security-demos audit-security-demos gen-encyclopedia sync-basic-wiki install-prog ai-bridge btc-bridge https-bridge ws-bridge save-macos-arm64 standalone-macos release-mac save-windows-x64 standalone-windows release-windows save-termux standalone-termux release-termux save-linux-x64 standalone-linux release-linux clean

all: vbe-profile-check $(TARGET)

.PHONY: vbe-profile-check
vbe-profile-check:
	@mkdir -p build
	@if [ "$(GRIDOS_VBE_4K)" = "1" ]; then want=1; else want=0; fi; \
	 if [ ! -f build/.vbe_profile ] || [ "$$(cat build/.vbe_profile)" != "$$want" ]; then \
	   rm -f build/vbe.o build/font8x16.o build/kernel.o build/console.o build/shell.o build/grid-os.bin build/grid-os.elf; \
	   echo $$want > build/.vbe_profile; \
	 fi

build:
	mkdir -p build

$(DISK_IMAGE): | build
	dd if=/dev/zero of=$(DISK_IMAGE) bs=1M count=$(DISK_MB) status=none

disk: $(DISK_IMAGE)

gen-security-demos:
	python3 tools/gen_security_demos.py

audit-security-demos:
	python3 tools/audit_security_demos.py

gen-encyclopedia:
	python3 tools/gen_basic_encyclopedia.py

seed-disk: gen-security-demos gen-encyclopedia audit-security-demos $(TARGET) $(DISK_IMAGE)
	python3 tools/gfs_seed.py

sync-basic-wiki:
	python3 tools/sync_basic_wiki.py

PROG ?= gridsh
install-prog: $(DISK_IMAGE) build/$(PROG).elf
	python3 tools/gfs_install.py /programs/$(PROG) build/$(PROG).elf

$(DISK_TEST_IMAGE): seed-disk
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
	$(CC) $(CFLAGS) $(USER_CFLAGS) -Iuser -T user/linker.ld user/$(1).c -o $$@ -nostdlib -Wl,-e,_start $(if $(HOST_GCC),-no-pie,)

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

run-4k: $(DISK_IMAGE)
	rm -f build/vbe.o build/font8x16.o build/kernel.o build/console.o build/shell.o build/grid-os.bin build/grid-os.elf
	$(MAKE) GRIDOS_VBE_4K=1 $(TARGET)
	./tools/qemu_hdmi_4k.sh

run-hd: $(TARGET) $(DISK_IMAGE)
	./tools/qemu_hdmi_hd.sh

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

test-host-basic:
	@cc -std=c11 -Ikernel/include -O2 -o build/basic_host tools/basic_host_test.c kernel/basic.c kernel/basic_pp.c
	@printf '10 PRINT 7/2\n20 END\n' | build/basic_host | grep -qx '3.5'
	@printf '10 PRINT 1;\n20 END\n' | build/basic_host | grep -qx '1'
	@printf '10 A:=5\n20 PRINT A\n30 END\n' | build/basic_host | grep -qx '5'
	@printf '10 X$$=GRID.STATUS$$\n20 PRINT X$$\n30 END\n' | build/basic_host | grep -q '7.1.1'
	@printf '10 PRINT GRID.PING("gateway")\n20 END\n' | build/basic_host | grep -qx '1'
	@printf '10 CONST N=42\n20 PRINT N\n30 END\n' | build/basic_host | grep -qx '42'
	@printf '10 DATA 1,2,3\n20 READ A,B,C\n30 PRINT A+C\n40 END\n' | build/basic_host | grep -qx '4'
	@printf '10 RANDOMIZE 9999\n20 PRINT RND(100)\n30 END\n' | build/basic_host | grep -qx '14'
	@printf '10 PRINT INSTR$$("Grid OS","OS")\n20 END\n' | build/basic_host | grep -qx '6'
	@printf '10 PRINT TRIM$$("  hi  ")\n20 END\n' | build/basic_host | grep -qx 'hi'
	@printf '10 PRINT MIN(3,7)\n20 END\n' | build/basic_host | grep -qx '3'
	@printf '10 DEF FN DBL(X)=X*2\n20 PRINT DBL(5)\n30 END\n' | build/basic_host | grep -qx '10'
	@printf '10 SUB GREET(N$$)\n20 PRINT "Hello, "; N$$\n30 END SUB\n40 CALL GREET("Flynn")\n50 END\n' | build/basic_host | grep -qx 'Hello, Flynn'
	@printf '10 DIM M(3,3)\n20 M(2,3)=23\n30 PRINT M(2,3)\n40 END\n' | build/basic_host | grep -qx '23'
	@printf '10 IF 0 THEN PRINT 1 ELSEIF 1 THEN PRINT 2 ELSE PRINT 3\n20 END\n' | build/basic_host | grep -qx '2'
	@printf '10 PRINT ((((2*3))))\n20 END\n' | build/basic_host | grep -qx '6'
	@sh -c 'p=`awk "BEGIN{for(i=0;i<400;i++)printf\"(\"}"`; q=`awk "BEGIN{for(i=0;i<400;i++)printf\")\"}"`; printf "10 PRINT %s1%s\n20 END\n" "$$p" "$$q" | build/basic_host | grep -q "nesting too deep"'
	@printf '10 DIM A(65535,65535)\n20 A(2,0)=1\n30 END\n' | build/basic_host | grep -q 'too large'
	@printf '10 DIM A(2147483647)\n20 END\n' | build/basic_host | grep -q 'too large'
	@printf '10 WHILE 1\n20 NEXT\n30 END\n' | build/basic_host | grep -q 'without FOR'
	@printf '10 PRINT MID$$("hi",2000000000)\n20 PRINT "end"\n30 END\n' | build/basic_host | grep -qx 'end'
	@printf '10 PRINT 1E2000000000\n20 END\n' | build/basic_host | grep -qx '1000000000000'

test-host-vault:
	@cc -std=c11 -Ikernel/include -O2 -o build/vault_host tools/vault_host_test.c
	@build/vault_host

test-host-vault-disk: $(DISK_IMAGE)
	@cc -std=c11 -Ikernel/include -O2 -o build/vault_disk_host tools/vault_disk_host_test.c
	@build/vault_disk_host
	@cc -std=c11 -Ikernel/include -O2 -o build/v5load tools/vault_v5_disk_load_test.c
	@cp build/grid.img build/grid-test.img
	@python3 tools/prepare_vault_v5_disk.py build/grid-test.img
	@build/v5load build/grid-test.img

test-host-pp:
	@cc -std=c11 -Ikernel/include -O2 -o build/pp_host tools/pp_host_test.c kernel/basic_pp.c
	@build/pp_host

test-host-tcp:
	@cc -std=c11 -Ikernel/include -O2 -o build/tcp_host tools/tcp_host_test.c kernel/tcp.c
	@build/tcp_host

test-host-net:
	@cc -std=c11 -Ikernel/include -O2 -o build/net_host tools/net_host_test.c
	@build/net_host

test-host-spawn:
	@chmod +x tools/spawn_regression.sh
	@tools/spawn_regression.sh

test-host-sched:
	@cc -std=c11 -Ikernel/include -O2 -o build/sched_host tools/sched_host_test.c kernel/sched.c
	@build/sched_host

test-host: build test-host-basic test-host-pp test-host-vault test-host-vault-disk test-host-tcp test-host-net test-host-spawn test-host-sched

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
	  sleep 1; printf '10 PRINT "IDE-RUN-OK"\n20 END\n'; sleep 2; \
	  printf '\033'; sleep 2; printf ':run\n'; sleep 5; printf '\n'; \
	  sleep 2; printf '\033'; sleep 1; printf ':new\n'; sleep 1; printf '\n'; \
	  sleep 1; printf '10 PRINT "hello grid"\n20 END\n'; sleep 2; \
	  printf '\033'; sleep 2; printf ':run\n'; sleep 5; printf '\n'; \
	  sleep 1; printf '\033'; sleep 1; printf 'net ping gateway\n'; sleep 4; printf '\n'; \
	  sleep 1; printf '\033'; sleep 1; printf 'pkg mods\n'; sleep 3; printf '\n'; \
	  sleep 1; printf '\033'; sleep 1; printf 'spawn gridsh\n'; sleep 2; \
	  printf 'disc\n'; sleep 1; printf 'exit\n'; sleep 2; printf '\n'; \
	  sleep 1; printf '\033'; sleep 1; printf 'poweroff\n'; sleep 5) | \
	$(QEMU) -machine $(QEMU_MACHINE) -cpu $(QEMU_CPU) -m $(QEMU_RAM) \
		-kernel build/grid-os.elf $(QEMU_DRIVE_TEST) $(QEMU_VIRTIO) \
		$(QEMU_NET) $(QEMU_SERIAL) -display none $(QEMU_COMMON) \
		> build/test-e2e.log 2>&1; \
	rc=$$?; \
	if [ $$rc -ne 3 ]; then echo "expected debug-exit code 3, got $$rc"; exit 1; fi; \
	grep -q '=== GridBASIC run ===' build/test-e2e.log || { echo "IDE :run banner missing"; exit 1; }; \
	grep -q 'IDE-RUN-OK' build/test-e2e.log || { echo "IDE buffer :run output missing"; exit 1; }; \
	grep -Eq 'hello grid|GridBASIC 7\.(0|1\.1) online|grid line ' build/test-e2e.log || { echo "IDE buffer hello output missing"; exit 1; }; \
	grep -q 'OK15' build/test-e2e.log || { echo "basictest output missing"; exit 1; }; \
	grep -q 'Reply received' build/test-e2e.log || { echo "net ping output missing"; exit 1; }; \
	grep -q 'disc-status' build/test-e2e.log || { echo "pkg mods output missing"; exit 1; }; \
	grep -q 'http-probe' build/test-e2e.log || { echo "flynn-net-tools module missing"; exit 1; }; \
	grep -q 'Disc:' build/test-e2e.log || { echo "gridsh disc output missing"; exit 1; }; \
	grep -q 'End of line' build/test-e2e.log || { echo "gridsh clean exit missing"; exit 1; }; \
	if grep -q 'Program fault' build/test-e2e.log; then echo "unexpected program fault"; exit 1; fi; \
	grep -q 'Derezzing' build/test-e2e.log || { echo "poweroff banner missing"; exit 1; }; \
	echo "e2e OK"

test: test-host test-qemu-smoke test-e2e

ai-bridge:
	python3 tools/gridai_bridge.py

btc-bridge:
	python3 tools/gridbtc_bridge.py

https-bridge:
	python3 tools/gridhttps_bridge.py

save-macos-arm64: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_mac_silicon.sh
	./tools/save_mac_silicon.sh

standalone-macos: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/build_standalone_mac.sh
	./tools/build_standalone_mac.sh

release-mac: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_mac_silicon.sh tools/build_standalone_mac.sh
	GRID_OS_VERSION=v7.1.1 ./tools/save_mac_silicon.sh
	GRID_OS_VERSION=7.1.1 ./tools/build_standalone_mac.sh
	@echo "Upload dist/* to GitHub release v7.1.1 with: gh release upload v7.1.1 dist/*"

save-windows-x64: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_windows_x64.sh
	./tools/save_windows_x64.sh

standalone-windows: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/build_standalone_windows.sh
	./tools/build_standalone_windows.sh

release-windows: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_windows_x64.sh tools/build_standalone_windows.sh
	GRID_OS_VERSION=v7.1.1 ./tools/save_windows_x64.sh
	GRID_OS_VERSION=7.1.1 ./tools/build_standalone_windows.sh
	@echo "Upload dist/*-Windows-* to GitHub release with: gh release upload v7.1.1 dist/GridOS-*-Windows-x64.zip dist/grid-os-windows-x64-*.zip"

save-termux: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_termux.sh
	./tools/save_termux.sh

standalone-termux: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/build_standalone_termux.sh
	./tools/build_standalone_termux.sh

release-termux: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_termux.sh tools/build_standalone_termux.sh
	GRID_OS_VERSION=v7.1.1 ./tools/save_termux.sh
	GRID_OS_VERSION=7.1.1 ./tools/build_standalone_termux.sh
	@echo "Upload dist/*Android-Termux* to GitHub release with: gh release upload v7.1.1 dist/GridOS-*-Android-Termux.* dist/grid-os-android-termux-*.zip"

save-linux-x64: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_linux_x64.sh
	./tools/save_linux_x64.sh

standalone-linux: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/standalone_linux_stub.sh
	@echo "Run ./GridOS-Linux.sh from release bundle"

release-linux: $(TARGET) $(DISK_IMAGE)
	chmod +x tools/save_linux_x64.sh
	GRID_OS_VERSION=v7.1.1 ./tools/save_linux_x64.sh
	@echo "Upload dist/grid-os-linux-x64-* to GitHub release v7.1.1"

ws-bridge:
	python3 tools/gridws_bridge.py

clean:
	rm -rf build
