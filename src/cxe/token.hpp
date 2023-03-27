#pragma once
#include "verify.hpp"
#include "buffer.hpp"
#include "scan.hpp"

namespace cxe {

    using token_t = std::span<const char>;

    using tokens_t = cxe::buffer<token_t>;

    bool operator==(const char* a, const token_t& b) {
        return cxe::scan::equals(a, b);
    }

    bool operator==(const token_t& a, const char* b) {
        return cxe::scan::equals(b, a);
    }

    bool operator==(const token_t& a, const token_t& b) {
        return cxe::scan::equals(a, b);
    }

}
