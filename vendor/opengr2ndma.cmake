set(OPENGR2NDMA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(OPENGR2NDMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(opengr2ndma
    GIT_REPOSITORY https://github.com/cemreefe/OpenGr2ndma.git
    GIT_TAG de364b5669e3dc899956ce6c18632b7bc053bcb0
)

FetchContent_MakeAvailable(opengr2ndma)
