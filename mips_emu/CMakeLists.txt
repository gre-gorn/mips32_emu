cmake_minimum_required(VERSION 3.10)
project(MIPS32_Emulator VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific settings
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Build configuration
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_EXECUTABLE "Build standalone executable" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Source files for the library
set(LIBRARY_SOURCES
    mips32.cpp
    mips32.h
    bus.cpp
    bus.h
    ibus.cpp
    ibus.h
    mips32_wrapper.cpp
    mips32_wrapper.h
    mips32_api.cpp
    mips32_api.h
)

# Source files for the executable
set(EXECUTABLE_SOURCES
    mips_emu.cpp
    ${LIBRARY_SOURCES}
)

# Create the shared library
if(BUILD_SHARED_LIBS)
    add_library(mips32_emu SHARED ${LIBRARY_SOURCES})
    
    # Set library properties
    set_target_properties(mips32_emu PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME "mips32_emu"
        CLEAN_DIRECT_OUTPUT 1
    )
    
    # Platform-specific library settings
    if(WIN32)
        set_target_properties(mips32_emu PROPERTIES
            PREFIX ""
            SUFFIX ".dll"
        )
    elseif(APPLE)
        set_target_properties(mips32_emu PROPERTIES
            PREFIX "lib"
            SUFFIX ".dylib"
        )
    else()
        set_target_properties(mips32_emu PROPERTIES
            PREFIX "lib"
            SUFFIX ".so"
        )
    endif()
    
    # Define export symbols
    target_compile_definitions(mips32_emu PRIVATE MIPS32_EXPORTS)
    
    # Set include directories
    target_include_directories(mips32_emu PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
    
    # Compiler-specific flags
    if(MSVC)
        target_compile_options(mips32_emu PRIVATE /W4)
        target_compile_definitions(mips32_emu PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(mips32_emu PRIVATE -Wall -Wextra -pedantic)
    endif()
    
    # Debug/Release configurations
    target_compile_definitions(mips32_emu PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
endif()

# Create the executable (optional)
if(BUILD_EXECUTABLE)
    add_executable(mips32_emu_exe ${EXECUTABLE_SOURCES})
    
    set_target_properties(mips32_emu_exe PROPERTIES
        OUTPUT_NAME "mips32_emu"
    )
    
    if(MSVC)
        target_compile_options(mips32_emu_exe PRIVATE /W4)
        target_compile_definitions(mips32_emu_exe PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(mips32_emu_exe PRIVATE -Wall -Wextra -pedantic)
    endif()
endif()

# Install configuration
if(BUILD_SHARED_LIBS)
    install(TARGETS mips32_emu
        EXPORT MIPS32EmuTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )
    
    install(FILES
        mips32_api.h
        DESTINATION include
    )
    
    # Generate and install CMake config files
    include(CMakePackageConfigHelpers)
    
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/MIPS32EmuConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    
    export(EXPORT MIPS32EmuTargets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/MIPS32EmuTargets.cmake"
        NAMESPACE MIPS32Emu::
    )
    
    configure_file(cmake/MIPS32EmuConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/MIPS32EmuConfig.cmake"
        @ONLY
    )
    
    install(EXPORT MIPS32EmuTargets
        FILE MIPS32EmuTargets.cmake
        NAMESPACE MIPS32Emu::
        DESTINATION lib/cmake/MIPS32Emu
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/MIPS32EmuConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/MIPS32EmuConfigVersion.cmake"
        DESTINATION lib/cmake/MIPS32Emu
    )
endif()

# Unity-specific build target
add_custom_target(unity-package
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/unity-package
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mips32_emu> ${CMAKE_BINARY_DIR}/unity-package/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/mips32_api.h ${CMAKE_BINARY_DIR}/unity-package/
    DEPENDS mips32_emu
    COMMENT "Creating Unity package"
)

# Test configuration
if(BUILD_TESTS)
    enable_testing()
    
    # Simple API test
    add_executable(api_test test/api_test.cpp)
    target_link_libraries(api_test mips32_emu)
    add_test(NAME APITest COMMAND api_test)
    
    # Memory test
    add_executable(memory_test test/memory_test.cpp)
    target_link_libraries(memory_test mips32_emu)
    add_test(NAME MemoryTest COMMAND memory_test)
endif()

# Print build information
message(STATUS "MIPS32 Emulator Build Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "  Executable: ${BUILD_EXECUTABLE}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")

if(BUILD_SHARED_LIBS)
    if(WIN32)
        message(STATUS "  Output: mips32_emu.dll")
    elseif(APPLE)
        message(STATUS "  Output: libmips32_emu.dylib")
    else()
        message(STATUS "  Output: libmips32_emu.so")
    endif()
endif()
