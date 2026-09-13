module;

#define STX_MODULE_BUILD
#include "lbyte/stx/ct/record.hpp"

export module lbyte.stx.ct.record;

import lbyte.stx.core;
import lbyte.stx.mem;
export import lbyte.stx.arr;

export namespace lbyte::stx::ct
{
    namespace attr
    {
        using ::lbyte::stx::ct::attr::packed;
        using ::lbyte::stx::ct::attr::align;
        using ::lbyte::stx::ct::attr::gap;
    }

    using ::lbyte::stx::ct::record_key;
    using ::lbyte::stx::ct::key_str;
    using ::lbyte::stx::ct::member;
    using ::lbyte::stx::ct::smember;
    using ::lbyte::stx::ct::record;
    using ::lbyte::stx::ct::record_value_of_t;

    using ::lbyte::stx::ct::record_view;
    using ::lbyte::stx::ct::view;

    using ::lbyte::stx::ct::loader;
    using ::lbyte::stx::ct::store_impl;

    using ::lbyte::stx::ct::load;
    using ::lbyte::stx::ct::store;
}