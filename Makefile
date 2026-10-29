PROJECT := NovaOS
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso-root
KERNEL := $(BUILD_DIR)/$(PROJECT).elf
ISO := $(BUILD_DIR)/$(PROJECT).iso

CC = clang
LD = ld
HOST_CC ?= cc

LIMINE_VERSION := 12.9.6
LIMINE_ARCHIVE := $(BUILD_DIR)/limine-binary.tar.gz
LIMINE_DIR := $(BUILD_DIR)/limine-binary
LIMINE_TOOL := $(BUILD_DIR)/limine
LIMINE_URL := https://github.com/Limine-Bootloader/Limine/releases/download/v$(LIMINE_VERSION)/limine-binary.tar.gz

CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror \
           -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
           -fno-asynchronous-unwind-tables -mno-red-zone -mno-sse \
           -mno-mmx -mno-80387 -mcmodel=kernel -m64 -Iinclude
LDFLAGS := -nostdlib -static -m elf_x86_64 -T linker.ld -z max-page-size=0x1000

.PHONY: all kernel iso run debug clean

all: iso

kernel: $(KERNEL)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/main.o: kernel/main.c include/limine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(BUILD_DIR)/main.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/main.o

$(LIMINE_DIR)/limine.c: $(LIMINE_ARCHIVE)
	tar -xzf $< -C $(BUILD_DIR)

$(LIMINE_ARCHIVE): | $(BUILD_DIR)
	command -v curl >/dev/null || { echo "error: curl is required to fetch Limine" >&2; exit 1; }
	curl -fL --retry 3 $(LIMINE_URL) -o $@

$(LIMINE_TOOL): $(LIMINE_DIR)/limine.c
	$(HOST_CC) -std=c99 -O2 -D_FILE_OFFSET_BITS=64 $< -o $@

$(ISO): $(KERNEL) limine.conf $(LIMINE_TOOL)
	command -v xorriso >/dev/null || { echo "error: xorriso is required for ISO creation" >&2; exit 1; }
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot $(ISO_ROOT)/EFI/BOOT
	cp $(KERNEL) $(ISO_ROOT)/boot/$(PROJECT).elf
	cp limine.conf $(ISO_ROOT)/boot/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_ROOT)/boot/limine-bios.sys
	cp $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_ROOT)/boot/limine-bios-cd.bin
	cp $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine-uefi-cd.bin
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/BOOTX64.EFI
	xorriso -as mkisofs -R -r -J \
		-b boot/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 \
		-boot-info-table -hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine-uefi-cd.bin -efi-boot-part \
		--efi-boot-image --protective-msdos-label $(ISO_ROOT) -o $@
	$(LIMINE_TOOL) bios-install $@

iso: $(ISO)

run: $(ISO)
	command -v qemu-system-x86_64 >/dev/null || { echo "error: qemu-system-x86_64 is required" >&2; exit 1; }
	qemu-system-x86_64 -cdrom $(ISO)

debug: $(ISO)
	command -v qemu-system-x86_64 >/dev/null || { echo "error: qemu-system-x86_64 is required" >&2; exit 1; }
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio -s -S

clean:
	rm -rf $(BUILD_DIR)