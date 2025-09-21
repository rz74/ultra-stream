#!/bin/bash

# Distribution Build Script for Data Processing Engine
# This script creates different types of distributable builds

set -e

echo "========== Data Processing Engine - Distribution Builder =========="
echo

# Clean previous builds
echo "[INFO] Cleaning previous builds..."
rm -rf build/ dist/

# Create distribution directory
mkdir -p dist

echo "[INFO] Available build types:"
echo "1. Standard build (default)"
echo "2. Static executable (portable, no dependencies)"
echo "3. Shared library + executable"
echo "4. Release optimized build"
echo "5. All variants"
echo

read -p "Choose build type (1-5): " choice

case $choice in
    1)
        echo "[INFO] Building standard executables..."
        mkdir -p build && cd build
        cmake .. && make
        cp bin/* ../dist/
        ;;
    2)
        echo "[INFO] Building static executables (portable)..."
        mkdir -p build && cd build
        cmake -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" .. && make
        cp bin/* ../dist/
        echo "[INFO] Static executables created (no external dependencies required)"
        ;;
    3)
        echo "[INFO] Building shared library + executables..."
        mkdir -p build && cd build
        # Modify CMakeLists.txt to enable shared library
        sed -i 's/# add_library(data_processing_engine SHARED/add_library(data_processing_engine SHARED/' ../CMakeLists.txt
        cmake .. && make
        cp bin/* ../dist/
        cp lib* ../dist/ 2>/dev/null || true
        # Restore CMakeLists.txt
        sed -i 's/add_library(data_processing_engine SHARED/# add_library(data_processing_engine SHARED/' ../CMakeLists.txt
        echo "[INFO] Shared library and executables created"
        ;;
    4)
        echo "[INFO] Building release optimized build..."
        mkdir -p build && cd build
        cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG" .. && make
        cp bin/* ../dist/
        echo "[INFO] Release optimized executables created"
        ;;
    5)
        echo "[INFO] Building all variants..."
        
        # Standard build
        echo "Building standard..."
        mkdir -p build_standard && cd build_standard
        cmake .. && make
        mkdir -p ../dist/standard && cp bin/* ../dist/standard/
        cd ..
        
        # Static build
        echo "Building static..."
        mkdir -p build_static && cd build_static
        cmake -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" .. && make
        mkdir -p ../dist/static && cp bin/* ../dist/static/
        cd ..
        
        # Release build
        echo "Building release..."
        mkdir -p build_release && cd build_release
        cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG" .. && make
        mkdir -p ../dist/release && cp bin/* ../dist/release/
        cd ..
        
        echo "[INFO] All variants created in dist/ directory"
        ;;
    *)
        echo "[ERROR] Invalid choice"
        exit 1
        ;;
esac

echo
echo "========== Distribution Build Complete =========="
echo "[INFO] Output files available in dist/ directory:"
ls -la dist/
echo
echo "[INFO] To create a distributable package:"
echo "tar -czf data_processing_engine.tar.gz dist/"
echo "or"
echo "zip -r data_processing_engine.zip dist/"