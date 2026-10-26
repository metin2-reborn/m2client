# The embedded Python 2.7, built from source on every platform as a static library with the extension
# modules the game scripts need built in: the client is one executable, with no Python shared
# library beside it. The standard library it runs is the game directory's lib/ (python_launcher), so
# nothing of this install is needed at run time.
FetchContent_Declare(python-cmake-buildsystem
    GIT_REPOSITORY https://github.com/python-cmake-buildsystem/python-cmake-buildsystem.git
    GIT_TAG 39d95fd6f6bc78f4d7fa4217093281aa736517c9 # tag v2.7 (last with 2.7 support)
    SOURCE_SUBDIR this-directory-does-not-exist # built as an ExternalProject
)

FetchContent_MakeAvailable(python-cmake-buildsystem)

set(M2_PYTHON_PREFIX "${CMAKE_BINARY_DIR}/python27-install" CACHE INTERNAL "")
set(M2_PYTHON_TARBALL_URL "https://www.python.org/ftp/python/2.7.18/Python-2.7.18.tgz")
set(M2_PYTHON_TARBALL_SHA "da3080e3b488f648a3d7a4560ddee895284c3380b11d6de75edb986526b9a814")

# CPython 2.7 predates C99 strictness; newer compilers turn these into errors by default.
if(WIN32)
    set(M2_PYTHON_CFLAGS "")
    set(M2_PYTHON_PLATFORM_ARGS
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL # as the client (CMakeLists.txt)
        -DUSE_SYSTEM_ZLIB=OFF # the zlib in the CPython sources
        -DINSTALL_WINDOWS_TRADITIONAL=OFF
    )
    # The install layout differs: libs/python27.lib and include/.
    set(M2_PYTHON_LIBRARY "${M2_PYTHON_PREFIX}/libs/python27.lib")
    set(M2_PYTHON_INCLUDE "${M2_PYTHON_PREFIX}/include")
    set(M2_PYTHON_LINK_LIBRARIES "ws2_32;version;advapi32;shlwapi;winmm")
else()
    set(M2_PYTHON_CFLAGS "-fPIC -fno-strict-aliasing -O2 -g -std=gnu99 -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=return-type")
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        string(APPEND M2_PYTHON_CFLAGS " -Wno-error=incompatible-function-pointer-types -Wno-error=deprecated-non-prototype")
    endif()
    set(M2_PYTHON_PLATFORM_ARGS -DUSE_SYSTEM_ZLIB=ON)
    set(M2_PYTHON_LIBRARY "${M2_PYTHON_PREFIX}/lib/libpython2.7.a")
    set(M2_PYTHON_INCLUDE "${M2_PYTHON_PREFIX}/include/python2.7")
    set(M2_PYTHON_LINK_LIBRARIES "pthread;dl;util;m;z;expat")
endif()

ExternalProject_Add(python27_ep
    SOURCE_DIR "${python-cmake-buildsystem_SOURCE_DIR}"
    BINARY_DIR "${CMAKE_BINARY_DIR}/python27-build"
    INSTALL_DIR "${M2_PYTHON_PREFIX}"
    CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    "-DCMAKE_C_FLAGS=${M2_PYTHON_CFLAGS}"
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DPYTHON_VERSION=2.7.18
    -DDOWNLOAD_SOURCES=ON
    -DPY_URL=${M2_PYTHON_TARBALL_URL}
    -DPY_SHA256=${M2_PYTHON_TARBALL_SHA}
    -DBUILD_LIBPYTHON_SHARED=OFF
    -DBUILD_EXTENSIONS_AS_BUILTIN=ON
    -DBUILD_TESTING=OFF
    -DINSTALL_TEST=OFF
    -DINSTALL_MANUAL=OFF
    -DINSTALL_DEVELOPMENT=ON
    -DENABLE_SSL=OFF -DENABLE_HASHLIB=OFF
    -DENABLE_TKINTER=OFF -DENABLE_CURSES=OFF -DENABLE_CURSES_PANEL=OFF
    -DENABLE_BSDDB=OFF -DENABLE_SQLITE3=OFF -DENABLE_READLINE=OFF
    -DENABLE_DBM=OFF -DENABLE_GDBM=OFF -DENABLE_BZ2=OFF -DENABLE_CTYPES=OFF
    -DENABLE_NIS=OFF -DENABLE_CRYPT=OFF -DENABLE_SPWD=OFF -DENABLE_TERMIOS=OFF -DENABLE_RESOURCE=OFF
    -DENABLE_DECIMAL=ON -DENABLE_MULTIPROCESSING=OFF -DENABLE_SOCKET=ON
    -DENABLE_SELECT=ON -DENABLE_ZLIB=ON -DENABLE_PYEXPAT=ON -DENABLE_ELEMENTTREE=ON
    -DENABLE_UNICODEDATA=ON -DENABLE_CODECS_CN=ON -DENABLE_CODECS_HK=ON
    -DENABLE_CODECS_ISO2022=ON -DENABLE_CODECS_JP=ON -DENABLE_CODECS_KR=ON
    -DENABLE_CODECS_TW=ON -DENABLE_MULTIBYTECODEC=ON
    -DUSE_SYSTEM_LIBRARIES=OFF -DUSE_SYSTEM_EXPAT=OFF
    -DUSE_LIBEDIT=OFF
    ${M2_PYTHON_PLATFORM_ARGS}
    BUILD_BYPRODUCTS "${M2_PYTHON_LIBRARY}"
    USES_TERMINAL_BUILD TRUE
)

# The include directory must exist at configure time (CMake rejects imported targets
# whose INTERFACE_INCLUDE_DIRECTORIES are missing); the ExternalProject fills it later.
file(MAKE_DIRECTORY "${M2_PYTHON_INCLUDE}")

add_library(python27 STATIC IMPORTED GLOBAL)

set_target_properties(python27 PROPERTIES
    IMPORTED_LOCATION "${M2_PYTHON_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${M2_PYTHON_INCLUDE}"
    INTERFACE_LINK_LIBRARIES "${M2_PYTHON_LINK_LIBRARIES}"
)
if(WIN32)
    # Without it pyconfig.h declares the API dllimport, for python27.dll.
    set_target_properties(python27 PROPERTIES INTERFACE_COMPILE_DEFINITIONS Py_NO_ENABLE_SHARED)
endif()

add_dependencies(python27 python27_ep)
