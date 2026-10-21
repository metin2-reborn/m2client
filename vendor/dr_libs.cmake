FetchContent_Declare(dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG dfe8377631000664666519fdb83da193fd8037f4
)

FetchContent_MakeAvailable(dr_libs)

add_library(dr_libs INTERFACE)
target_include_directories(dr_libs SYSTEM INTERFACE "${dr_libs_SOURCE_DIR}")
