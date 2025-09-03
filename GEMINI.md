# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- gs_effect_t and gs_texture_t must be destroyed within graphics context, and this control is so hard for RAII and we decide that we won't manage effect and texture by unique_ptr.
- OBS team maintains the CMake and GitHub Actions so we don't need to improve these parts.
