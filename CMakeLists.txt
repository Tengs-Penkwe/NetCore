cmake_minimum_required(VERSION 3.10)

############################################
######### Project Name & Standard #########
############################################
project(NetCore)

# Set the C standard to C23 and C++ standard to C++17
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

############################################
############# Debug or Release ############
############################################
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Debug-specific compile options
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Release-specific compile options
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

############################################
############# Header Files ############
############################################
# Root directory
include_directories(${PROJECT_SOURCE_DIR}/include)
# Klib library
include_directories(${PROJECT_SOURCE_DIR}/lib/klib)
# Colletions-C library
include_directories(${PROJECT_SOURCE_DIR}/lib/cc/src/include)
# Lock free stuctures
include_directories(${PROJECT_SOURCE_DIR}/lib/liblfds/liblfds7.1.1/liblfds711/inc)
link_directories(${PROJECT_SOURCE_DIR}/lib/liblfds/liblfds7.1.1/liblfds711/bin)
# Unity testing framework
include_directories(tests/Unity/src)
add_subdirectory(tests/Unity)

############################################
############# Source Files ############
############################################
# Source files for the driver
file(GLOB DRIVER_C_SRCS 
    "src/netutil/*.c"  "src/netstack/*.c" "src/event/*.c" 
    "src/device/*.c" "src/log/*.c" "src/errors/*.c" "src/lock_free/*.c")
file(GLOB DRIVER_MAIN "src/driver.c")
file(GLOB DRIVER_CPP_SRCS)
# Unit test for the driver
file(GLOB UNIT_TEST_SRCS "tests/unit/*.c")

# Source files for the user
file(GLOB USER_C_SRCS "src/ipc/*.c")
file(GLOB USER_CPP_SRCS "src/user.cpp")

############################################
############# Compile Flags ############
############################################
# Enable common warnings
add_compile_options(-Wall -Wextra -Werror -pedantic)

set(COMMON_OPTIONS
    -Wlogical-op
    -Wduplicated-cond -Wduplicated-branches
    -Wnull-dereference -Wvector-operation-performance
    -Walloca -Wunsafe-loop-optimizations -Wdisabled-optimization
    -Wpacked-bitfield-compat -Wstack-protector
    -Wcast-align -Wcast-qual
    -Wtautological-compare
    -Wsign-compare -Wsign-conversion
    -Wstrict-overflow=5 -Wfloat-equal -Wdouble-promotion
    -Wformat=2
    -Wswitch-default
    -Wundef -Wuninitialized -Wpointer-arith
    -Wshadow -Wunused -Wno-unused-but-set-variable
    -Wmissing-declarations -Wredundant-decls
    -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wimplicit-function-declaration
    -Wmissing-field-initializers -Wtype-limits -Wno-packed-bitfield-compat
    -Wmissing-noreturn -Wno-format
    -Wmissing-include-dirs
)

set(STRICT_OPTIONS
    -Wconversion 
    -Wpadded
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuseless-cast -Wconversion-null -Wextra-semi")

############################################
############# Executables ############
############################################
# The network driver executable
add_executable(NetCore ${DRIVER_C_SRCS} ${DRIVER_CPP_SRCS} ${DRIVER_MAIN})
target_link_libraries(NetCore lfds711)
target_compile_options(NetCore PRIVATE
    ${COMMON_OPTIONS} ${STRICT_OPTIONS}
# For klib
    -Wno-conversion  -Wno-sign-conversion -Wno-cast-qual
# For liblfds
    -Wno-unknown-pragmas  -Wno-padded          
    )

# The user executable
add_executable(USER ${USER_C_SRCS} ${USER_CPP_SRCS})
target_compile_options(NetCore PRIVATE ${COMMON_OPTIONS} ${STRICT_OPTIONS})

# The unit tests for the driver
add_executable(UNIT_TESTS ${UNIT_TEST_SRCS} ${DRIVER_C_SRCS} ${DRIVER_CPP_SRCS})
target_link_libraries(UNIT_TESTS lfds711 unity)
target_compile_options(UNIT_TESTS PRIVATE
    -Wno-conversion  -Wno-sign-conversion -Wno-cast-qual -Wno-unknown-pragmas  -Wno-padded -Wno-attributes
    -Wno-unused-but-set-variable
    )

############################################
#############    Commands    ############
############################################
add_custom_command(
    OUTPUT NetCore_DEBUG
    COMMAND NetCore --log-file="/dev/stdout" --log-ansi
    DEPENDS NetCore
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Debug NetCore executable"
)

add_custom_command(
    OUTPUT NetCore_RUN
    COMMAND NetCore --log-file="log/output.json" --log-level=2 &
    DEPENDS NetCore
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Debug NetCore executable log level DEBUG"
)

add_custom_command(
    OUTPUT USER_RUN
    COMMAND USER &
    DEPENDS USER
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running USER executable"
)

add_custom_command(
    OUTPUT TCP_IP_UNIT_TEST
    COMMAND UNIT_TESTS
    DEPENDS UNIT_TESTS
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running unit tests"
)

# Custom target to run both executables
add_custom_target(run
    DEPENDS USER_RUN NetCore_RUN 
)

# Custom target to run both executables
add_custom_target(debug
    DEPENDS USER_RUN NetCore_DEBUG 
)

# Custom target to run unit tests
add_custom_target(unit_test
    DEPENDS TCP_IP_UNIT_TEST
)