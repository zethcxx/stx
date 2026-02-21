set_project( "stx"   )
set_version( "1.0.0" )
set_license( "MIT"   )

set_description("Modern C++23 header-only systems toolbelt for low-level binary analysis, memory manipulation, and security research.")

target("stx")
    set_kind( "headeronly" )
    add_includedirs( "include", { public = true } )

    set_languages("cxx23")
    add_headerfiles(
        "include/stx/core.hpp" ,
        "include/stx/fn.hpp"   ,
        "include/stx/fs.hpp"   ,
        "include/stx/mem.hpp"  ,
        "include/stx/range.hpp",
        "include/stx/stx.hpp"  ,
        "include/stx/time.hpp"
    )
