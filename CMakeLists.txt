cmake_minimum_required( VERSION 3.15 )
project( stx
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research."
)

add_library( stx INTERFACE )
add_library( stx::stx ALIAS stx )

target_compile_features( stx INTERFACE cxx_std_23)

target_include_directories( stx INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install( DIRECTORY include/ DESTINATION include )

install( TARGETS stx EXPORT stxTargets )
install( EXPORT stxTargets
    FILE stxConfig.cmake
    NAMESPACE stx::
    DESTINATION lib/cmake/stx
)
