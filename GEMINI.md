# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.

## How to build on macOS

1. Run `cmake --preset macos`.
2. Run `cmake --build --preset macos`.

## How to run tests on macOS

1. Run `cmake --preset macos-testing`.
2. Run `cmake --build --preset macos-testing`.
3. Run `ctest --preset macos-testing`.
