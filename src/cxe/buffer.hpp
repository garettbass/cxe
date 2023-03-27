#pragma once
#include <span>
#include <vector>
#include "verify.hpp"

namespace cxe {

    template<typename T>
    class buffer : std::vector<T> {
        using base = std::vector<T>;

        buffer(const buffer&) = default;
        buffer& operator=(const buffer&) = default;

    public:

        buffer() = default;

        buffer(buffer&&) = default;
        buffer& operator=(buffer&&) = default;

        using const_iterator = typename base::const_iterator;
        using       iterator = typename base::iterator;

        using base::operator[];
        using base::at;

        using base::empty;
        using base::size;
        using base::reserve;
        using base::resize;

        using base::data;
        using base::begin;
        using base::end;

        using base::front;
        using base::back;

        using base::assign;
        using base::clear;
        using base::emplace_back;
        using base::insert;
        using base::push_back;
        using base::pop_back;

    };

    // buffer<char> tries to ensure that the char[] remains nul terminated
    template<>
    class buffer<char> : std::vector<char> {
        using base = std::vector<char>;

        buffer(const buffer&) = default;
        buffer& operator=(const buffer&) = default;

    public:

        buffer(size_t min_capacity = 63) { reserve(min_capacity); }

        buffer(buffer&&) = default;
        buffer& operator=(buffer&&) = default;

        const char& operator[](size_t i) const { return at(i); }
              char& operator[](size_t i)       { return at(i); }

        const char& at(size_t i) const { verify(size()>i); return base::at(i); }
              char& at(size_t i)       { verify(size()>i); return base::at(i); }

        bool empty() const { return size() == 0; }

        size_t capacity() const {
            verify(nul_terminated());
            return base::capacity() - 1;
        }

        size_t size() const {
            verify(nul_terminated());
            return base::size() - 1;
        }

        void reserve(size_t min_capacity) {
            base::reserve(min_capacity + 1);
            if (not nul_terminated())
                base::push_back(0);
            verify(nul_terminated());
        }

        void resize(size_t new_size) {
            base::resize(new_size + 1);
            if (not nul_terminated())
                base::back() = 0;
            verify(nul_terminated());
        }

        void resize(size_t new_size, char c) {
            base::resize(new_size, c);
            if (not nul_terminated())
                base::push_back(0);
            verify(nul_terminated());
        }

        const char* data() const { return base::data(); }
              char* data()       { return base::data(); }

        const char* begin() const { return data(); }
              char* begin()       { return data(); }
        const char*   end() const { return data() + size(); }
              char*   end()       { return data() + size(); }

        const char& front() const { return at(0); }
              char& front()       { return at(0); }
        const char&  back() const { return at(size()-1); }
              char&  back()       { return at(size()-1); }

        void assign(const std::initializer_list<char> init) {
            base::assign(init);
            base::push_back(0);
        }

        void clear() {
            base::clear();
            base::push_back(0);
        }

        char* erase(const char* where) {
            verify(where >= begin());
            verify(where <  end());
            size_t i = where - begin();
            i = base::erase(base::begin() + i) - base::begin();
            verify(nul_terminated());
            return begin() + i;
        }

        char* erase(const char* start, const char* stop) {
            verify(start >= begin());
            verify(start <= end());
            verify(stop  >= begin());
            verify(stop  <= end());
            size_t i = start - begin(), e = stop  - begin();
            i = base::erase(base::begin()+i, base::begin()+e) - base::begin();
            verify(nul_terminated());
            return begin() + i;
        }

        char* insert(const char* where, char c) {
            verify(where >= begin());
            verify(where <= end());
            size_t i = where - begin();
            i = base::insert(base::begin()+i, c) - base::begin();
            verify(nul_terminated());
            return begin() + i;
        }

        template<typename Itr>
        char* insert(const char* where, Itr start, Itr stop) {
            verify(where >= begin());
            verify(where <= end());
            size_t i = where - begin();
            i = base::insert(base::begin() + i, start, stop) - base::begin();
            verify(nul_terminated());
            return begin() + i;
        }

        void push_back(char c) {
            verify(nul_terminated());
            base::back() = c;
            base::push_back(0);
            verify(nul_terminated());
        }

