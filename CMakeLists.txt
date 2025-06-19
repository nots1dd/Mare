cmake_minimum_required(VERSION 3.27)
project(MareCompiler)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Directories ===
set(COMPILER_TARGET_DIR "Compiler")
set(BINARY_TARGET_DIR "Bin")
set(RUNTIME_TARGET_DIR "Runtime")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${BINARY_TARGET_DIR})

# === LLVM Setup ===
find_package(LLVM REQUIRED CONFIG)
message(STATUS "🧠 LLVM ${LLVM_PACKAGE_VERSION} Found in: ${LLVM_DIR}")
message(STATUS "🔧 LLVM Lib Dir: ${LLVM_LIBRARY_DIRS}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# === Sources ===
set(COMPILER_SRC "${COMPILER_TARGET_DIR}/main.cpp")
set(COMPILER_SRC_JIT "${COMPILER_TARGET_DIR}/llvm-test-jit.cpp")
set(RUNTIME_SRC "${RUNTIME_TARGET_DIR}/Runtime.cpp")
set(ENTRY_FILE "Entry.cpp")

# === Shared Runtime Library ===
add_library(mare-std-m SHARED ${RUNTIME_SRC})
set_target_properties(mare-std-m PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${RUNTIME_TARGET_DIR}
    OUTPUT_NAME "mare-std-m"
)

# Get git commit hash
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE __MARE_COMMIT_HASH__
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_TOOLS_BINARY_DIR}/llvm-config --host-target
  OUTPUT_VARIABLE __MARE_LLVM_TRIPLE__
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set variables
set(__MARE_VERSION__ "0.0.1")
set(__MARE_TARGET_ARCH__ "${CMAKE_SYSTEM_PROCESSOR}")
set(__MARE_BUILD_TYPE__ "${CMAKE_BUILD_TYPE}")

# Configure header file
configure_file(
  ${CMAKE_SOURCE_DIR}/Compiler/Include/Config.hpp.in
  ${CMAKE_BINARY_DIR}/generated/Config.hpp
  @ONLY
)

# === Compiler ===
add_executable(mare ${COMPILER_SRC})
target_link_libraries(mare PRIVATE LLVM-19)
set_target_properties(mare PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BINARY_TARGET_DIR}
)
# Include generated headers
target_include_directories(mare PRIVATE ${CMAKE_BINARY_DIR}/generated)

# === Custom Targets ===
add_custom_target(compiler DEPENDS mare)
add_custom_target(runtime DEPENDS mare-std-m)

# === Color Macros ===
string(ASCII 27 Esc)
set(RESET "${Esc}[0m")
set(BOLD "${Esc}[1m")
set(GREEN "${Esc}[32m")
set(CYAN  "${Esc}[36m")
set(PINK  "${Esc}[35m")
set(YELLOW "${Esc}[33m")

# === MareCompiler Build Summary ===
message(NOTICE "\n━━━━━━━━━━━━━━━━━━━━━━━  ${PINK}${BOLD}${PROJECT_NAME} Build Summary${RESET}  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")

message(STATUS "┌─ 🛠️  Build Configuration ─────────────────────────────────────────────")
message(STATUS "│ Operating System        : ${CMAKE_SYSTEM_NAME}")
message(STATUS "│ CMake Version           : ${CMAKE_VERSION}")
message(STATUS "│ Compiler                : ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "│ C++ Standard            : C++${CMAKE_CXX_STANDARD}")
message(STATUS "│ Build Type              : ${CMAKE_BUILD_TYPE}")
message(STATUS "│ Build System            : ${CMAKE_MAKE_PROGRAM}")
message(STATUS "│ Linker                  : ${CMAKE_LINKER}")
message(STATUS "│ Compiler Flags          : ${CMAKE_CXX_FLAGS}")
message(STATUS "│ Linker Flags            : ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "│ LLVM Version            : ${LLVM_PACKAGE_VERSION}")
message(STATUS "│ LLVM Link Mode          : -lLLVM-19")

message(STATUS "├─ 📂 Output Locations ───────────────────────────────────────────────")
message(STATUS "│ Binaries Output Dir     : ${CMAKE_BINARY_DIR}/${BINARY_TARGET_DIR}")
message(STATUS "│ Runtime Library Dir     : ${CMAKE_BINARY_DIR}/${RUNTIME_TARGET_DIR}")

message(STATUS "├─ 🚀 Built Targets ───────────────────────────────────────────────────")
message(STATUS "│ Compiler (Static)       : ${BINARY_TARGET_DIR}/mare")
message(STATUS "└─  Runtime (Shared lib)  : ${RUNTIME_TARGET_DIR}/libmare-std-m.so")
message(NOTICE "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")

# === Final All Target ===
add_custom_target(everything
  DEPENDS mare mare-std-m summary
)
