cmake_minimum_required(VERSION 3.25)
project(metin2client LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Windows: the release C runtime in every configuration, as the Python 2.7 DLL is linked against it
# (its headers want python27_d.lib under _DEBUG, which does not exist).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

option(M2_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(M2_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-sanitize=alignment -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(vendor)

include(cmake/Flags.cmake)

add_subdirectory(src)
add_subdirectory(tools)

enable_testing()

add_subdirectory(tests)
