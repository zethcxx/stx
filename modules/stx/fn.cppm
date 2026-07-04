module;
#define STX_MODULE_BUILD

// Global module fragment
#include "lbyte/stx/fn.hpp"

export module lbyte.stx.fn;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::caller_t;
    using ::lbyte::stx::caller;
}

