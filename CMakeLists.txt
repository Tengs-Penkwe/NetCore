cmake_minimum_required(VERSION 3.10)

# Set your project name
project(TCP-IP)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Root directory
include_directories(${PROJECT_SOURCE_DIR}/include)
# Klib library
include_directories(${PROJECT_SOURCE_DIR}/lib/klib)
# Colletions-C library
include_directories(${PROJECT_SOURCE_DIR}/lib/cc/src/include)

# Add source files
file(GLOB C_SOURCES 
    "src/netutil/*.c"  
    "src/netstack/*.c"        
    "src/event/*.c"
    "src/device/*.c"        
    "src/log/*.c" 
    "src/errors/*.c"
)
file(GLOB CPP_SOURCES "src/*.cpp")
# Common compile options for all targets
add_compile_options(
    -Wall -Wextra -Werror -pedantic
    -Wshadow -Wunused -Wno-unused-but-set-variable
    -Wmissing-declarations -Wredundant-decls 
    -Wmissing-field-initializers -Wtype-limits -Wno-packed-bitfield-compat
    -Wmissing-noreturn -Wno-format
    )

# Common compile options for C files
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wimplicit-function-declaration")

# Common compile options for C++ files
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

# Debug-specific compile options
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Release-specific compile options
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Define the executable
add_executable(TCP-IP ${C_SOURCES} ${CPP_SOURCES})

# Define your executable
# add_executable(YourExecutable source_file1.cpp source_file2.cpp)

# Specific compile options for a target
# target_compile_options(YourExecutable PRIVATE -Wshadow -Wconversion)

# Link libraries if there are any, e.g., pthread
# target_link_libraries(TCP-IP [libraries...])

# For tests, you might want to include another subdirectory
# add_subdirectory(tests)

# Custom target to run the executable
add_custom_target(run
    COMMAND TCP-IP
    DEPENDS TCP-IP
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
