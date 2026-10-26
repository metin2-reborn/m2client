# LZO 2 (the compressed sections of the old game archives and the guild mark transfer): the
# distribution's library on Linux, built from the source release on Windows.
if(WIN32)
    FetchContent_Declare(lzo
        URL https://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz
        URL_HASH SHA256=c0f892943208266f9b6543b3ae308fab6284c5c90e627931446fb49b4221a072
        SOURCE_SUBDIR metin2-unused # built below
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(lzo)
    file(GLOB LZO_SOURCES "${lzo_SOURCE_DIR}/src/*.c")
    add_library(lzo2 STATIC ${LZO_SOURCES})
    target_include_directories(lzo2 SYSTEM PUBLIC "${lzo_SOURCE_DIR}/include")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LZO2 REQUIRED IMPORTED_TARGET GLOBAL lzo2)
    add_library(lzo2 INTERFACE)
    target_link_libraries(lzo2 INTERFACE PkgConfig::LZO2)
endif()
