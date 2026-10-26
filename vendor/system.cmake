# Libraries from the system. Linux takes them from the distribution; Windows has no package manager
# to rely on, so zlib is built from source there and fonts are found without fontconfig.
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

if(WIN32)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG 51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf # tag v1.3.1
    )
    FetchContent_MakeAvailable(zlib)
    # zconf.h is generated into the binary directory.
    target_include_directories(zlibstatic PUBLIC "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET GLOBAL fontconfig)
    find_package(ZLIB REQUIRED)
endif()
