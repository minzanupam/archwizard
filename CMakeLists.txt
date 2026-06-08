cmake_minimum_required(VERSION 3.16)

project(ArchWizard)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

# asan config
set(CMAKE_CXX_FLAGS_ASAN "-O1 -g -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
    "Flags used by the CXX compiler during ASan builds." FORCE)
set(CMAKE_C_FLAGS_ASAN "-O1 -g -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
    "Flags used by the C compiler during ASan builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_ASAN "-fsanitize=address" CACHE STRING
    "Flags used for linking binaries during ASan builds." FORCE)

# default to Debug build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "ASan")
endif()

set(SOURCES src/main.cpp)

# glfw config
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw-3.4)

# load opengl for use with glad
find_package(OpenGL REQUIRED)

# glad config
add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/include)
target_link_libraries(glad PUBLIC OpenGL::GL)

# compile binary
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

# link binary
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw glad)
