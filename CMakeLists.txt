cmake_minimum_required(VERSION 3.16)
project(GooCompiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_C_COMPILER "clang++")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set output directories
set(COMPILER_TARGET_DIR Compiler)
set(BINARY_TARGET_DIR ${CMAKE_BINARY_DIR}/Bin)
set(RUNTIME_TARGET_DIR Runtime)

# Compiler files
set(COMPILER_SRC ${COMPILER_TARGET_DIR}/llvm-test.cpp)
set(COMPILER_SRC_JIT ${COMPILER_TARGET_DIR}/llvm-test-jit.cpp)
set(COMPILER_BIN ${BINARY_TARGET_DIR}/goo)
set(COMPILER_BIN_JIT ${BINARY_TARGET_DIR}/goo-jit)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Runtime files
set(RUNTIME_SRC ${RUNTIME_TARGET_DIR}/Runtime.cpp)

# Entry file
set(ENTRY_FILE Entry.cpp)
set(FINAL_BIN ${BINARY_TARGET_DIR}/hope)

# Create output directory
file(MAKE_DIRECTORY ${BINARY_TARGET_DIR})

# Step 1: Compile runtime as a static library
add_library(Runtime STATIC ${RUNTIME_SRC})

# Step 2: Compile the compiler binaries
add_executable(goo ${COMPILER_SRC})
target_link_libraries(goo PRIVATE LLVM)
set_target_properties(goo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BINARY_TARGET_DIR})

add_executable(goo-jit ${COMPILER_SRC_JIT})
target_link_libraries(goo-jit PRIVATE LLVM)
set_target_properties(goo-jit PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BINARY_TARGET_DIR})

# Step 3: Compile entry binary and link with runtime
add_executable(hope ${ENTRY_FILE})
target_link_libraries(hope PRIVATE Runtime)
set_target_properties(hope PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BINARY_TARGET_DIR})

# Step 4: Custom targets to match Makefile execution order
add_custom_target(compiler
    COMMAND ${CMAKE_COMMAND} --build . --target goo
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(compiler-jit
    COMMAND ${CMAKE_COMMAND} --build . --target goo-jit
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(runtime
    COMMAND ${CMAKE_COMMAND} --build . --target Runtime
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


add_custom_target(entry
    DEPENDS runtime
    COMMAND ${CMAKE_COMMAND} --build . --target hope
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(execute
    DEPENDS run-compiler entry
    COMMAND ${CMAKE_COMMAND} -E env ${BINARY_TARGET_DIR}/hope ${EXTRA_FLAGS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Use Ninja generator
set(CMAKE_GENERATOR "Ninja")
