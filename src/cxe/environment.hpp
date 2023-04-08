#pragma once
#include "verify.hpp"
#include "buffer.hpp"
#include "scan.hpp"

namespace cxe::environment {

    class variable {
        buffer<char> _name;
        buffer<char> _name_eq_value;

        variable(const variable&) = delete;
        variable& operator=(const variable&) = delete;

    public:

        /** /
        // fails to clean up if variable value is empty string
        // e.g. "VAR="
        ~variable() {
            verify(_name.size());
            verify(_name_eq_value.size());
            verify(_name_eq_value.nul_terminated());

            char* const eq = strchr(_name_eq_value.data(), '=');
            verify(eq);
            verify(eq[0] == '=');
            char* const value = eq + 1;

            if (0 != strcmp(getenv(_name.data()), value)) {
                return; // someone else owns $NAME now
            }

            #if _WIN32
                // _putenv("NAME=") removes Windows environment variable
                _name.push_back('=');
                _putenv(_name.data());
            #else
                // unsetenv("NAME") removes Posix environment variable
                unsetenv(_name.data());
            #endif
            _name.clear();
            _name_eq_value.clear();
        }
        /**/

        template<typename Name, typename Value, typename... Args>
        variable(const Name& name, const Value& value, const Args&... args) {
            print_to(_name,name);
            verify(_name.size());
            verify(_name.nul_terminated());

            print_to(_name_eq_value,name,"=",value,args...);
            verify(_name_eq_value.size());
            verify(_name_eq_value.size() == strlen(_name_eq_value.data()));
            verify(_name_eq_value[0] != '=');
            #if _WIN32
                _putenv(_name_eq_value.data());
            #else
                putenv(_name_eq_value.data());
            #endif
        }

    };

    static bool resolve_variable(
        buffer<char>& buf,
        const size_t offset,
        const size_t length
    ) {
        verify(buf.size());
        verify(offset > 0);
        verify(offset < buf.size());
        verify(offset + length <= buf.size());

        const size_t dollar = offset - 1;
        const size_t terminator = offset + length;

        // nul-terminate $NAME in buffer
        buf.insert(buf.begin() + terminator, 0);

        const char* const value = getenv(buf.data() + offset);
        if (not value) return false;

        // erase $NAME\0
        buf.erase(buf.begin() + dollar, buf.begin() + terminator + 1);

        const char* const value_end = value + strlen(value);

        buf.insert(buf.begin() + dollar, value, value_end);
        buf.truncate();
        return true;
    }

    static bool resolve_variables(buffer<char>& buf) {
        using namespace ::cxe::scan;

        for (;;) {
            itr_t itr = buf.data();
            end_t end = itr + buf.size();

            if (not seek('$', itr, end)) break;
            if (not skip('$', itr, end)) break;

            const char* const var_ptr = itr;

            isident::state s{};
            const size_t var_len = skip_while(isident{s}, itr, end);
            verify(var_len);

            const char* const var_end = itr;

            const size_t var_offset = var_ptr - buf.data();
            const size_t var_length = var_end - var_ptr;
            if (not resolve_variable(buf, var_offset, var_length))
                return false;

        }
        return true;
    }

} // namespace cxe::environment