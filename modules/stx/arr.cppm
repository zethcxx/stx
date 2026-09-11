module;
#define STX_MODULE_BUILD

#include "lbyte/stx/arr.hpp"

export module lbyte.stx.arr;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::arr;
    using ::lbyte::stx::arr_key;
    using ::lbyte::stx::dirty_arr;

    using ::lbyte::stx::arr_of;
}
