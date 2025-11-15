#!/usr/bin/env bash
# Build script for EA Compression shared library on Linux

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building EA Compression shared library...${NC}"

# Set compiler flags
CXXFLAGS="-fPIC -O3 -Wall -Wextra"
INCLUDES="-I. -IUNIX -IHUFF -IREFPACK -IBTREE -IJDLZ -ICOMP"
LDFLAGS="-shared -Wl,-soname,libea_compression.so.1"
OUTPUT="libea_compression.so.1.0.0"

# Compile the shared library
g++ $CXXFLAGS $INCLUDES $LDFLAGS \
    -fexec-charset=ISO-8859-1 \
    -fpermissive \
    ea_compression_lib.cpp \
    -o $OUTPUT

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Shared library built successfully: $OUTPUT${NC}"
    
    # Create symbolic links
    ln -sf $OUTPUT libea_compression.so.1
    ln -sf libea_compression.so.1 libea_compression.so
    
    echo -e "${GREEN}✓ Symbolic links created${NC}"
    echo ""
    echo -e "${YELLOW}To install the library system-wide:${NC}"
    echo "  sudo cp $OUTPUT /usr/local/lib/"
    echo "  sudo cp ea_compression_lib.h /usr/local/include/"
    echo "  sudo ldconfig"
    echo ""
    echo -e "${YELLOW}To use the library:${NC}"
    echo "  Compile: gcc -o myapp myapp.c -lea_compression"
    echo "  Or set: export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$(pwd)"
else
    echo -e "${RED}✗ Compilation failed${NC}"
    exit 1
fi
