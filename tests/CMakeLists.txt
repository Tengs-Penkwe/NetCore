cmake_minimum_required(VERSION 3.10)

# Unity test framework
add_subdirectory(Unity)

# # Add Unity source files
# set(UNITY_SOURCES
#     Unity/src/unity.c
#     Unity/extras/fixture/src/unity_fixture.c
# )

# # Add Unity as a library
# add_library(Unity ${UNITY_SOURCES})

# # Include directories for Unity
# target_include_directories(Unity PUBLIC Unity/src Unity/extras/fixture/src)

# Add your test source files here
file(GLOB UNIT_TEST_SRCS "unit/*.c")
file(GLOB SYSTEM_TEST_SRCS "system/*.c")

set(DISABLE_WARNINGS
    -Wno-attributes -Wno-unused-but-set-variable -Wno-strict-prototypes -Wno-old-style-definition -Wno-missing-prototypes -Wno-implicit-function-declaration
    -Wno-missing-declarations -Wno-uninitialized
)

# Unit tests
add_executable(Unit_tests ${UNIT_TEST_SRCS})
target_link_libraries(Unit_tests NetCoreLib unity)
target_compile_options(Unit_tests PRIVATE ${COMMON_OPTIONS} ${KLIB_OPTIONS} ${LFDS_OPTIONS} ${DISABLE_WARNINGS})

add_executable(System_tests ${SYSTEM_TEST_SRCS})
target_link_libraries(System_tests NetCoreLib unity)
target_compile_options(System_tests PRIVATE ${COMMON_OPTIONS} ${KLIB_OPTIONS} ${LFDS_OPTIONS} ${DISABLE_WARNINGS})

add_custom_command(
    OUTPUT NETCORE_UNIT_TESTS
    COMMAND Unit_tests
    DEPENDS Unit_tests
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running unit tests"
)

add_custom_command(
    OUTPUT NETCORE_SYSTEM_TESTS
    COMMAND System_tests
    DEPENDS System_tests
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running system tests"
)

# Custom target to run unit tests
add_custom_target(tests
    DEPENDS NETCORE_UNIT_TESTS NETCORE_SYSTEM_TESTS
)