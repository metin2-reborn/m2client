# The game code: one static library with everything under metin2/ except the entry point, so the
# client, the tools and the tests link the same objects.
file(GLOB_RECURSE M2_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/metin2/*.cpp")
list(FILTER M2_SOURCES EXCLUDE REGEX "/metin2/main\\.cpp$")

add_library(metin2 STATIC ${M2_SOURCES})
# Game code includes its headers as <metin2/...>; the definitions and options apply to everything
# that links the library.
target_include_directories(metin2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(metin2 PUBLIC ${M2_GAME_COMPILE_DEFINITIONS})
target_compile_options(metin2 PUBLIC ${M2_GAME_COMPILE_OPTIONS})
target_link_libraries(metin2 PUBLIC
    opengr2ndma
    python27 lzo2 monocypher tomlplusplus asio
    raylib OpenAL minimp3 dr_libs freetype ZLIB::ZLIB Threads::Threads ${CMAKE_DL_LIBS})
if(WIN32)
    target_link_libraries(metin2 PUBLIC dbghelp) # the crash handler's symbols
else()
    target_link_libraries(metin2 PUBLIC PkgConfig::FONTCONFIG)
endif()
target_compile_options(metin2 PRIVATE ${M2_CLEAN_WARNINGS})
# The window code calls GLFW (raylib's window backend) for the pointer lock and custom cursors, and
# the OpenGL renderer calls GL through raylib's glad; the cursor files are read from the source tree.
FetchContent_GetProperties(raylib)
target_include_directories(metin2 PRIVATE "${raylib_SOURCE_DIR}/src/external/glfw/include" "${raylib_SOURCE_DIR}/src")
target_compile_definitions(metin2 PRIVATE "M2_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\"")
# The Python headers are installed by the Python build.
add_dependencies(metin2 python27_ep)

add_executable(client metin2/main.cpp)
target_link_libraries(client PRIVATE metin2)
target_compile_options(client PRIVATE ${M2_CLEAN_WARNINGS})
target_link_options(client PRIVATE ${M2_EXE_LINK_OPTIONS})
set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
