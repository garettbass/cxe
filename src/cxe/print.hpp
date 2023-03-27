#pragma once
#include <stdint.h>
#include <span>
#include "verify.hpp"
#include "buffer.hpp"

namespace cxe {

    template<typename A, typename B>
    A reinterpret(B b) {
        static_assert(sizeof(B) == sizeof(A), "size mismatch");
        union { A a; B b; } conversion = { .b = b };
        return conversion.a;
    }

    //--------------------------------------------------------------------------

    struct hex {
        const uint64_t value;

        static inline const char prefix[] = "0x";
        static inline const char digits[] = "0123456789ABCDEF";

        hex(   uint64_t value) : value(value) {}
        hex(    int64_t value) : value(reinterpret<uint64_t>(value)) {}
        hex(   uint32_t value) : value(value) {}
        hex(    int32_t value) : value(reinterpret<uint32_t>(value)) {}
        hex(   uint16_t value) : value(value) {}
        hex(    int16_t value) : value(reinterpret<uint16_t>(value)) {}
        hex(    uint8_t value) : value(value) {}
        hex(     int8_t value) : value(reinterpret<uint8_t>(value)) {}
        hex(     double value) : value(reinterpret<uint64_t>(value)) {}
        hex(      float value) : value(reinterpret<uint32_t>(value)) {}
        hex(const void* value) : value(reinterpret<size_t>(value)) {}
    };

    struct bin {
        const uint64_t value;

        static inline const char prefix[] = "0b";
        static inline const char digits[] = "01";

        bin(   uint64_t value) : value(value) {}
        bin(    int64_t value) : value(reinterpret<uint64_t>(value)) {}
        bin(   uint32_t value) : value(value) {}
        bin(    int32_t value) : value(reinterpret<uint32_t>(value)) {}
        bin(   uint16_t value) : value(value) {}
        bin(    int16_t value) : value(reinterpret<uint16_t>(value)) {}
        bin(    uint8_t value) : value(value) {}
        bin(     int8_t value) : value(reinterpret<uint8_t>(value)) {}
        bin(     double value) : value(reinterpret<uint64_t>(value)) {}
        bin(      float value) : value(reinterpret<uint32_t>(value)) {}
        bin(const void* value) : value(reinterpret<size_t>(value)) {}
    };

    //--------------------------------------------------------------------------

    template<size_t BufferSize, size_t PrefixSize, size_t DigitsSize>
    char* to_c_str(
        char (&storage)[BufferSize],
        unsigned long long value,
        const char (&prefix)[PrefixSize],
        const char (&digits)[DigitsSize]
    ) {
        char* sto_rend = storage;
        char* sto_ritr = sto_rend + BufferSize;
        *--sto_ritr = 0;
        if (value == 0) { *--sto_ritr = '0'; }
        const unsigned long long base = DigitsSize - 1;
        while (value > 0 && sto_ritr > sto_rend) {
            *--sto_ritr = digits[value % base];
            value /= base;
        }
        if (PrefixSize) {
            const char* const pre_rend = prefix;
            const char* pre_ritr = pre_rend + PrefixSize - 1;
            while (sto_ritr > sto_rend and pre_ritr > pre_rend) {
                *--sto_ritr = *--pre_ritr;
            }
        }
        return sto_ritr;
    }

