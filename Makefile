# Makefile for the kernel

# Compiler and flags
CC = gcc
AS = as
LD = ld
CFLAGS = -Wall -Wextra -nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32 -I./include -I./arch/x86/include
ASFLAGS = --32
LDFLAGS = -m elf_i386 -T arch/x86/kernel/kernel.ld

# Source directories
KERNEL_DIR = kernel
MM_DIR = mm
FS_DIR = fs
DRIVERS_DIR = drivers
ARCH_DIR = arch/x86
LIB_DIR = lib
INIT_DIR = init
IPC_DIR = ipc
SECURITY_DIR = security
BLOCK_DIR = block
CRYPTO_DIR = crypto
NET_DIR = net

# Source files
KERNEL_SRCS = $(wildcard $(KERNEL_DIR)/*.c) $(wildcard $(KERNEL_DIR)/*/*.c) $(wildcard $(KERNEL_DIR)/*/*/*.c)
MM_SRCS = $(wildcard $(MM_DIR)/*.c) $(wildcard $(MM_DIR)/*/*.c)
FS_SRCS = $(wildcard $(FS_DIR)/*.c) $(wildcard $(FS_DIR)/*/*.c)
DRIVERS_SRCS = $(wildcard $(DRIVERS_DIR)/*/*.c) $(wildcard $(DRIVERS_DIR)/*/*/*.c)
ARCH_SRCS = $(wildcard $(ARCH_DIR)/kernel/*.c)
ARCH_ASM_SRCS = $(wildcard $(ARCH_DIR)/boot/*.S) $(wildcard $(ARCH_DIR)/kernel/*.S)
LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
INIT_SRCS = $(wildcard $(INIT_DIR)/*.c)
IPC_SRCS = $(wildcard $(IPC_DIR)/*.c)
SECURITY_SRCS = $(wildcard $(SECURITY_DIR)/*.c)
BLOCK_SRCS = $(wildcard $(BLOCK_DIR)/*.c)
CRYPTO_SRCS = $(wildcard $(CRYPTO_DIR)/*.c)
NET_SRCS = $(wildcard $(NET_DIR)/*.c)

# Object files
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
MM_OBJS = $(MM_SRCS:.c=.o)
FS_OBJS = $(FS_SRCS:.c=.o)
DRIVERS_OBJS = $(DRIVERS_SRCS:.c=.o)
ARCH_OBJS = $(ARCH_SRCS:.c=.o)
ARCH_ASM_OBJS = $(ARCH_ASM_SRCS:.S=.o)
LIB_OBJS = $(LIB_SRCS:.c=.o)
INIT_OBJS = $(INIT_SRCS:.c=.o)
IPC_OBJS = $(IPC_SRCS:.c=.o)
SECURITY_OBJS = $(SECURITY_SRCS:.c=.o)
BLOCK_OBJS = $(BLOCK_SRCS:.c=.o)
CRYPTO_OBJS = $(CRYPTO_SRCS:.c=.o)
NET_OBJS = $(NET_SRCS:.c=.o)

# All object files
OBJS = $(KERNEL_OBJS) $(MM_OBJS) $(FS_OBJS) $(DRIVERS_OBJS) $(ARCH_OBJS) $(ARCH_ASM_OBJS) $(LIB_OBJS) \
       $(INIT_OBJS) $(IPC_OBJS) $(SECURITY_OBJS) $(BLOCK_OBJS) $(CRYPTO_OBJS) $(NET_OBJS)

# Output files
KERNEL = kernel.bin
ISO = horizon.iso

# Default target
all: $(KERNEL)

# Build the kernel
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile assembly files
%.o: %.S
	$(AS) $(ASFLAGS) -o $@ $<

# Create an ISO image
iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/
	echo "menuentry \"Horizon Kernel\" {" > iso/boot/grub/grub.cfg
	echo "    multiboot /boot/$(KERNEL)" >> iso/boot/grub/grub.cfg
	echo "}" >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso

# Run in QEMU
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

# Run in QEMU with debug
debug: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -s -S

# Clean up
clean:
	rm -f $(KERNEL) $(OBJS) $(ISO)
	rm -rf iso

# Phony targets
.PHONY: all iso run debug clean
