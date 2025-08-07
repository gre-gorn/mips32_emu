#!/bin/bash

# MIPS32 Emulator Build Script
# Supports building for different platforms and configurations

set -e  # Exit on any error

# Default values
BUILD_TYPE="Release"
BUILD_SHARED="ON"
BUILD_EXECUTABLE="OFF"
BUILD_TESTS="OFF"
CLEAN_BUILD="false"
INSTALL_PREFIX=""
UNITY_PACKAGE="false"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show help
show_help() {
    echo "MIPS32 Emulator Build Script"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -t, --type TYPE         Build type: Debug, Release, RelWithDebInfo (default: Release)"
    echo "  -s, --shared BOOL       Build shared library: ON/OFF (default: ON)"
    echo "  -e, --executable BOOL   Build executable: ON/OFF (default: OFF)"
    echo "  -T, --tests BOOL        Build tests: ON/OFF (default: OFF)"
    echo "  -c, --clean             Clean build directory before building"
    echo "  -i, --install PREFIX    Install prefix (optional)"
    echo "  -u, --unity             Create Unity package after build"
    echo ""
    echo "Examples:"
    echo "  $0                      # Build shared library in Release mode"
    echo "  $0 -t Debug -T ON       # Build with debug info and tests"
    echo "  $0 -e ON -s OFF         # Build standalone executable"
    echo "  $0 -u                   # Build and create Unity package"
    echo "  $0 -c -t Release        # Clean build in Release mode"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -s|--shared)
            BUILD_SHARED="$2"
            shift 2
            ;;
        -e|--executable)
            BUILD_EXECUTABLE="$2"
            shift 2
            ;;
        -T|--tests)
            BUILD_TESTS="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -i|--install)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -u|--unity)
            UNITY_PACKAGE="true"
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Validate build type
if [[ ! "$BUILD_TYPE" =~ ^(Debug|Release|RelWithDebInfo|MinSizeRel)$ ]]; then
    print_error "Invalid build type: $BUILD_TYPE"
    exit 1
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build"

print_info "MIPS32 Emulator Build Configuration:"
print_info "  Project Directory: $PROJECT_DIR"
print_info "  Build Directory: $BUILD_DIR"
print_info "  Build Type: $BUILD_TYPE"
print_info "  Shared Library: $BUILD_SHARED"
print_info "  Executable: $BUILD_EXECUTABLE"
print_info "  Tests: $BUILD_TESTS"
print_info "  Unity Package: $UNITY_PACKAGE"

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == "true" ]]; then
    print_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Prepare CMake arguments
CMAKE_ARGS=(
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DBUILD_SHARED_LIBS=$BUILD_SHARED"
    "-DBUILD_EXECUTABLE=$BUILD_EXECUTABLE"
    "-DBUILD_TESTS=$BUILD_TESTS"
)

if [[ -n "$INSTALL_PREFIX" ]]; then
    CMAKE_ARGS+=("-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX")
fi

# Run CMake configuration
print_info "Running CMake configuration..."
if ! cmake "${CMAKE_ARGS[@]}" "$PROJECT_DIR/mips_emu"; then
    print_error "CMake configuration failed!"
    exit 1
fi

print_success "CMake configuration completed successfully"

# Build the project
print_info "Building project..."
if ! cmake --build . --config "$BUILD_TYPE" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4); then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully"

# Run tests if enabled
if [[ "$BUILD_TESTS" == "ON" ]]; then
    print_info "Running tests..."
    if ! ctest --config "$BUILD_TYPE" --output-on-failure; then
        print_warning "Some tests failed!"
    else
        print_success "All tests passed"
    fi
fi

# Create Unity package if requested
if [[ "$UNITY_PACKAGE" == "true" && "$BUILD_SHARED" == "ON" ]]; then
    print_info "Creating Unity package..."
    if ! cmake --build . --target unity-package; then
        print_error "Unity package creation failed!"
        exit 1
    fi
    
    UNITY_PACKAGE_DIR="$BUILD_DIR/unity-package"
    print_success "Unity package created in: $UNITY_PACKAGE_DIR"
    
    # List package contents
    print_info "Package contents:"
    ls -la "$UNITY_PACKAGE_DIR"
fi

# Install if prefix was specified
if [[ -n "$INSTALL_PREFIX" ]]; then
    print_info "Installing to: $INSTALL_PREFIX"
    if ! cmake --build . --target install; then
        print_error "Installation failed!"
        exit 1
    fi
    print_success "Installation completed"
fi

# Show output files
print_info "Build artifacts:"
if [[ "$BUILD_SHARED" == "ON" ]]; then
    # Find shared library
    if [[ "$OSTYPE" == "darwin"* ]]; then
        SHARED_LIB=$(find . -name "*.dylib" -type f)
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        SHARED_LIB=$(find . -name "*.dll" -type f)
    else
        SHARED_LIB=$(find . -name "*.so" -type f)
    fi
    
    if [[ -n "$SHARED_LIB" ]]; then
        print_info "  Shared library: $SHARED_LIB"
        ls -la "$SHARED_LIB"
    fi
fi

if [[ "$BUILD_EXECUTABLE" == "ON" ]]; then
    EXECUTABLE=$(find . -name "mips32_emu" -type f -executable)
    if [[ -n "$EXECUTABLE" ]]; then
        print_info "  Executable: $EXECUTABLE"
        ls -la "$EXECUTABLE"
    fi
fi

print_success "Build script completed successfully!"
print_info "You can now use the MIPS32 emulator library in Unity or other applications."
