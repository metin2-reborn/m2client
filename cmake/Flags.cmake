# Compiler flags for the game code. Clang is the compiler on every platform: clang++ on Linux,
# clang-cl (the MSVC-compatible driver) on Windows, where GCC-style code generation options go
# through /clang: and the warning options are taken as they are.
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(M2_GAME_COMPILE_OPTIONS
        /clang:-fno-strict-aliasing
        /clang:-fwrapv
        /utf-8 # the sources and the game data are UTF-8
    )
    set(M2_CLEAN_WARNINGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers $<$<CONFIG:Debug>:/WX>)
else()
    set(M2_GAME_COMPILE_OPTIONS
        -fno-strict-aliasing
        -fwrapv
    )
    set(M2_CLEAN_WARNINGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers $<$<CONFIG:Debug>:-Werror>)
endif()
set(M2_GAME_COMPILE_DEFINITIONS
    NDEBUG # assert() is compiled out, as in the original release build
)
if(WIN32)
    list(APPEND M2_GAME_COMPILE_DEFINITIONS
        NOMINMAX # windows.h must not define min/max
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS # the C runtime's fopen/strncpy are used as the standard defines them
        _CRT_NONSTDC_NO_WARNINGS
    )
endif()
# Development tools (the automation harness) exist only for developing and testing the client. Debug and RelWithDebInfo builds have them; Release and
# MinSizeRel builds compile them out (M2_DEV_TOOLS=0), with no way to turn them back on.
if(CMAKE_BUILD_TYPE MATCHES "^(Release|MinSizeRel)$")
    set(M2_DEV_TOOLS 0)
else()
    set(M2_DEV_TOOLS 1)
endif()
add_compile_definitions(M2_DEV_TOOLS=${M2_DEV_TOOLS})
message(STATUS "Development tools (automation harness): ${M2_DEV_TOOLS}")
# Executables export their symbols so the crash handler's backtraces carry function names (on
# Windows the crash handler reads the PDB instead).
if(WIN32)
    set(M2_EXE_LINK_OPTIONS)
else()
    set(M2_EXE_LINK_OPTIONS -rdynamic)
endif()
