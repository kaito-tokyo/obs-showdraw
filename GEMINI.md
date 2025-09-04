# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- C and C++ files must be formatted using clang-format-19 after any modification.
- CMake files must ve formatted using gersemi after any modification.

## How to build on macOS

1. Run `cmake --preset macos`.
2. Run `cmake --build --preset macos`.

## How to run tests on macOS

1. Run `cmake --preset macos-testing`.
2. Run `cmake --build --preset macos-testing`.
3. Run `ctest --preset macos-testing`.
