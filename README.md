# chalkboard

C++20 library for building HTML math reports with LaTeX/MathJax rendering.

![chalkboard viewer demo](assets/demo.png)

## Requirements

- C++20
- CMake 3.20+

## Installation

```cmake
include(FetchContent)
FetchContent_Declare(
        chalkboard
        GIT_REPOSITORY https://github.com/koftamainee/chalkboard.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(chalkboard)
set(CHALKBOARD_BUILD_VIEWER ON CACHE BOOL "" FORCE) # to build web viewer for html docs
set(CHALKBOARD_BUILD_EXAMPLES ON CACHE BOOL "" FORCE) # to build examples
```

## `LatexSerializable` concept

Any type used in math output must provide a free function `to_latex`:

```cpp
std::string to_latex(const MyType& v);
```

This includes primitive types — there is no built-in implementation for `int`, `double`, etc. You must provide them yourself. See [`examples/common.h`](examples/common.h) for a reference set of implementations.
