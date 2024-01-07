cmake_minimum_required(VERSION 3.21)

############################################
######### Project Name & Standard #########
############################################
project(NetCore)

# Set the C standard to C23 and C++ standard to C++17
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 23)
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
# Lock free stuctures: liblfds (https://www.liblfds.org/)
include_directories(${PROJECT_SOURCE_DIR}/lib/liblfds/liblfds7.1.1/liblfds711/inc)
link_directories(${PROJECT_SOURCE_DIR}/lib/liblfds/liblfds7.1.1/liblfds711/bin)
# Lock free stuctures: Junction 
include_directories(${PROJECT_SOURCE_DIR}/lib/junction)
# Lock free stuctures: LibCuckoo
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/libcuckoo)
include_directories(${PROJECT_SOURCE_DIR}/lib/libcuckoo)

############################################
############# Source Files ############
############################################
# Source files for the driver
file(GLOB DRIVER_C_SRCS 
    "src/netutil/*.c"  "src/netstack/*.c" "src/event/*.c" 
    "src/device/*.c" "src/log/*.c" "src/errors/*.c" "src/lock_free/*.c")
file(GLOB DRIVER_MAIN "app/driver/driver.c")
file(GLOB DRIVER_CPP_SRCS)
# Unit test for the driver
file(GLOB UNIT_TEST_SRCS "tests/unit/*.c")

# Source files for the user
file(GLOB USER_C_SRCS "src/ipc/*.c")
file(GLOB USER_CPP_SRCS "app/user/user.cpp")

############################################
############# Compile Flags ############
############################################

# Common warnings
add_compile_options(-Wall -Wextra -Werror -pedantic -Wno-unknown-warning-option)

# Clang options: use -Weverything but disable some warnings
if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CLANG_OPTIONS -Weverything
        -Wno-reserved-identifier -Wno-reserved-macro-identifier -Wno-newline-eof -Wno-extra-semi-stmt -Wno-declaration-after-statement
        -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-binary-literal -Wno-c++98-compat -Wno-pre-c2x-compat
        -Wno-covered-switch-default -Wno-switch-enum -Wno-disabled-macro-expansion
        -Wno-used-but-marked-unused
        -Wno-cast-align -Wno-packed
    )
endif()

set(COMMON_OPTIONS
    ${CLANG_OPTIONS} 
    -Wconversion -Wpadded -Wcast-qual
    -Wlogical-op
    -Wduplicated-cond -Wduplicated-branches
    -Wnull-dereference -Wvector-operation-performance
    -Walloca -Wunsafe-loop-optimizations -Wdisabled-optimization
    -Wpacked-bitfield-compat -Wstack-protector
    -Wtautological-compare
    -Wsign-compare -Wsign-conversion
    -Wstrict-overflow=5 -Wfloat-equal -Wdouble-promotion
    -Wformat=2
    -Wswitch-default
    -Wundef -Wuninitialized -Wpointer-arith
    -Wshadow -Wunused -Wno-unused-but-set-variable
    -Wmissing-declarations -Wredundant-decls
    -Wmissing-field-initializers -Wtype-limits -Wno-packed-bitfield-compat
    -Wmissing-noreturn -Wno-format
    -Wmissing-include-dirs
)

set(KLIB_OPTIONS
    -Wno-conversion -Wno-sign-conversion -Wno-cast-qual -Wno-unsafe-buffer-usage -Wno-array-bounds-pointer-arithmetic -Wno-comma 
)
set(LFDS_OPTIONS
    -Wno-unknown-pragmas -Wno-padded -Wno-atomic-implicit-seq-cst        
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wimplicit-function-declaration ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuseless-cast -Wconversion-null -Wextra-semi")

############################################
############# Library ############
###########################################
# Create a library target
add_library(NetCoreLib ${DRIVER_C_SRCS} ${DRIVER_CPP_SRCS})

# Link libraries and set compile options (as needed)
target_link_libraries(NetCoreLib lfds711 libcuckoo)
target_compile_options(NetCoreLib PRIVATE ${COMMON_OPTIONS} ${KLIB_OPTIONS} ${LFDS_OPTIONS})

############################################
############# Executables ############
############################################
# The network driver executable
add_executable(NetCore ${DRIVER_MAIN})
target_link_libraries(NetCore NetCoreLib)
target_compile_options(NetCore PRIVATE ${COMMON_OPTIONS} ${KLIB_OPTIONS} ${LFDS_OPTIONS})

# The user executable
add_executable(User ${USER_C_SRCS} ${USER_CPP_SRCS})
target_compile_options(User PRIVATE ${COMMON_OPTIONS})

# The tests for the driver
add_subdirectory(tests)    # Contains tests and its CMakeLists.txt

############################################
#############    Commands    ############
############################################
add_custom_command(
    OUTPUT NETCORE_DEBUG
    COMMAND NetCore --log-file="/dev/stdout" --log-ansi
    DEPENDS NetCore
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Debug NetCore executable"
)

add_custom_command(
    OUTPUT NETCORE_RUN
    COMMAND NetCore --log-file="log/output.json" --log-level=2 &
    DEPENDS NetCore
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Debug NetCore executable log level DEBUG"
)

add_custom_command(
    OUTPUT USER_RUN
    COMMAND User &
    DEPENDS User
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running USER executable"
)

# Set capabilities (We need unlimited signal queue size)
add_custom_command(
    TARGET NetCore POST_BUILD
    COMMAND sudo setcap cap_sys_resource=eip $<TARGET_FILE:NetCore>
    COMMENT "Setting capabilities on NetCore to allow unlimited signal queue size"
)

# Custom target to run both executables
add_custom_target(run
    DEPENDS USER_RUN NETCORE_RUN 
)

# Custom target to run both executables
add_custom_target(debug
    DEPENDS USER_RUN NETCORE_DEBUG 
)