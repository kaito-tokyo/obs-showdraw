# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- gs_effect_t and gs_texture_t must be destroyed within graphics context, and this is not possible by unique_ptr.
