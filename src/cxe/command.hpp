#pragma once
#include "token.hpp"
#include "environment.hpp"
#include "scope.hpp"
#include "shell.hpp"
#include "verify.hpp"

namespace cxe {

    template<typename T>
    T& reset(T& src) { return *new(&src) T(); }

    template<typename T>
    T& move(T* dst, T& src) {
        if (dst != &src) { dst->~T(); new(dst) T(std::move(src)); }
        return *dst;
    }

    class command {

        using this_t = command;

        buffer<char*> _argv;

        static char* argalloc(const char* src, const size_t len) {
            verify(src);
            verify(src[0]);
            char* const dst = (char*)calloc(len + 1, sizeof(char));
            strncpy(dst, src, len);
            verify(dst[len] == 0);
            return dst;
        }

    public:

        ~command() { for (char* p : _argv) { if (p) free(p); } }

        command() = default;

        command(this_t&& src) : _argv(std::move(src._argv)) { reset(src); }

        this_t& operator=(this_t&& src) { return move(this, src); }

        explicit operator bool() const { return _argv.size(); }

        bool empty() const { return _argv.empty(); }

        auto begin() { return _argv.begin(); }

        auto end() { return _argv.end(); }

        char** argv() { return _argv.data(); }

        template<typename Src>
        const char* append(const Src& src) {
            char* const arg = argalloc(src.data(), src.size());
            _argv.push_back(arg);
            return arg;
        }

        using match_t = bool(*)(const token_t& a, const token_t& b);

        const char* find(match_t match, const token_t& expect) const {
            for (const char* arg : _argv)
                if (match(expect, token_t(arg, strlen(arg))))
                    return arg;
            return nullptr;
        }
    };

    //--------------------------------------------------------------------------

    class commands {
        using this_t = commands;

        buffer<command*> _cmds;

    public:

        ~commands() { for (auto* cmd : _cmds) delete cmd; }

        commands() = default;

        commands(this_t&& src) : _cmds(std::move(src._cmds)) { reset(src); }

        this_t& operator=(this_t&& src) { return move(this, src); }

        explicit operator bool() const { return _cmds.size(); }

        size_t size() const { return _cmds.size(); }

        auto begin() const { return _cmds.begin(); }
        auto   end() const { return _cmds.end(); }

        command& append() {
            command* const cmd = new command();
            _cmds.push_back(cmd);
            return *cmd;
        }
    };

} // namespace cxe