cmake_minimum_required(VERSION 3.28)

project( stx
    VERSION 0.3.0
    LANGUAGES CXX
    DESCRIPTION "Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research."
)

option( LBYTE_STX_USE_MODULES "Enable C++20 modules support" OFF )

# ═══════════════════════════════════════════════════════════════════════════════
# STX — all modules
# ═══════════════════════════════════════════════════════════════════════════════

if ( LBYTE_STX_USE_MODULES )
    if ( NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio" )
        message( FATAL_ERROR "LBYTE_STX_USE_MODULES requires a module-capable generator: use -G Ninja (or a Visual Studio generator on Windows)" )
    endif()

    add_library( stx STATIC )
    target_compile_features( stx PUBLIC cxx_std_23 )

    target_include_directories(stx
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_sources( stx
    PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    BASE_DIRS modules/stx
    FILES
        modules/stx/core.cppm
        modules/stx/arr.cppm
        modules/stx/mem.cppm
        modules/stx/fn.cppm
        modules/stx/io.cppm
        modules/stx/bit.cppm
        modules/stx/endian.cppm
        modules/stx/literals.cppm
        modules/stx/ct.cppm
        modules/stx/ct.str.cppm
        modules/stx/ct.record.cppm
        modules/stx/time.cppm
        modules/stx/range.cppm
        modules/stx/cycle.cppm
        modules/stx/stx.cppm
    )
else()
    add_library(stx INTERFACE)
    target_compile_features(stx INTERFACE cxx_std_23)

    target_include_directories(stx
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
endif()

add_library(lbyte::stx ALIAS stx)

# ═══════════════════════════════════════════════════════════════════════════════
# Install
# ═══════════════════════════════════════════════════════════════════════════════

if ( LBYTE_STX_USE_MODULES )
    install(TARGETS stx
        EXPORT stxTargets
        FILE_SET CXX_MODULES DESTINATION lib/stx/modules
        INCLUDES DESTINATION include
    )
else()
    install(TARGETS stx
        EXPORT stxTargets
        INCLUDES DESTINATION include
    )
endif()

install(EXPORT stxTargets
    FILE lbyte-stx-config.cmake
    NAMESPACE lbyte::
    DESTINATION lib/cmake/lbyte-stx
)
