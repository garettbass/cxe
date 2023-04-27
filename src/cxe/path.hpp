#pragma once
#include <ctype.h>
#include <stdlib.h>
#include "buffer.hpp"
#include "scan.hpp"
#include "verify.hpp"

#ifdef _WIN32
    #include <direct.h> // _chdir
#else
    #include <unistd.h> // chdir
#endif

namespace cxe::path {

    bool absolute(const char* path) {
        if (0 == path) return false;

        using namespace ::cxe::scan;
        end_t end = path + strlen(path);
        if (skip('/', path, end)) return true;
        if (skip('\\', path, end)) return true;

        if (skip(isalpha, path, end)) {
            if (skip(':', path, end)) return true; // e.g. c:...

            itr_t colon = path;
            if (not seek(':', colon, end)) return false;

            itr_t slash = path;
            if (seek('/', slash, end)) {
                if (slash < colon) return false;
            }

            if (seek('\\', slash, end)) {
                if (slash < colon) return false;
            }

            return true; // e.g. file:/...
        }

        return false;
    }

    bool relative(const char* path) {
        return path and not absolute(path);
    }

    bool exists(const char* path);

    void normalize(buffer<char>& path) {
        if (path.empty()) return;

        bool has_whitespace = false;
        for (char& ch : path) {
            // normalize path separators
            if (ch == '\\') { ch = '/'; continue; }
            // detect whitespace in path
            has_whitespace = has_whitespace || isspace(ch);
        }

        if (has_whitespace) {
            // quote path to preserve whitespace
            if (path.front() != '"') '"' >> path;
            if (path.back()  != '"') path << '"';
        }
    }

    void qualify(buffer<char>& path) {
        if (absolute(path.data())) return;

        #ifdef _WIN32

            char* abs_path = _fullpath(nullptr, path.data(), 0);
            path.clear();
            path << abs_path;
            free(abs_path);
            normalize(path);

        #else

            char abs_path[PATH_MAX + 1] = {0};
            realpath(path.data(), abs_path);
            path.clear();
            path << abs_path;
            normalize(path);

        #endif
    }

    // void get(buffer<char>& path) {
    //     path.resize(PATH_MAX);
    //     getcwd(path.data(), path.size());
    //     path.truncate();
    // }

    bool set(const char* path) {

        #ifdef _WIN32

            return 0 == _chdir(path);

        #else

            return 0 == chdir(path);

        #endif

    }

} // namespace cxe::path