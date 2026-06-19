set_project( "stx"   )
set_version( "0.2.0" )
set_license( "MIT"   )

set_xmakever( "2.8.1" )

set_description("Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research.")

add_requires("ctre v3.10.0")

option( "use_modules" )
    set_default ( false )
    set_showmenu( true  )

-- ═══════════════════════════════════════════════════════════════════════════════
-- STX — all modules
-- ═══════════════════════════════════════════════════════════════════════════════

target("stx")
    set_languages  ( "cxx23"  , { public = true })
    add_includedirs( "include", { public = true })
    add_headerfiles( "include/(lbyte/stx/*.hpp)" )
    add_headerfiles( "include/(lbyte/stx.hpp)"   )
    add_packages   ( "ctre"   )

    if has_config( "use_modules" ) then
        set_kind( "static" )
        set_policy( "build.c++.modules", true )
        add_cxflags( "-U_FORTIFY_SOURCE" )

        add_files( "modules/stx/*.cppm", { public = true })
    else
        set_kind("headeronly")
    end

    on_load( function ( target )
        target:add( "includedirs", "include" )
        if has_config( "use_modules" ) then
            target:add( "cxxmodules", "modules/stx/*.cppm" )
        end
    end)