    template<size_t BufferSize, size_t PrefixSize>
    char* to_c_str(
        char (&storage)[BufferSize],
        unsigned long long value,
        const char (&prefix)[PrefixSize]
    ) {
        char* sto_rend = storage;
        char* sto_ritr = sto_rend + BufferSize;
        *--sto_ritr = 0;
        if (value == 0) { *--sto_ritr = '0'; }
        const unsigned long long base = 10;
        while (value > 0 and sto_ritr > sto_rend) {
            *--sto_ritr = '0' + value % base;
            value /= base;
        }
        if (PrefixSize) {
            const char* const pre_rend = prefix;
            const char* pre_ritr = pre_rend + PrefixSize - 1;
            while (sto_ritr > sto_rend and pre_ritr > pre_rend) {
                *--sto_ritr = *--pre_ritr;
            }
        }
        return sto_ritr;
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], unsigned long long value ) {
        return to_c_str(storage, value, "");
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], signed long long i) {
        const bool negative = i < 0;
        if (negative) {
            unsigned long long u = -i;
            return to_c_str(storage, u, "-");
        } else {
            unsigned long long u = i;
            return to_c_str(storage, u, "");
        }
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], unsigned long value ) {
        return to_c_str(storage, (unsigned long long)value);
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], signed long value ) {
        return to_c_str(storage, (signed long long)value);
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], unsigned int value ) {
        return to_c_str(storage, (unsigned long long)value);
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], signed int value ) {
        return to_c_str(storage, (signed long long)value);
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], unsigned short value ) {
        return to_c_str(storage, (unsigned long long)value);
    }

    template<size_t BufferSize>
    char* to_c_str(char (&storage)[BufferSize], signed short value ) {
        return to_c_str(storage, (signed long long)value);
    }

    //--------------------------------------------------------------------------

    size_t print_to(buffer<char>& buf, const std::span<const char>& span) {
        const size_t old_buf_size = buf.size();
        buf << span;
        return buf.size() - old_buf_size;
    }

    size_t print_to(buffer<char>& buf, const std::span<char>& span) {
        const size_t old_buf_size = buf.size();
        buf << span;
        return buf.size() - old_buf_size;
    }

    size_t print_to(buffer<char>& buf, const char* const str) {
        const size_t old_buf_size = buf.size();
        buf << str;
        return buf.size() - old_buf_size;
    }

    //--------------------------------------------------------------------------

    size_t print_to(FILE* stream, const std::span<const char>& span) {
        return fwrite(span.data(), sizeof(char), span.size(), stdout);
    }

    size_t print_to(FILE* stream, const std::span<char>& span) {
        return fwrite(span.data(), sizeof(char), span.size(), stdout);
    }

    template<size_t N>
    size_t print_to(FILE* stream, const char (&arr)[N]) {
        return fputs(arr, stream);
    }

    size_t print_to(FILE* stream, const char* const str) {
        return fputs(str, stream);
    }

    //--------------------------------------------------------------------------

    template<typename Out>
    size_t print_to(Out&& out, bool b) {
        return b ? print_to(out, "true") : print_to(out, "false");
    }

    template<typename Out>
    size_t print_to(Out&& out, unsigned long long u) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, u));
    }

    template<typename Out>
    size_t print_to(Out&& out, signed long long i) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, i));
    }

    template<typename Out>
    size_t print_to(Out&& out, unsigned long u) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, u));
    }

    template<typename Out>
    size_t print_to(Out&& out, signed long i) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, i));
    }

    template<typename Out>
    size_t print_to(Out&& out, unsigned int u) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, uint64_t(u)));
    }

    template<typename Out>
    size_t print_to(Out&& out, signed int i) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, int64_t(i)));
    }

    template<typename Out>
    size_t print_to(Out&& out, unsigned short u) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, uint64_t(u)));
    }

    template<typename Out>
    size_t print_to(Out&& out, signed short i) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, int64_t(i)));
    }

    template<typename Out>
    size_t print_to(Out&& out, const bin& b) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, b.value, b.prefix, b.digits));
    }

    template<typename Out>
    size_t print_to(Out&& out, const hex& h) {
        char storage[32] {};
        return print_to(out, to_c_str(storage, h.value, h.prefix, h.digits));
    }

    //--------------------------------------------------------------------------

    template<typename Out, typename Arg1, typename Arg2, typename... Args>
    size_t print_to(
        Out&& out,
        const Arg1& arg1,
        const Arg2& arg2,
        const Args&... args
    ) {
        size_t n = print_to(out, arg1) + print_to(out, arg2);
        return (n + ... + print_to(out, args));
    }

    template<typename Out, typename... Args>
    size_t println_to(Out&& out, const Args&... args) {
        return (print_to(out, args) + ... + print_to(out, "\n"));
    }

    //--------------------------------------------------------------------------

    template<typename... Args>
    size_t print(const Args&... args) {
        return print_to(stdout, args...);
    }

    template<typename... Args>
    size_t println(const Args&... args) {
        return println_to(stdout, args...);
    }

    //--------------------------------------------------------------------------

    template<typename... Args>
    size_t printerr(const Args&... args) {
        return print_to(stderr, args...);
    }

    template<typename... Args>
    size_t printerrln(const Args&... args) {
        return println_to(stderr, args...);
    }

    namespace escape_codes {
        static inline constexpr const char RESET         [] = "\x1b[0m";
        
        static inline constexpr const char BOLD          [] = "\x1b[1m";
        static inline constexpr const char THIN          [] = "\x1b[2m";
        static inline constexpr const char NORMAL        [] = "\x1b[22m";

        static inline constexpr const char ITALIC        [] = "\x1b[3m";
        static inline constexpr const char END_ITALIC    [] = "\x1b[23m";

        static inline constexpr const char UNDERLINE     [] = "\x1b[4m";
        static inline constexpr const char END_UNDERLINE [] = "\x1b[24m";

        static inline constexpr const char STRIKE        [] = "\x1b[8m";
        static inline constexpr const char END_STRIKE    [] = "\x1b[29m";

        static inline constexpr const char BLINK_1       [] = "\x1b[5m";
        static inline constexpr const char BLINK_2       [] = "\x1b[6m";
        static inline constexpr const char END_BLINK     [] = "\x1b[6m";

        static inline constexpr const char INVERT        [] = "\x1b[7m";

        static inline constexpr const char RESET_COLOR   [] = "\x1b[39m";
        static inline constexpr const char BLACK         [] = "\x1b[30m";
        static inline constexpr const char DKRED         [] = "\x1b[31m";
        static inline constexpr const char DKGREEN       [] = "\x1b[32m";
        static inline constexpr const char DKYELLOW      [] = "\x1b[33m";
        static inline constexpr const char DKBLUE        [] = "\x1b[34m";
        static inline constexpr const char DKPURPLE      [] = "\x1b[35m";
        static inline constexpr const char DKCYAN        [] = "\x1b[36m";
        static inline constexpr const char LTGREY        [] = "\x1b[37m";

        static inline constexpr const char DKGREY        [] = "\x1b[1;30m";
        static inline constexpr const char LTRED         [] = "\x1b[1;31m";
        static inline constexpr const char LTGREEN       [] = "\x1b[1;32m";
        static inline constexpr const char LTYELLOW      [] = "\x1b[1;33m";
        static inline constexpr const char LTBLUE        [] = "\x1b[1;34m";
        static inline constexpr const char LTPURPLE      [] = "\x1b[1;35m";
        static inline constexpr const char LTCYAN        [] = "\x1b[1;36m";
        static inline constexpr const char WHITE         [] = "\x1b[1;37m";

        static inline constexpr const char BG_BLACK       [] = "\x1b[40m";
        static inline constexpr const char BG_DKRED       [] = "\x1b[41m";
        static inline constexpr const char BG_DKGREEN     [] = "\x1b[42m";
        static inline constexpr const char BG_DKORANGE    [] = "\x1b[43m";
        static inline constexpr const char BG_DKBLUE      [] = "\x1b[44m";
        static inline constexpr const char BG_DKPURPLE    [] = "\x1b[45m";
        static inline constexpr const char BG_DKCYAN      [] = "\x1b[46m";
        static inline constexpr const char BG_LTGREY      [] = "\x1b[47m";

        static inline constexpr const char BG_DKGREY      [] = "\x1b[1;40m";
        static inline constexpr const char BG_LTRED       [] = "\x1b[1;41m";
        static inline constexpr const char BG_LTGREEN     [] = "\x1b[1;42m";
        static inline constexpr const char BG_YELLOW      [] = "\x1b[1;43m";
        static inline constexpr const char BG_LTBLUE      [] = "\x1b[1;44m";
        static inline constexpr const char BG_LTPURPLE    [] = "\x1b[1;45m";
        static inline constexpr const char BG_LTCYAN      [] = "\x1b[1;46m";
        static inline constexpr const char BG_WHITE       [] = "\x1b[1;47m";

    };

} // namespace cxe