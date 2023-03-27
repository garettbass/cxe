#pragma once
#include <ctype.h>
#include <span>
#include "verify.hpp"

namespace cxe::scan {

    using str_t = const char*;
    using itr_t = const char*;
    using end_t = const char* const;
    using span_t = std::span<const char>;

    using compare_t = int(const char a, const char b);

    int match_case(const char a, const char b) {
        return int(a) - int(b);
    }

    int ignore_case(const char a, const char b) {
        return tolower(a) - tolower(b);
    }

    bool eq(compare_t cmp, const char a, const char b) {
        return 0 == cmp(a, b);
    }

    bool ne(compare_t cmp, const char a, const char b) {
        return 0 != cmp(a, b);
    }

    bool equals(str_t s, end_t ptr, end_t end, compare_t cmp) {
        if (ptr >= end) return false;
        itr_t p = ptr;
        for (; p < end and *s and eq(cmp,*s,*p); ++p,++s) {}
        return eq(cmp,*s,'\0') and p == end;
    }

    bool equals(str_t s, end_t ptr, end_t end) {
        if (ptr >= end) return false;
        itr_t p = ptr;
        for (; p < end and *s and *s == *p; ++p,++s) {}
        return *s == '\0' and p == end;
    }

    bool equals(str_t s, const span_t& span, compare_t cmp) {
        const size_t s_size = strlen(s);
        const size_t span_size = span.size();
        if (s_size != span_size) return false;
        end_t ptr = span.data();
        end_t end = ptr + span_size;
        return equals(s, ptr, end, cmp);
    }

    bool equals(str_t s, const span_t& span) {
        return equals(s, span, match_case);
    }

    bool equals(const span_t& a, const span_t& b, compare_t cmp) {
        const size_t a_size = a.size();
        const size_t b_size = b.size();
        if (a_size != b_size) return false;
        itr_t a_ptr = a.data();
        itr_t b_ptr = b.data();
        if (a_ptr == b_ptr) return true;
        end_t a_end = a_ptr + a_size;
        for (; a_ptr < a_end; ++a_ptr, ++b_ptr) {
            if (ne(cmp,*a_ptr,*b_ptr)) return false;
        }
        return true;
    }

    bool equals(const span_t& a, const span_t& b) {
        return equals(a, b, match_case);
    }

    bool prefix(const char c, end_t ptr, end_t end, compare_t cmp) {
        return ptr < end and eq(cmp, ptr[0], c);
    }

    bool prefix(const char c, end_t ptr, end_t end) {
        return ptr < end and ptr[0] == c;
    }

    template<typename IsChar>
    bool prefix(IsChar ischar, end_t ptr, end_t end) {
        return ptr < end and ischar(ptr[0]);
    }

    bool prefix(str_t s, end_t ptr, end_t end, compare_t cmp) {
        if (ptr >= end) return false;
        for (itr_t p = ptr; p < end and *s and *p and eq(cmp,*s,*p); ++p,++s) {}
        return eq(cmp,*s,'\0');
    }

    bool prefix(str_t s, end_t ptr, end_t end) {
        if (ptr >= end) return false;
        for (itr_t p = ptr; p < end and *s and *p and *s == *p; ++p,++s) {}
        return *s == '\0';
    }

    bool prefix(str_t s, const span_t& span, compare_t cmp) {
        const size_t s_size = strlen(s);
        const size_t span_size = span.size();
        if (s_size > span_size) return false;
        end_t ptr = span.data();
        end_t end = ptr + span_size;
        return prefix(s, ptr, end, cmp);
    }

    bool prefix(str_t s, const span_t& span) {
        return prefix(s, span, match_case);
    }

    bool prefix(const span_t& a, const span_t& b, compare_t cmp) {
        const size_t a_size = a.size();
        const size_t b_size = b.size();
        if (a_size > b_size) return false;
        itr_t a_ptr = a.data();
        itr_t b_ptr = b.data();
        if (a_ptr == b_ptr) return true;
        end_t a_end = a_ptr + a_size;
        for (; a_ptr < a_end; ++a_ptr, ++b_ptr) {
            if (ne(cmp,*a_ptr,*b_ptr)) return false;
        }
        return true;
    }

    bool prefix(const span_t& a, const span_t& b) {
        return prefix(a, b, match_case);
    }

    bool suffix(str_t s, const span_t& span, compare_t cmp) {
        const size_t s_size = strlen(s);
        const size_t span_size = span.size();
        if (s_size > span_size) return false;
        end_t end = span.data() + span_size;
        end_t ptr = end - s_size;
        return prefix(s, ptr, end, cmp);
    }

    bool suffix(str_t s, const span_t& span) {
        return suffix(s, span, match_case);
    }

    bool suffix(const span_t& a, const span_t& b, compare_t cmp) {
        const size_t a_size = a.size();
        const size_t b_size = b.size();
        if (a_size > b_size) return false;
        if (a.data() == b.data()) return true;
        end_t b_end = b.data() + b_size;
        end_t b_ptr = b_end - a_size;
        return prefix(a, span_t(b_ptr, a_size), cmp);
    }

    bool suffix(const span_t& a, const span_t& b) {
        return suffix(a, b, match_case);
    }

