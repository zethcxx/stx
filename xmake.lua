set_project( "stx"   )
set_version( "1.0.0" )
set_license( "MIT"   )

set_xmakever( "2.8.1" )

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
        add_cxflags( "-U_FORTIFY_SOURCE" )

        add_files( "modules/stx/*.cppm", { public = true })
    else
        set_kind("headeronly")

    end

    on_install( function ( package )
        local includedir = package:installdir("include")
        os.cp( "include/lbyte/stx.hpp",  includedir .. "/lbyte"      )
        os.cp( "include/lbyte/stx/*.hpp", includedir .. "/lbyte/stx" )

        if package:config( "use_modules" ) then
            import("package.tools.xmake").install( package )
        end
    end)

    on_load( function ( package )
        package:add( "includedirs", "include" )
        if package:config( "use_modules" ) then
            package:add( "cxxmodules", "modules/stx/*.cppm" )
        end
    end)

