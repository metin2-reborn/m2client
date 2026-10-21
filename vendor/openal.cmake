# OpenAL Soft 1.25.2: audio output for the Miles layer.
set(LIBTYPE STATIC CACHE STRING "" FORCE)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_UPDATE_BUILD_VERSION OFF CACHE BOOL "" FORCE)
set(ALSOFT_ENABLE_MODULES OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_OSS OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_JACK OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_PORTAUDIO OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_SNDIO OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_SDL2 OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_SDL3 OFF CACHE BOOL "" FORCE)

FetchContent_Declare(openal
    GIT_REPOSITORY https://github.com/kcat/openal-soft.git
    GIT_TAG b2c48f7718ef3fcf67921a8b6534c4914e328970 # tag 1.25.2
    GIT_SHALLOW FALSE
)

FetchContent_MakeAvailable(openal)

# OpenAL Soft 1.25.2 enables -Werror=function-effects, which Clang 23 trips on with libstdc++ 16.
if(TARGET OpenAL)
    target_compile_options(OpenAL PRIVATE -Wno-function-effects -Wno-error)

    # Drop "<VAR>-NOTFOUND" entries that OpenAL's optional backend probes leave in its link interface.
    foreach(prop LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
        get_target_property(_libs OpenAL ${prop})
        if(_libs)
            list(FILTER _libs EXCLUDE REGEX "-NOTFOUND$")
            set_target_properties(OpenAL PROPERTIES ${prop} "${_libs}")
        endif()
    endforeach()
endif()
