# Compiler flags for the game code.
set(M2_GAME_COMPILE_OPTIONS
    -fno-strict-aliasing
    -fwrapv
)
# Warnings for the game libraries, the client and the tools.
set(M2_CLEAN_WARNINGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers $<$<CONFIG:Debug>:-Werror>)
set(M2_GAME_COMPILE_DEFINITIONS
    NDEBUG # assert() is compiled out, as in the original release build
)
# Development tools (the automation harness) exist only for developing and testing the client. Debug and RelWithDebInfo builds have them; Release and
# MinSizeRel builds compile them out (M2_DEV_TOOLS=0), with no way to turn them back on.
if(CMAKE_BUILD_TYPE MATCHES "^(Release|MinSizeRel)$")
    set(M2_DEV_TOOLS 0)
else()
    set(M2_DEV_TOOLS 1)
endif()
add_compile_definitions(M2_DEV_TOOLS=${M2_DEV_TOOLS})
message(STATUS "Development tools (automation harness): ${M2_DEV_TOOLS}")
# Executables export their symbols so the crash handler's backtraces carry function names.
set(M2_EXE_LINK_OPTIONS -rdynamic)
