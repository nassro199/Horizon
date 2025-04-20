#!/bin/bash
# Build script for the Horizon kernel

# Exit on error
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print a colored message
function print_message() {
    echo -e "${BLUE}[BUILD]${NC} $1"
}

# Print an error message
function print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Print a success message
function print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Print a warning message
function print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if required tools are installed
function check_tools() {
    print_message "Checking required tools..."
    
    # Check for GCC
    if ! command -v gcc &> /dev/null; then
        print_error "GCC is not installed"
        exit 1
    fi
    
    # Check for LD
    if ! command -v ld &> /dev/null; then
        print_error "LD is not installed"
        exit 1
    fi
    
    # Check for NASM
    if ! command -v nasm &> /dev/null; then
        print_error "NASM is not installed"
        exit 1
    fi
    
    # Check for GRUB tools
    if ! command -v grub-mkrescue &> /dev/null; then
        print_error "GRUB tools are not installed"
        exit 1
    fi
    
    print_success "All required tools are installed"
}

# Clean the build directory
function clean() {
    print_message "Cleaning build directory..."
    
    # Remove build artifacts
    rm -rf build
    rm -f kernel.bin
    rm -f horizon.iso
    
    print_success "Build directory cleaned"
}

# Build the kernel
function build() {
    print_message "Building kernel..."
    
    # Create build directory
    mkdir -p build
    
    # Build the kernel
    make
    
    print_success "Kernel built successfully"
}

# Create a bootable ISO
function create_iso() {
    print_message "Creating bootable ISO..."
    
    # Create ISO directory structure
    mkdir -p build/iso/boot/grub
    
    # Copy kernel binary
    cp kernel.bin build/iso/boot/
    
    # Create GRUB configuration
    cat > build/iso/boot/grub/grub.cfg << EOF
menuentry "Horizon Kernel" {
    multiboot /boot/kernel.bin
}
EOF
    
    # Create ISO
    grub-mkrescue -o horizon.iso build/iso
    
    print_success "Bootable ISO created: horizon.iso"
}

# Run the kernel in QEMU
function run() {
    print_message "Running kernel in QEMU..."
    
    # Check if QEMU is installed
    if ! command -v qemu-system-i386 &> /dev/null; then
        print_error "QEMU is not installed"
        exit 1
    fi
    
    # Run the kernel
    qemu-system-i386 -cdrom horizon.iso
    
    print_success "QEMU exited"
}

# Debug the kernel in QEMU with GDB
function debug() {
    print_message "Debugging kernel in QEMU with GDB..."
    
    # Check if QEMU is installed
    if ! command -v qemu-system-i386 &> /dev/null; then
        print_error "QEMU is not installed"
        exit 1
    fi
    
    # Check if GDB is installed
    if ! command -v gdb &> /dev/null; then
        print_error "GDB is not installed"
        exit 1
    fi
    
    # Run the kernel with GDB server
    qemu-system-i386 -cdrom horizon.iso -s -S &
    
    # Connect GDB to QEMU
    gdb -ex "target remote localhost:1234" -ex "symbol-file kernel.bin"
    
    # Kill QEMU when GDB exits
    killall qemu-system-i386
    
    print_success "Debugging session ended"
}

# Print usage information
function usage() {
    echo "Usage: $0 [OPTION]"
    echo "Build and run the Horizon kernel"
    echo ""
    echo "Options:"
    echo "  clean     Clean the build directory"
    echo "  build     Build the kernel"
    echo "  iso       Create a bootable ISO"
    echo "  run       Run the kernel in QEMU"
    echo "  debug     Debug the kernel in QEMU with GDB"
    echo "  all       Clean, build, create ISO, and run"
    echo "  help      Display this help and exit"
    echo ""
    echo "If no option is specified, 'build' is assumed"
}

# Main function
function main() {
    # Check command line arguments
    if [ $# -eq 0 ]; then
        build
    else
        case "$1" in
            clean)
                clean
                ;;
            build)
                build
                ;;
            iso)
                build
                create_iso
                ;;
            run)
                build
                create_iso
                run
                ;;
            debug)
                build
                create_iso
                debug
                ;;
            all)
                clean
                build
                create_iso
                run
                ;;
            help)
                usage
                ;;
            *)
                print_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    fi
}

# Check tools before doing anything
check_tools

# Call main function with command line arguments
main "$@"
