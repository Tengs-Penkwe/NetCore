cmake_minimum_required(VERSION 3.10)

# Add Unity source files
set(UNITY_SOURCES
    Unity/src/unity.c
    Unity/extras/fixture/src/unity_fixture.c
)

# Add Unity as a library
add_library(Unity ${UNITY_SOURCES})

# Include directories for Unity
target_include_directories(Unity PUBLIC Unity/src Unity/extras/fixture/src)
