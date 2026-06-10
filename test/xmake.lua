set_project("stx_tests")
set_version("1.0.0")
set_xmakever("2.8.0")

--: Includes --------------------------------------------
includes("xmake/rules/compile_commands.lua")
includes("xmake/actions.lua")

add_requires("catch2 v3.13.0")

-- add_defines("__has_feature(x)=0")
-- add_defines("__building_module(x)=0")
add_defines("_POSIX_SEM_VALUE_MAX=2147483647")
add_cxflags("-pthread")
add_cxflags("-std=c++23")

local stx_dir = path.join(os.scriptdir(), "..")

local function register_test(name, file_path)
    target( name )
        set_kind     ( "binary" )
        set_languages( "c++23"  )
        set_default  ( false    )

        add_packages( "catch2" )

        add_files( file_path )
        add_includedirs( path.join( stx_dir, "include" ))

        on_config ( act.configure   )
        on_prepare( act.print_info  )
        on_run    ( act.run_process )
    target_end()
end

register_test("test-core"    , "tests/core.cpp"    )
register_test("test-mem"     , "tests/mem.cpp"     )
register_test("test-fn"      , "tests/fn.cpp"      )
register_test("test-range"   , "tests/range.cpp"   )
register_test("test-time"    , "tests/time.cpp"    )
register_test("test-fs"      , "tests/fs.cpp"      )
register_test("test-literals", "tests/literals.cpp")
register_test("test-endian"  , "tests/endian.cpp"  )
register_test("test-bit"     , "tests/bit.cpp"     )
register_test("test-str"     , "tests/str.cpp"     )

target("all")
    set_kind     ( "binary" )
    set_languages( "c++23"  )
    set_default  ( true     )

    add_packages("catch2")
    add_files("tests/*.cpp")
    add_includedirs(path.join(stx_dir, "include"))

    on_config ( act.configure   )
    on_prepare( act.print_info  )
    on_run    ( act.run_process )

target_end()

