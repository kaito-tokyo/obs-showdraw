# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- OBS team maintains the CMake and GitHub Actions so we don't need to improve these parts.

## How to build and run tests on macOS

1. Run `cmake --preset macos`.
2. Run `cmake --build build_macos`.
3. Run `ctest`.
