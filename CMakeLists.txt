cmake_minimum_required(VERSION 3.27)
project(GooCompiler)

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
set(COMPILER_SRC "${COMPILER_TARGET_DIR}/goo.cpp")
set(COMPILER_SRC_JIT "${COMPILER_TARGET_DIR}/llvm-test-jit.cpp")
set(RUNTIME_SRC "${RUNTIME_TARGET_DIR}/Runtime.cpp")
set(ENTRY_FILE "Entry.cpp")

# === Shared Runtime Library ===
add_library(goostdm SHARED ${RUNTIME_SRC})
set_target_properties(goostdm PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${RUNTIME_TARGET_DIR}
    OUTPUT_NAME "goostdm"
)

# === Compiler ===
add_executable(goo ${COMPILER_SRC})
target_link_libraries(goo PRIVATE LLVM-19)
set_target_properties(goo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BINARY_TARGET_DIR}
)

# === Custom Targets ===
add_custom_target(compiler DEPENDS goo)
add_custom_target(compiler-jit DEPENDS goo-jit)
add_custom_target(runtime DEPENDS goostdm)

# === Color Macros ===
string(ASCII 27 Esc)
set(RESET "${Esc}[0m")
set(BOLD "${Esc}[1m")
set(GREEN "${Esc}[32m")
set(CYAN  "${Esc}[36m")
set(PINK  "${Esc}[35m")
set(YELLOW "${Esc}[33m")

# === GooCompiler Build Summary ===
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
message(STATUS "│ Final Shared Library    : ${CMAKE_BINARY_DIR}/${RUNTIME_TARGET_DIR}/libgoostdm.so")

message(STATUS "├─ 🚀 Built Targets ───────────────────────────────────────────────────")
message(STATUS "│ Compiler (Static)       : ${BINARY_TARGET_DIR}/goo")
message(STATUS "└─  Runtime (Shared lib)  : ${RUNTIME_TARGET_DIR}/libgoostdm.so")
message(NOTICE "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")

# === Final All Target ===
add_custom_target(everything
  DEPENDS goo goostdm summary
)