    bool chop(str_t s, span_t& span, compare_t cmp) {
        const size_t s_size = strlen(s);
        const size_t span_size = span.size();
        if (s_size > span_size) return false;
        end_t end = span.data() + span_size;
        end_t ptr = end - s_size;
        if (not prefix(s, ptr, end, cmp)) return false;
        span = span_t(span.data(), span.data() + span_size - s_size);
        return true;
    }

    bool chop(str_t s, span_t& span) {
        return chop(s, span, match_case);
    }

    bool contains(str_t s, end_t ptr, end_t end) {
        if (ptr >= end) return false;
        for (itr_t p = ptr; p < end; ++p)
            if (prefix(s, p, end))
                return true;
        return false;
    }

    bool contains(str_t s, const span_t& span) {
        const size_t s_size = strlen(s);
        const size_t span_size = span.size();
        if (s_size > span_size) return false;
        end_t ptr = span.data();
        end_t end = ptr + span_size;
        return contains(s, ptr, end);
    }

    bool contains(const span_t& a, const span_t& b) {
        const size_t a_size = a.size();
        const size_t b_size = b.size();
        if (a_size > b_size) return false;
        itr_t a_ptr = a.data();
        itr_t b_ptr = b.data();
        if (a_ptr == b_ptr) return true;
        end_t b_end = b_ptr + b_size;
        for (; b_ptr < b_end; ++b_ptr) {
            if (prefix(a, span_t(b_ptr, b_end)))
                return true;
        }
        return false;
    }

    bool seek(char c, itr_t& itr, end_t end) {
        if (itr >= end) return false;
        for (itr_t p = itr; p < end; ++p)
            if (prefix(c, p, end))
                return (itr = p, true);
        return false;
    }

    template<typename IsChar>
    bool seek(IsChar ischar, itr_t& itr, end_t end) {
        if (itr >= end) return false;
        for (itr_t p = itr; p < end; ++p)
            if (prefix(ischar, p, end))
                return (itr = p, true);
        return false;
    }

    bool seek(str_t s, itr_t& itr, end_t end) {
        if (itr >= end) return false;
        for (itr_t p = itr; p < end; ++p)
            if (prefix(s, p, end))
                return (itr = p, true);
        return false;
    }

    bool seek(str_t s, span_t& span) {
        const size_t s_size = strlen(s);
        const size_t span_size = span.size();
        if (s_size > span_size) return false;
        itr_t itr = span.data();
        end_t end = itr + span_size;
        return seek(s, itr, end) and (span = span_t(itr, end), true);
    }

    template<typename IsChar>
    bool seek(IsChar ischar, span_t& span) {
        const size_t span_size = span.size();
        itr_t itr = span.data();
        end_t end = itr + span_size;
        return seek(ischar, itr, end) and (span = span_t(itr, end), true);
    }

    bool skip(const char c, itr_t& itr, end_t end) {
        return prefix(c, itr, end) and (++itr, true);
    }

    template<typename IsChar>
    bool skip(IsChar ischar, itr_t& itr, end_t end) {
        return prefix(ischar, itr, end) and (++itr, true);
    }

    bool skip(str_t pre, itr_t& itr, end_t end) {
        return prefix(pre, itr, end) and (itr += strlen(pre), true);
    }

    bool skip(str_t pre, span_t& span) {
        const size_t s_size = strlen(pre);
        const size_t span_size = span.size();
        if (s_size > span_size) return false;
        itr_t itr = span.data();
        end_t end = itr + span_size;
        return skip(pre, itr, end) and (span = span_t(itr, end), true);
    }

    template<typename IsChar>
    bool skip(IsChar ischar, span_t& span) {
        const size_t span_size = span.size();
        itr_t itr = span.data();
        end_t end = itr + span_size;
        return skip(ischar, itr, end) and (span = span_t(itr, end), true);
    }

    bool skip(const span_t& pre, span_t& t) {
        itr_t itr = t.data();
        end_t end = itr + t.size();
        return prefix(pre, t) and (t = span_t(itr + pre.size(), end), true);
    }

    template<typename IsChar>
    size_t skip_while(IsChar ischar, itr_t& itr, end_t end) {
        size_t n = 0; for(; skip(ischar, itr, end); ++n) {} return n;
    }

    struct istoken {
        struct state {
            char quotes = 0;
            char prev   = 0;
        }& s;
        bool operator()(char c) {
            if (s.quotes >= 2) {
                return s.prev = 0, false; // token ends after second quote
            }
            if (c == '"') {
                if (s.prev != '\\')
                    s.quotes += 1;
                return s.prev = c, true; // include quote character
            }
            if (s.quotes == 1) {
                return s.prev = c, true; // include any character inside quotes
            }
            switch (c) {
                case '{': case '}':
                case '[': case ']':
                case '(': case ')':
                case '#':
                return s.prev = 0, false;
            }
            s.prev = c;
            return not isspace(c);
        }
    };

    struct ischar {
        const char* const chars;
        bool operator()(char c) const { return strchr(chars, c); }
    };

    static bool isAZaz_(const char c) { return c == '_' or isalpha(c); }

    static bool isAZaz_09(const char c) { return c == '_' or isalnum(c); }

    struct isident {
        struct state {
            bool (*ischar)(const char) = isAZaz_;
        }& s;
        bool operator()(char c) {
            if (s.ischar(c)) {
                s.ischar = isAZaz_09;
                return true;
            }
            return false;
        }
    };

} // namespace cxe::scan