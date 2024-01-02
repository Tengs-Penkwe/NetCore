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
file(GLOB INTEGRATION_TEST_SRCS "integration/*.c")

set(DISABLE_WARNINGS
    -Wno-attributes -Wno-unused-but-set-variable -Wno-strict-prototypes -Wno-old-style-definition -Wno-missing-prototypes -Wno-implicit-function-declaration
    -Wno-missing-declarations -Wno-uninitialized -Wno-suggest-attribute=noreturn
    -Wno-error
)

# Unit tests
add_executable(Unit_tests ${UNIT_TEST_SRCS})
target_link_libraries(Unit_tests NetCoreLib unity)
target_compile_options(Unit_tests PRIVATE ${COMMON_OPTIONS} ${KLIB_OPTIONS} ${LFDS_OPTIONS} ${DISABLE_WARNINGS})

add_executable(Integration_tests ${INTEGRATION_TEST_SRCS})
target_link_libraries(Integration_tests NetCoreLib unity)
target_compile_options(Integration_tests PRIVATE ${COMMON_OPTIONS} ${KLIB_OPTIONS} ${LFDS_OPTIONS} ${DISABLE_WARNINGS})

add_custom_command(
    OUTPUT NETCORE_UNIT_TESTS
    COMMAND Unit_tests
    DEPENDS Unit_tests
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running unit tests"
)

add_custom_command(
    OUTPUT NETCORE_INTEGRATION_TESTS
    COMMAND Integration_tests
    DEPENDS Integration_tests
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running integration tests"
)

# Custom target to run unit tests
add_custom_target(tests
    DEPENDS NETCORE_UNIT_TESTS NETCORE_INTEGRATION_TESTS
)