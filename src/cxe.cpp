///usr/bin/env \
    sh $(dirname $0)/build.sh \
    -Wall -Werror -Wno-comment \
    -ferror-limit=2 \
    -fsanitize=address \
    -std=c++20 "$0" \
    --windows "-D_DISABLE_VECTOR_ANNOTATION" \
    --macos "-lstdc++" \
    "$@"; \
exit $?

#if defined(_WIN32)
#include <direct.h>
#include <process.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <new>
#include "cxe/buffer.hpp"
#include "cxe/command.hpp"
#include "cxe/context.hpp"
#include "cxe/environment.hpp"
#include "cxe/file.hpp"
#include "cxe/parser.hpp"
#include "cxe/path.hpp"
#include "cxe/print.hpp"
#include "cxe/scan.hpp"
#include "cxe/scope.hpp"
#include "cxe/shell.hpp"
#include "cxe/token.hpp"
#include "cxe/usage.hpp"

using namespace cxe;

bool streq(const char* a, const char* b) {
    verify(a);
    verify(b);
    return 0 == strcmp(a, b);
}

template<typename Span>
bool streq(const char* a, const Span& b) {
    verify(a);
    verify(b.data());
    verify(b.size());
    const size_t size = strlen(a);
    if (size != b.size()) return false;
    return 0 == strncmp(a, b.data(), size);
}

template<typename Span>
bool streq(const Span& a, const Span& b) {
    verify(a.data());
    verify(a.size());
    verify(b.data());
    verify(b.size());
    const size_t size = a.size();
    if (size != b.size()) return false;
    return 0 == strncmp(a.data(), b.data(), size);
}

bool has_arg(const char* const arg, const int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (streq(argv[i], arg)) return true;
    }
    return false;
}

//------------------------------------------------------------------------------

std::span<const char> span(const char* const str) {
    if (str) return { str, strlen(str) };
    return { "", size_t(0) };
}

std::span<const char> span(const buffer<char>& vec) {
    return span(vec.data());
}

//------------------------------------------------------------------------------

#if defined(_WIN32)

    enum : uint32_t { CP_UTF8 = 65001 };

    extern "C"
    int __stdcall
    SetConsoleOutputCP(uint32_t wCodePageID);

    #define chdir _chdir

#endif

//------------------------------------------------------------------------------

