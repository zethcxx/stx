set_project( "stx"   )
set_version( "0.2.0" )
set_license( "MIT"   )

set_xmakever( "2.8.1" )

set_description("Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research.")

option( "use_modules" )
    set_default ( false )
    set_showmenu( true  )

option( "with_zou" )
    set_default ( true  )
    set_showmenu( true  )
    set_description( "Build zou (compile-time utilities: str, time, range)" )

-- ═══════════════════════════════════════════════════════════════════════════════
-- STX — core: types, memory, functions, filesystem, bit, endian, literals
-- ═══════════════════════════════════════════════════════════════════════════════

target("stx")
    set_languages  ( "cxx23"  , { public = true })
    add_includedirs( "include", { public = true })
    add_headerfiles( "include/(lbyte/stx/*.hpp)" )
    add_headerfiles( "include/(lbyte/stx.hpp)"   )

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

-- ═══════════════════════════════════════════════════════════════════════════════
-- ZOU — compile-time utilities: string literals, time, ranges
-- ═══════════════════════════════════════════════════════════════════════════════

target("zou")
    set_languages  ( "cxx23"  , { public = true })
    add_includedirs( "include", { public = true })
    add_headerfiles( "include/(lbyte/zou/*.hpp)" )
    add_headerfiles( "include/(lbyte/zou.hpp)" )

    if has_config( "use_modules" ) then
        set_kind( "static" )
        set_policy( "build.c++.modules", true )
        add_cxflags( "-U_FORTIFY_SOURCE" )

        add_files( "modules/zou/*.cppm", { public = true })
    else
        set_kind("headeronly")
    end

    add_deps( "stx" )

    on_load( function ( target )
        target:add( "includedirs", "include" )
        if has_config( "use_modules" ) then
            target:add( "cxxmodules", "modules/zou/*.cppm" )
        end
    end)
