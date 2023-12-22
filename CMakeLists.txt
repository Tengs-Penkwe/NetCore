cmake_minimum_required(VERSION 3.10)

# Set your project name
project(TCP-IP)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add source directory
file(GLOB SOURCES "src/*.cpp" "src/netutil/*.c" "src/driver/*.c" "src/log/*.c" "src/errors/*.c")

# Common compile options for all targets
add_compile_options(
    -Wall -Wextra -Werror -pedantic
    )

# Define the executable
add_executable(TCP-IP ${SOURCES})

# Define your executable
# add_executable(YourExecutable source_file1.cpp source_file2.cpp)

# Specific compile options for a target
# target_compile_options(YourExecutable PRIVATE -Wshadow -Wconversion)

# Link libraries if there are any, e.g., pthread
# target_link_libraries(TCP-IP [libraries...])

# For tests, you might want to include another subdirectory
# add_subdirectory(tests)
