# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- C and C++ files must be formatted using clang-format-19 after any modification.
- CMake files must be formatted using gersemi after any modification.
- OBS team maintains the CMake and GitHub Actions so we don't need to improve these parts.

## How to build and run tests on macOS

1. Run `cmake --preset macos` if CMake-related files were changed.
2. Run `cmake --build --preset macos`.
3. Run `ctest --preset macos --rerun-failed --output-on-failure`.
