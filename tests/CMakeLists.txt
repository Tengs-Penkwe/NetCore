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
file(GLOB TEST_SRCS "unit/*.c")

# Unit tests
add_executable(Unit_tests ${TEST_SRCS})
target_link_libraries(Unit_tests NetCoreLib unity)
target_compile_options(Unit_tests PRIVATE ${KLIB_OPTIONS} ${LFDS_OPTIONS}
    -Wno-attributes -Wno-unused-but-set-variable -Wno-strict-prototypes -Wno-old-style-definition -Wno-missing-prototypes -Wno-implicit-function-declaration
)

add_custom_command(
    OUTPUT NETCORE_UNIT_TESTS
    COMMAND Unit_tests
    DEPENDS Unit_tests
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running unit tests"
)

# Custom target to run unit tests
add_custom_target(tests
    DEPENDS NETCORE_UNIT_TESTS
)