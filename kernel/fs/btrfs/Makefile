# BTRFS Makefile

# Object files
obj-y := btrfs.o super.o inode.o file.o btree.o

# Include directories
INCLUDES := -I$(TOPDIR)/include

# Compiler flags
CFLAGS := -Wall -Wextra -Werror -std=c99 $(INCLUDES)

# Default target
all: $(obj-y)

# Compile rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f *.o
