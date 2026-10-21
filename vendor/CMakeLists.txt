# Third-party dependencies. Everything not taken from the system is fetched with FetchContent and pinned
# to an exact tag, commit or checksum. Licenses are in licenses/.
include(FetchContent)
include(ExternalProject)

set(FETCHCONTENT_QUIET OFF)
# Several upstream projects still declare cmake_minimum_required < 3.5.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
# Imported targets found here are used by the game code in other directories.
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)

include(system.cmake)
include(raylib.cmake)
include(openal.cmake)
include(minimp3.cmake)
include(dr_libs.cmake)
include(opengr2ndma.cmake)
include(monocypher.cmake)
include(lzo2.cmake)
include(freetype.cmake)
include(python27.cmake)
include(tomlplusplus.cmake)
include(asio.cmake)
