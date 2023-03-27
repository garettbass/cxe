#pragma once
#include <stdio.h>
#include "verify.hpp"

namespace cxe {

    struct file {

        using close_t = int (*)(FILE*);

        FILE* const stream = nullptr;

        close_t const closef = nullptr;

        ~file() { close(); }

        file() = default;

        file(FILE* stream, close_t closef) : stream(stream), closef(closef) {}

        file(const char* filename, const char* mode)
        : file(::fopen(filename, mode), ::fclose) {}

        static file fopen(const char* filename, const char* mode) {
            return { ::fopen(filename, mode), ::fclose };
        }

        static file popen(const char* command) {
            #if defined(_WIN32)
                return { ::_popen(command, "r"), ::_pclose };
            #else
                return { ::popen(command, "r"), ::pclose };
            #endif

        }

        int close() {
            int result = stream ? closef(stream) : -1;
            new(this) file();
            return result;
        }

        // move constructor
        file(file&& src) : stream(src.stream) { new(&src) file(); }

        file& operator=(file&& src) {
            if (this != &src) {
                this->~file();
                new(this) file(std::move(src));
            }
            return *this;
        }

        explicit operator bool() const { return not eof(); }

        operator FILE*() { return stream; }

        bool closed() const { return stream == nullptr; }

        bool eof() const { return closed() || feof(stream); }

        size_t size() const {
            if (not stream) return 0;
            const size_t start = ftell(stream);
            fseek(stream, 0, SEEK_END);
            const size_t size = ftell(stream);
            fseek(stream, start, SEEK_SET);
            return size;
        }

        size_t tell() const { return stream ? ftell(stream) : 0; }

        bool set(size_t offset) {
            if (not stream) return false;

            return 0 == fseek(stream, offset, SEEK_SET);
        }

        template<size_t N>
        size_t read(char (&dst)[N]) {
            static_assert(N > 1);
            if (eof()) return 0;

            const size_t n = fread(dst, sizeof(char), N - 1, stream);
            dst[n] = 0;
            return n;
        }

        size_t read(char* dst, size_t len) {
            if (eof()) return 0;

            return fread(dst, sizeof(char), len, stream);
        }

        template<size_t N>
        bool seek(const char (&str)[N]) {
            static_assert(N > 0);
            verify(str[0]);
            if (eof())
                return false;

            const size_t str_len = strlen(str);

            char buf[64*N];
            const size_t start = ftell(stream);
            size_t offset = start;
            while (!eof()) {
                const size_t n = read(buf);
                if (n < str_len)
                    break;

                if (const char* str_ptr = strstr(buf, str)) {
                    offset += str_ptr - buf;
                    if (0 == fseek(stream, offset, SEEK_SET))
                        return true;

                    break;
                }

                // if we found the first character of `str` at the end
                // of `buf`, don't skip past it!
                const char* buf_suffix = buf + (n - (str_len - 1));
                const char* str_prefix = strchr(buf_suffix, str[0]);
                if (str_prefix > buf_suffix) {
                    offset += str_prefix - buf;
                    if (0 == fseek(stream, offset, SEEK_SET))
                        continue;

                    break;
                }
                offset += n;
            }
            fseek(stream, start, SEEK_SET);
            return false;
        }

        template<size_t N>
        bool seek_and_skip(const char (&str)[N]) {
            return seek(str) and skip(strlen(str));
        }

        bool skip(size_t size) {
            if (eof()) return false;

            const size_t start = ftell(stream);
            if (0 == fseek(stream, size, SEEK_CUR))
                return true;

            fseek(stream, start, SEEK_SET);
            return false;
        }

    }; // struct file

} // namespace cxe