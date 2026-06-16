cmake_minimum_required(VERSION 3.28)

project( stx
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research."
)

option( LBYTE_STX_USE_MODULES "Enable C++20 modules support" OFF )
option( LBYTE_STX_BUILD_ZOU  "Build zou (compile-time utilities: str, time, range)" ON )

# ═══════════════════════════════════════════════════════════════════════════════
# STX — core: types, memory, functions, filesystem, bit, endian, literals
# ═══════════════════════════════════════════════════════════════════════════════

if ( LBYTE_STX_USE_MODULES )
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
        modules/stx/mem.cppm
        modules/stx/fn.cppm
        modules/stx/io.cppm
        modules/stx/bit.cppm
        modules/stx/endian.cppm
        modules/stx/literals.cppm
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
# ZOU — compile-time utilities: string literals, time, ranges
# ═══════════════════════════════════════════════════════════════════════════════

if ( LBYTE_STX_BUILD_ZOU )
    if ( LBYTE_STX_USE_MODULES )
        add_library( zou STATIC )
        target_compile_features( zou PUBLIC cxx_std_23 )

        target_include_directories( zou
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
        )

        target_sources( zou
        PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        BASE_DIRS modules/zou
        FILES
            modules/zou/str.cppm
            modules/zou/time.cppm
            modules/zou/range.cppm
            modules/zou/zou.cppm
        )

        target_link_libraries( zou PUBLIC stx )
    else()
        add_library(zou INTERFACE)
        target_compile_features(zou INTERFACE cxx_std_23)

        target_include_directories(zou
            INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
        )

        target_link_libraries(zou INTERFACE lbyte::stx)
    endif()

    add_library(lbyte::zou ALIAS zou)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Install
# ═══════════════════════════════════════════════════════════════════════════════

if ( LBYTE_STX_USE_MODULES )
    install(TARGETS stx
        EXPORT stxTargets
        CXX_MODULES_SET CXX_MODULES DESTINATION lib/stx/modules
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

if ( LBYTE_STX_BUILD_ZOU )
    if ( LBYTE_STX_USE_MODULES )
        install(TARGETS zou
            EXPORT zouTargets
            CXX_MODULES_SET CXX_MODULES DESTINATION lib/zou/modules
            INCLUDES DESTINATION include
        )
    else()
        install(TARGETS zou
            EXPORT zouTargets
            INCLUDES DESTINATION include
        )
    endif()

    install(EXPORT zouTargets
        FILE lbyte-zou-config.cmake
        NAMESPACE lbyte::
        DESTINATION lib/cmake/lbyte-zou
    )
endif()
