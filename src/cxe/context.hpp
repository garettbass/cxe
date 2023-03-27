#pragma once
#include "verify.hpp"
#include "token.hpp"

namespace cxe {

    struct location {
        using span_t = std::span<const char>;
        span_t file   {};
        span_t text   {};
        size_t line   {}; // one-based
        size_t column {}; // one-based
        size_t length {};

        explicit operator bool() const { return line > 0; }
    };

    //--------------------------------------------------------------------------

    struct source {
        buffer<char> file {};
        buffer<char> text {};
        size_t       line {}; // one-based

        explicit operator bool() const { return line > 0; }
    };

    //--------------------------------------------------------------------------

    struct context {
        using span_t = std::span<const char>;
        const span_t cxe_path;
        const span_t cxe_name;
        const span_t cli_text;
        const span_t src_text;
        const span_t src_path;
        const span_t src_name;
        const span_t compiler_path;
        const bool compiler_is_clang;
        const bool compiler_is_gcc;

        context(
            span_t cxe_path,
            span_t cxe_name,
            span_t cli_text,
            span_t src_text,
            span_t src_path,
            span_t src_name,
            span_t compiler_path
        )
        : cxe_path(cxe_path)
        , cxe_name(cxe_name)
        , cli_text(cli_text)
        , src_text(src_text)
        , src_path(src_path)
        , src_name(src_name)
        , compiler_path(compiler_path)
        , compiler_is_clang (scan::contains("clang", compiler_path))
        , compiler_is_gcc   (scan::contains("gcc",   compiler_path)) {}

        context(const context&) = delete;
        context& operator=(const context&) = delete;

        static bool contains(const token_t& t, const span_t& s) {
            return scan::contains(t, s);
        }

        bool contains(const token_t& t) const {
            return contains(t, cli_text) or contains(t, src_text);
        }

        location operator[](token_t t) const { return locate(t); }

        location locate(token_t t) const {
            verify(contains(t));
            const std::span<const char> cmd_or_src =
                contains(t, cli_text) ? cli_text :
                contains(t, src_text) ? src_text :
                std::span<const char>{};

            if (cmd_or_src.empty()) return {};

            using namespace cxe::scan;
            itr_t itr = cmd_or_src.data();
            itr_t text_ptr = itr;
            size_t line = 1, column = 1;
            for (end_t end = t.data(); itr < end;) {
                if (skip("\r\n", itr, end) or skip("\n", itr, end)) {
                    text_ptr = itr;
                    line += 1;
                    column = 1;
                    continue;
                } else {
                    column += 1;
                    itr += 1;
                }
            }
            const char* text_end = text_ptr;
            for (end_t end = cmd_or_src.data() + cmd_or_src.size(); itr < end;) {
                if (prefix("\r\n", itr, end) or prefix("\n", itr, end)) {
                    break;
                } else {
                    ++itr;
                }
            }
            text_end = itr;
            token_t text { text_ptr, text_end };
            return (cmd_or_src == cli_text)
                ? location { token_t(), text, line, column, t.size() }
                : location { src_path,  text, line, column, t.size() };
        }

    };

    template<typename... Args>
    void diagnostic(location loc, const Args&... args) {
        using namespace escape_codes;
        if (loc.file.size()) {
            print(WHITE);
            print(loc.file,":",loc.line,":",loc.column,": ");
            print(RESET);
        }
        println(args...);
        if (loc.text.size()) {
            println(loc.text);
            for (size_t i = 1; i < loc.column; ++i) {
                print(" ");
            }
            print(LTGREEN,"^");
            for (size_t i = 1; i < loc.length; ++i) {
                print("~");
            }
            println(RESET);
        }
    }

    template<typename... Args>
    void warning(location loc, const Args&... args) {
        using namespace escape_codes;
        diagnostic(loc,LTYELLOW,"warning: ",RESET,args...);
    }

    template<typename... Args>
    void note(location loc, const Args&... args) {
        using namespace escape_codes;
        diagnostic(loc,DKGREY,"note: ",RESET,args...);
    }

    template<typename... Args>
    void error(location loc, const Args&... args) {
        using namespace escape_codes;
        diagnostic(loc,LTRED,"error: ",RESET,args...);
    }

    template<typename... Args>
    void error(int exit_code, location loc, const Args&... args) {
        error(loc, args...);
        if (exit_code) exit(exit_code);
    }

} // namespace cxe