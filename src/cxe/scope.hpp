#pragma once
#include "verify.hpp"
#include "print.hpp"

#ifndef CXE_SCOPE_ENABLED
#define CXE_SCOPE_ENABLED 0
#endif // CXE_SCOPE_ENABLED

namespace cxe {

    class scope {
        #if CXE_SCOPE_ENABLED
        static inline size_t _depth = 0;
        const char* name = nullptr;
        #endif // CXE_SCOPE_ENABLED

    public:

        ~scope() {
            #if CXE_SCOPE_ENABLED
            verify(_depth > size_t(0));
            _depth -= 1;
            println("}");
            #endif // CXE_SCOPE_ENABLED
        }

        scope(const char* name) {
            #if CXE_SCOPE_ENABLED
            this->name = name;
            verify(_depth < ~size_t(0));
            println(name," {");
            _depth += 1;
            #endif // CXE_SCOPE_ENABLED
        }

        void indent() const {
            #if CXE_SCOPE_ENABLED
            for (int i = 0; i < _depth; ++i) cxe::print("    ");
            #endif // CXE_SCOPE_ENABLED
        }

        template<typename... Args>
        void println(const Args&... args) const {
            #if CXE_SCOPE_ENABLED
            indent(); cxe::println(args...);
            #endif // CXE_SCOPE_ENABLED
        }
    };

} // namespace cxe