int main(const int argc, const char* argv[], const char* envp[]) {
    #ifdef _WIN32
        // enable UTF-8 output on Windows
        SetConsoleOutputCP(CP_UTF8);
    #endif

    // for (int i = 0; i < argc; ++i) echo(argv[i]);
    // puts("");
    // for (int i = 0; envp[i]; ++i) echo(envp[i]);

    if (argc < 2) {
        puts(USAGE);
        return 1;
    }

    for (int i = 0; i < argc; ++i) {
        auto arg = span(argv[i]);
        if (cxe::scan::equals("--help", arg)) {
            puts(USAGE);
            return 1;
        }
    }

    using namespace escape_codes;

    const buffer<char> cxe_path_buffer = [&]() -> auto {
        buffer<char> buf;
        buf << span(argv[0]);
        path::normalize(buf);
        if (scan::contains("/",buf)) {
            path::qualify(buf);
        } else {
            buffer<char> buf2;
            if (0 == shell::which(buf2, buf.data())) {
                buf = std::move(buf2);
            } else {
                error(1,{},"command not found: \"",buf,"\"");
            }
        }
        return buf;
    }();

    const token_t cxe_path = span(cxe_path_buffer);

    const token_t cxe_name = [&]() -> auto {
        using namespace ::cxe::scan;
        token_t name = cxe_path;
        while (seek("/", name) and skip("/", name));
        chop(".exe", name);
        return name;
    }();

    const buffer<char> src_path_buffer = [&]() -> auto {
        buffer<char> buf;
        buf << span(argv[1]);
        path::normalize(buf);
        path::qualify(buf);
        return buf;
    }();

    const token_t src_path = span(src_path_buffer);

    const token_t src_name = [&]() -> auto {
        using namespace ::cxe::scan;
        token_t name = src_path;
        while (seek("/", name) and skip("/", name));
        chop(".cpp", name, ignore_case) or
        chop(".cxx", name, ignore_case) or
        chop(".c++", name, ignore_case) or
        chop(".cc",  name, ignore_case) or
        chop(".c",   name, ignore_case);
        return name;
    }();

    const token_t src_dir = { src_path.data(), src_name.data() - 1 };

    const buffer<char> arg_buffer = [&]() -> auto {
        buffer<char> buf;
        buf.reserve(argc * 64);
        buf << cxe_name;
        for (int i = 1; i < argc; ++i) {
            if (buf[0])
                buf << " ";
            buf << argv[i];
        }
        return buf;
    }();

    if (not is_c_cpp_path(src_path)) {
        using namespace ::cxe::scan;
        location loc = {};
        itr_t arg_itr = arg_buffer.data();
        end_t arg_end = arg_itr + strlen(arg_itr);
        if (seek(src_path.data(), arg_itr, arg_end)) {
            const size_t offset = arg_itr - arg_buffer.data();
            loc = {
                .text = span(arg_buffer),
                .line = 1,
                .column = 1 + offset,
                .length = src_path.size(),
            };
        }
        error(1,loc,"expected C/C++ source file: ", src_path);
    }

    const buffer<char> src_buffer = [&]() -> auto {
        cxe::file f { src_path.data(), "r" };
        if (not f) {
            printf("file not found: %s\n", src_path.data());
            exit(1);
        }

        buffer<char> buf;

        const size_t start = f.tell();

        if (not f.seek(CXE_COMMENT_HEAD))
            return buf;

        if (not f.seek_and_skip(CXE_COMMENT_TAIL))
            return buf;

        const size_t size = f.tell() - start;
        buf.resize(size);

        f.set(start);
        f.read(buf.data(), size);
        return buf;
    }();

    const buffer<char> compiler_buffer = [&]() -> auto {
        using namespace ::cxe::scan;
        buffer<char> buf;
        if (is_cpp_path(src_path)) {
            // detect C++ compiler
            if (const char* const CXX = getenv("CXX"))    { buf << CXX; }
            else if (const char* const CC = getenv("CC")) { buf << CC;  }
            else if (0 == shell::which(buf, "clang"))     {}
            else if (0 == shell::which(buf, "gcc"))       {}
            else if (0 == shell::which(buf, "c++"))       {}
        }
        else if (is_c_path(src_path)) {
            // detect C compiler
            if (const char* const CC = getenv("CC"))  { buf << CC; }
            else if (0 == shell::which(buf, "clang")) {}
            else if (0 == shell::which(buf, "gcc"))   {}
            else if (0 == shell::which(buf, "cc"))    {}
        }
        else {
            location loc = {};
            itr_t arg_itr = arg_buffer.data();
            end_t arg_end = arg_itr + strlen(arg_itr);
            if (seek(src_path.data(), arg_itr, arg_end)) {
                const size_t offset = arg_itr - arg_buffer.data();
                loc = {
                    .text = span(arg_buffer),
                    .line = 1,
                    .column = 1 + offset,
                    .length = src_path.size(),
                };
            }
            error(1,loc,"compiler not found for source file: ", src_path);
        }

        path::normalize(buf);
        return buf;
    }();

    const context ctx {
        cxe_path,
        cxe_name,
        span(arg_buffer),
        span(src_buffer),
        src_path,
        src_name,
        span(compiler_buffer)
    };

    environment::variable CXE("CXE", cxe_path);
    environment::variable CXE_SRC_NAME("CXE_SRC_NAME", src_name);

    buffer<char> src_dir_buffer;
    src_dir_buffer << src_dir;
    path::set(src_dir_buffer.data());

    scope s = __func__;
    auto cmds = parser::parse(ctx);
    for (auto& cmd : cmds) {
        buffer<char> cmdline;
        for (const char* arg : cmd) {
            if (cmdline.size())
                cmdline << " ";
            cmdline << arg;
        }

        if (not scan::prefix(ctx.cxe_path, cmdline))
            println(cmdline.data());

        const int status = shell::run_argv(cmd.argv());
        if (status) { exit(status); }
    }

    return 0;
}