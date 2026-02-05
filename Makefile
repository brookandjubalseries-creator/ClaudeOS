# ClaudeOS Makefile
# Build system for the MultiClaude OS project

# Toolchain (i686-elf cross-compiler for proper ELF output)
TOOLCHAIN = /c/Users/Purge/Desktop/i686-elf-tools/bin
CC = $(TOOLCHAIN)/i686-elf-gcc.exe
LD = $(TOOLCHAIN)/i686-elf-ld.exe
AS = /c/Users/Purge/Desktop/bios_message/nasm-2.16.03/nasm

# Flags (i686-elf-gcc is already 32-bit, no -m32 needed)
CFLAGS = -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin \
         -Wall -Wextra -Iinclude
ASFLAGS = -f elf32
# Linker flags for i686-elf toolchain
LIBGCC = /c/Users/Purge/Desktop/i686-elf-tools/lib/gcc/i686-elf/7.1.0/libgcc.a
LDFLAGS = -T linker.ld -nostdlib

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
SHELL_DIR = shell
FS_DIR = fs
DRIVER_DIR = drivers

# Output
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
KERNEL_BIN = $(BUILD_DIR)/claudeos.bin
ISO_FILE = $(BUILD_DIR)/claudeos.iso

# Source files (add as they're created)
# Only include boot.asm, not demo.asm (which is a raw boot sector)
BOOT_SRC = $(BOOT_DIR)/boot.asm
KERNEL_C_SRC = $(wildcard $(KERNEL_DIR)/*.c)
KERNEL_ASM_SRC = $(wildcard $(KERNEL_DIR)/*.asm)
SHELL_SRC = $(wildcard $(SHELL_DIR)/*.c)
FS_SRC = $(wildcard $(FS_DIR)/*.c)
DRIVER_SRC = $(wildcard $(DRIVER_DIR)/*.c)

# Object files
BOOT_OBJ = $(BOOT_SRC:$(BOOT_DIR)/%.asm=$(BUILD_DIR)/boot_%.o)
KERNEL_C_OBJ = $(KERNEL_C_SRC:$(KERNEL_DIR)/%.c=$(BUILD_DIR)/kernel_%.o)
KERNEL_ASM_OBJ = $(KERNEL_ASM_SRC:$(KERNEL_DIR)/%.asm=$(BUILD_DIR)/kernel_%.o)
DRIVER_OBJ = $(DRIVER_SRC:$(DRIVER_DIR)/%.c=$(BUILD_DIR)/driver_%.o)
SHELL_OBJ = $(SHELL_SRC:$(SHELL_DIR)/%.c=$(BUILD_DIR)/shell_%.o)
FS_OBJ = $(FS_SRC:$(FS_DIR)/%.c=$(BUILD_DIR)/fs_%.o)

.PHONY: all clean run

all: $(BUILD_DIR) $(KERNEL_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot_%.o: $(BOOT_DIR)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel_%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_%.o: $(KERNEL_DIR)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/driver_%.o: $(DRIVER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/shell_%.o: $(SHELL_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fs_%.o: $(FS_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(BOOT_OBJ) $(KERNEL_C_OBJ) $(KERNEL_ASM_OBJ) $(DRIVER_OBJ) $(SHELL_OBJ) $(FS_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBGCC)

run: $(KERNEL_BIN)
	qemu-system-x86_64 -kernel $(KERNEL_BIN)

clean:
	rm -rf $(BUILD_DIR)

# Help target
help:
	@echo "ClaudeOS Build System"
	@echo "  make all   - Build the kernel"
	@echo "  make run   - Build and run in QEMU"
	@echo "  make clean - Remove build artifacts"
