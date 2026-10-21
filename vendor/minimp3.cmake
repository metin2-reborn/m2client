# minimp3: MP3 decoding (header only).
FetchContent_Declare(minimp3
    GIT_REPOSITORY https://github.com/lieff/minimp3.git
    GIT_TAG ea99364f61c14656440e8d77e9c233ccf3124633
)

FetchContent_MakeAvailable(minimp3)

add_library(minimp3 INTERFACE)
target_include_directories(minimp3 SYSTEM INTERFACE "${minimp3_SOURCE_DIR}")
