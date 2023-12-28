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

# Debug-specific compile options
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Release-specific compile options
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Root directory
include_directories(${PROJECT_SOURCE_DIR}/include)
# Klib library
include_directories(${PROJECT_SOURCE_DIR}/lib/klib)
# Colletions-C library
include_directories(${PROJECT_SOURCE_DIR}/lib/cc/src/include)
# Lock free stuctures
include_directories(${PROJECT_SOURCE_DIR}/lib/liblfds/liblfds7.1.1/liblfds711/inc)
link_directories(${PROJECT_SOURCE_DIR}/lib/liblfds/liblfds7.1.1/liblfds711/bin)

# Source files for the driver
file(GLOB DRIVER_C_SRCS 
    "src/driver.c"
    "src/netutil/*.c"  
    "src/netstack/*.c"        
    "src/event/*.c"
    "src/device/*.c"        
    "src/log/*.c" 
    "src/errors/*.c"
    "src/lock_free/*.c"
)
file(GLOB DRIVER_CPP_SRCS )

# Source files for the user
file(GLOB USER_C_SRCS 
    "src/ipc/*.c"
)

file(GLOB USER_CPP_SRCS "src/user.cpp")

# Common compile options for all targets
add_compile_options(
    -Wall -Wextra -Werror -pedantic
    -Wshadow -Wunused -Wno-unused-but-set-variable
    -Wmissing-declarations -Wredundant-decls 
    -Wmissing-field-initializers -Wtype-limits -Wno-packed-bitfield-compat
    -Wmissing-noreturn -Wno-format
    -Wno-unknown-pragmas            # For liblfds
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wimplicit-function-declaration")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

add_executable(TCP-IP ${DRIVER_C_SRCS} ${DRIVER_CPP_SRCS})
target_link_libraries(TCP-IP lfds711)

add_executable(USER ${USER_C_SRCS} ${USER_CPP_SRCS})

# Specific compile options for a target
# target_compile_options(YourExecutable PRIVATE -Wshadow -Wconversion)

# For tests, you might want to include another subdirectory
# add_subdirectory(tests)

# Custom command to run the TCP-IP executable
add_custom_command(
    OUTPUT TCP-IP_RUN
    COMMAND TCP-IP --log-file="../log/output.json" &
    DEPENDS TCP-IP
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    COMMENT "Running TCP-IP executable"
)

# Custom command to run the USER executable
add_custom_command(
    OUTPUT USER_RUN
    COMMAND USER &
    DEPENDS USER
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    COMMENT "Running USER executable"
)

# Custom target to run both executables
add_custom_target(run
    DEPENDS USER_RUN TCP-IP_RUN 
)