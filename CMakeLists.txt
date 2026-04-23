cmake_minimum_required(VERSION 3.28)

project( stx
    VERSION 2.1.0
    LANGUAGES CXX
    DESCRIPTION "Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research."
)

option( LBYTE_STX_USE_MODULES "Enable C++20 modules support" OFF )

if ( LBYTE_STX_USE_MODULES )
    add_library( stx STATIC )
    target_compile_features( stx PUBLIC cxx_std_23 )

    target_sources( stx
    PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        BASE_DIRS modules/stx
        FILES
            modules/stx/core.cppm
            modules/stx/mem.cppm
            modules/stx/time.cppm
            modules/stx/fn.cppm
            modules/stx/range.cppm
            modules/stx/fs.cppm
            modules/stx/dump.cppm
            modules/stx/stx.cppm
    )
else()
    add_library(stx INTERFACE)
    target_compile_features(stx INTERFACE cxx_std_23)
endif()

add_library(lbyte::stx ALIAS stx)

target_include_directories(stx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


install(TARGETS stx
    EXPORT stxTargets

    CXX_MODULES_SET CXX_MODULES DESTINATION lib/stx/modules
    INCLUDES DESTINATION include
)

install(EXPORT stxTargets
    FILE lbyte-stx-config.cmake
    NAMESPACE lbyte::
    DESTINATION lib/cmake/lbyte-stx
)