        char pop_back() {
            verify(nul_terminated());
            base::pop_back();
            char back = base::back();
            base::back() = 0;
            verify(nul_terminated());
            return back;
        }

        bool nul_terminated() const {
            return base::size() and base::back() == 0;
        }

        void truncate() { resize(strlen(data())); }
    };

    // buffer<T*> tries to ensure that the T*[] remains nullptr terminated
    template<typename T>
    class buffer<T*> : std::vector<T*> {
        using base = std::vector<T*>;

        buffer(const buffer&) = default;
        buffer& operator=(const buffer&) = default;

    public:

        buffer(size_t min_capacity = 7) { reserve(min_capacity); }

        buffer(buffer&&) = default;
        buffer& operator=(buffer&&) = default;

        const T*& operator[](size_t i) const { return at(i); }
              T*& operator[](size_t i)       { return at(i); }

        const T*& at(size_t i) const { verify(size()>i); return base::at(i); }
              T*& at(size_t i)       { verify(size()>i); return base::at(i); }

        bool empty() const { return size() == 0; }

        size_t capacity() const {
            verify(null_terminated());
            return base::capacity() - 1;
        }

        size_t size() const { 
            verify(null_terminated());
            return base::size() - 1;
        }

        void reserve(size_t min_capacity) {
            base::reserve(min_capacity + 1);
            if (not null_terminated())
                base::push_back(0);
            verify(null_terminated());
        }

        void resize(size_t new_size) {
            base::resize(new_size + 1);
            if (not null_terminated())
                base::back() = nullptr;
            verify(null_terminated());
        }

        auto data() const { return base::data(); }
        auto data()       { return base::data(); }

        auto begin() const { return data(); }
        auto begin()       { return data(); }
        auto   end() const { return data() + size(); }
        auto   end()       { return data() + size(); }

        auto front() const { return at(0); }
        auto front()       { return at(0); }
        auto  back() const { return at(size() - 1); }
        auto  back()       { return at(size() - 1); }

        void clear() { 
            base::clear();
            base::push_back(nullptr);
        }

        void push_back(T* p) {
            verify(null_terminated());
            base::back() = p;
            base::push_back(nullptr);
            verify(null_terminated());
        }

        void pop_back() {
            verify(null_terminated());
            base::pop_back();
            base::back() = nullptr;
            verify(null_terminated());
        }

        bool null_terminated() const {
            return base::size() and base::back() == nullptr;
        }
    };
}

cxe::buffer<char>& operator<<(cxe::buffer<char>& dst, const char chr) {
    verify(dst.nul_terminated());

    dst.push_back(chr);

    verify(dst.nul_terminated());
    return dst;
}

cxe::buffer<char>& operator<<(cxe::buffer<char>& dst, const char* const str) {
    verify(dst.nul_terminated());

    // append str characters up to nul
    for (const char* p = str; *p; ++p) dst.push_back(*p);

    verify(dst.nul_terminated());
    return dst;
}

cxe::buffer<char>& operator<<(cxe::buffer<char>& dst, const std::span<char>& span) {
    verify(dst.nul_terminated());

    // append token characters
    const char* const ptr = span.data();
    const char* const end = ptr + span.size();
    for (const char* p = ptr; *p and p < end; ++p) dst.push_back(*p);

    verify(dst.nul_terminated());
    return dst;
}

cxe::buffer<char>& operator<<(cxe::buffer<char>& dst, const std::span<const char>& span) {
    verify(dst.nul_terminated());

    // append token characters
    const char* const ptr = span.data();
    const char* const end = ptr + span.size();
    for (const char* p = ptr; *p and p < end; ++p) dst.push_back(*p);

    verify(dst.nul_terminated());
    return dst;
}

cxe::buffer<char>& operator>>(const char chr, cxe::buffer<char>& dst) {
    verify(dst.nul_terminated());

    dst.insert(dst.begin(), chr);

    verify(dst.nul_terminated());
    return dst;
}

cxe::buffer<char>& operator>>(const std::span<const char>& span, cxe::buffer<char>& dst) {
    verify(dst.nul_terminated());

    dst.insert(dst.begin(), span.begin(), span.end());

    verify(dst.nul_terminated());
    return dst;
}

