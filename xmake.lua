set_project( "stx"   )
set_version( "2.0.0" )
set_license( "MIT"   )

set_description("Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research.")

option( "use_modules" )
    set_default ( false )
    set_showmenu( true  )

target("stx")
    set_languages  ( "cxx23"  , { public = true })
    add_includedirs( "include", { public = true })
    add_headerfiles( "include/(lbyte/stx/*.hpp)" )
    add_headerfiles( "include/(lbyte/*.hpp)" )

    if has_config( "use_modules" ) then
        set_kind( "static" )
        set_policy( "build.c++.modules", true )

        add_files( "modules/stx/*.cppm", { public = true })
    else
        set_kind("headeronly")

    end
