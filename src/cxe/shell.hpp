#pragma once

#if defined(__APPLE__)
    #include <spawn.h>
    extern "C" {
        extern char** environ;
    }
#endif

#include <ctype.h>
#include "buffer.hpp"
#include "file.hpp"
#include "print.hpp"
#include "scan.hpp"
#include "verify.hpp"

namespace cxe::shell {

    int which(buffer<char>& out, const char* cmd);

    #if defined(_WIN32)
    namespace _win32 {

        struct SECURITY_ATTRIBUTES {
            uint32_t nLength = 0;
            void*    lpSecurityDescriptor = 0;
            int32_t  bInheritHandle = 0;
        };

        extern "C"
        int __stdcall
        CreatePipe(
            void**               hReadPipe,
            void**               hWritePipe,
            SECURITY_ATTRIBUTES* lpPipeAttributes,
            uint32_t             nSize
        );

        enum : uint32_t {
            STD_INPUT_HANDLE  = uint32_t(-10),
            STD_OUTPUT_HANDLE = uint32_t(-11),
            STD_ERROR_HANDLE  = uint32_t(-12),
        };

        extern "C"
        void* __stdcall
        GetStdHandle(uint32_t nStdHandle);

        enum : uint32_t { HANDLE_FLAG_INHERIT = 0x00000001 };

        extern "C"
        int __stdcall
        SetHandleInformation(
            void* hObject,
            uint32_t dwMask,
            uint32_t dwFlags
        );

        extern "C"
        int __cdecl
        _open_osfhandle(void* osfhandle, int flags);

        enum : uint32_t { STARTF_USESTDHANDLES = 0x00000100 };

        struct STARTUPINFOA {
            uint32_t cb = 0;
            char*    lpReserved = 0;
            char*    lpDesktop = 0;
            char*    lpTitle = 0;
            uint32_t dwX = 0;
            uint32_t dwY = 0;
            uint32_t dwXSize = 0;
            uint32_t dwYSize = 0;
            uint32_t dwXCountChars = 0;
            uint32_t dwYCountChars = 0;
            uint32_t dwFillAttribute = 0;
            uint32_t dwFlags = 0;
            uint16_t wShowWindow = 0;
            uint16_t cbReserved2 = 0;
            uint8_t* lpReserved2 = 0;
            void*    hStdInput = 0;
            void*    hStdOutput = 0;
            void*    hStdError = 0;
        };

        struct PROCESS_INFORMATION {
            void*    hProcess = 0;
            void*    hThread = 0;
            uint32_t dwProcessId = 0;
            uint32_t dwThreadId = 0;
        };

        extern "C"
        int __stdcall
        CreateProcessA(
            const char*          lpApplicationName,
            char*                lpCommandLine,
            void*                lpProcessAttributes,
            void*                lpThreadAttributes,
            int32_t              bInheritHandles,
            uint32_t             dwCreationFlags,
            void*                lpEnvironment,
            const char*          lpCurrentDirectory,
            STARTUPINFOA*        lpStartupInfo,
            PROCESS_INFORMATION* lpProcessInformation
        );

        extern "C"
        int __stdcall
        CloseHandle(void*);

        enum : uint32_t { INFINITE = ~0u };

        extern "C"
        uint32_t __stdcall
        WaitForSingleObject(
            void*    hHandle,
            uint32_t dwMilliseconds
        );

        extern "C"
        int __stdcall
        GetExitCodeProcess(
            void* hProcess,
            int*  lpExitCode
        );

    } // namespace _win32
    #endif

    //--------------------------------------------------------------------------

    void normalize_newlines(buffer<char>& buf) {
        using namespace scan;
        char* wr = buf.data();
        itr_t rd = wr;
        end_t end = wr + buf.size() - 1;
        for (; rd < end; ++rd, ++wr) {
            if (prefix("\r\n", rd, end)) {
                *wr = '\n'; ++rd;
            }
        }
        *wr = 0;
    }

    void trim_trailing_newlines(buffer<char>& buf) {
        while (buf.size() and buf.back() == 0) buf.pop_back();
        while (buf.size() and buf.back() == '\n') buf.pop_back();
    }

    //--------------------------------------------------------------------------

    int _run(const char* cmd) {
        #if defined(_WIN32)

            using namespace ::cxe::shell::_win32;

            STARTUPINFOA startup_info {
                .cb         = sizeof(startup_info),
                .dwFlags    = STARTF_USESTDHANDLES,
                .hStdInput  = GetStdHandle(STD_INPUT_HANDLE),
                .hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
                .hStdError  = GetStdHandle(STD_ERROR_HANDLE),
            };

            PROCESS_INFORMATION process_info;

            const size_t cmd_len = strlen(cmd);
            char* cmd_buf = (char*)_alloca(cmd_len + 1);
            strcpy(cmd_buf, cmd);
            cmd_buf[cmd_len] = 0;

            const int created = CreateProcessA(
                nullptr,        // lpApplicationName
                cmd_buf,        // lpCommandLine
                nullptr,        // lpProcessAttributes
                nullptr,        // lpThreadAttributes
                true,           // bInheritHandles
                0,              // dwCreationFlags
                nullptr,        // lpEnvironment
                nullptr,        // lpCurrentDirectory
                &startup_info,  // lpStartupInfo
                &process_info   // lpProcessInformation
            );

            if (not created) return -1;

            CloseHandle(process_info.hThread);

            WaitForSingleObject(process_info.hProcess, INFINITE);

            int exit_code;
            if (!GetExitCodeProcess(process_info.hProcess, &exit_code))
                return -1;

            return exit_code;

        #elif defined(__APPLE__)

            verify(false); // todo
            return -1;

        #endif
    }

    int run_argv(char* argv[]) {
        int status = -1;

        #if defined(_WIN32)

            using namespace ::cxe::shell::_win32;

            STARTUPINFOA si {
                .cb         = sizeof(si),
                .dwFlags    = STARTF_USESTDHANDLES,
                .hStdInput  = GetStdHandle(STD_INPUT_HANDLE),
                .hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
                .hStdError  = GetStdHandle(STD_ERROR_HANDLE),
            };

            PROCESS_INFORMATION pi;

            buffer<char> buf;
            for (auto argp = argv; *argp; ++argp) {
                if (buf.size())
                    buf << " ";
                buf << *argp;
            }

            const int created = CreateProcessA(
                nullptr,    // lpApplicationName
                buf.data(), // lpCommandLine
                nullptr,    // lpProcessAttributes
                nullptr,    // lpThreadAttributes
                true,       // bInheritHandles
                0,          // dwCreationFlags
                nullptr,    // lpEnvironment
                nullptr,    // lpCurrentDirectory
                &si,        // lpStartupInfo
                &pi         // lpProcessInformation
            );

            if (not created) return -1;

            CloseHandle(pi.hThread);

            WaitForSingleObject(pi.hProcess, INFINITE);

            if (GetExitCodeProcess(pi.hProcess, &status)) return status;

            // todo: try _spawnv() instead

        #elif defined(__APPLE__)

            pid_t pid;

            buffer<char> buf;
            shell::which(buf, argv[0]);
            const char* path = buf.data();

            int result = posix_spawn(&pid, path, nullptr, nullptr, argv, environ);

            switch (result) {
                case EINVAL:            //The value specified by file_actions or attrp is
                print("\nEINVAL");      //invalid.
                break;

                case E2BIG:             //The number of bytes in the new process's argument list
                print("\nE2BIG");       //is larger than the system-imposed limit.  This limit
                break;                  //is specified by the sysctl(3) MIB variable
                                        //KERN_ARGMAX.

                case EACCES:            //Search permission is denied for a component of the
                print("\nEACCES");      //path prefix.
                break;                  //The new process file is not an ordinary file.
                                        //The new process file mode denies execute permission.
                                        //The new process file is on a filesystem mounted with
                                        //execution disabled (MNT_NOEXEC in <sys/mount.h>).

                case EFAULT:            //The new process file is not as long as indicated by
                print("\nEFAULT");        //the size values in its header.
                break;                  //Path, argv, or envp point to an illegal address.

                case EIO:               //An I/O error occurred while reading from the file system.
                print("\nEIO");
                break;

                case ELOOP:             //Too many symbolic links were encountered in translating
                print("\nELOOP");       //the pathname.  This is taken to be indicative of a
                break;                  //looping symbolic link.

                case ENAMETOOLONG:      //A component of a pathname exceeded {NAME_MAX} characters,
                print("\nENAMETOOLONG");//or an entire path name exceeded {PATH_MAX} characters.
                break;

                case ENOENT:            //The new process file does not exist.
                print("\nENOENT");
                break;

                case ENOEXEC:           //The new process file has the appropriate access permission,
                print("\nENOEXEC");     //but has an unrecognized format (e.g., an
                break;                  //invalid magic number in its header).

                case ENOMEM:            //The new process requires more virtual memory than is
                print("\nENOMEM");      //allowed by the imposed maximum (getrlimit(2)).
                break;

                case ENOTDIR:           //A component of the path prefix is not a directory.
                print("\nENOTDIR");
                break;

                case ETXTBSY:           //The new process file is a pure procedure (shared text)
                print("\nETXTBSY");     //file that is currently open for writing or reading by
                break;                  //some process.
            }

            if (pid == waitpid(pid, &status, 0)) return status;

        #endif

        return -1;
    }

    int run(const char* cmd) {
        return _run(cmd);
    }

    template<typename Cmd, typename... Args>
    int run(const Cmd& cmd, const Args&... args) {
        buffer<char> buf;
        buf.reserve((32) + (8 * sizeof...(args)) + 1);
        print_to(buf, cmd, args...);
        return _run(buf.data());
    }

    //--------------------------------------------------------------------------

    int _run(buffer<char>& out, const char* cmd) {
        cxe::file f = cxe::file::popen(cmd);
        if (not f) {
            using namespace escape_codes;
            print(LTRED,"error: ",RESET,"command not found: \"",cmd,"\"");
            exit(1);
        }

        char buf[16]; while (f.read(buf)) out << buf;
        normalize_newlines(out);
        trim_trailing_newlines(out);
        // println("out: {",out.data(),"}[",strlen(out.data()),"]\n");
        return f.close();
    }

    int run(buffer<char>& out, const char* cmd) {
        return _run(out, cmd);
    }

    template<typename Cmd, typename... Args>
    int run(buffer<char>& out, const Cmd& cmd, const Args&... args) {
        buffer<char> buf;
        buf.reserve((32) + (8 * sizeof...(args)) + 1);
        print_to(buf, cmd, args...);
        return _run(out, buf.data());
    }

    //--------------------------------------------------------------------------

    void run_or_exit(buffer<char>& out, const char* cmd) {
        if (const int exit_code = _run(out, cmd)) exit(exit_code);
    }

    template<typename Cmd, typename... Args>
    void run_or_exit(buffer<char>& out, const Cmd& cmd, const Args&... args) {
        if (const int exit_code = run(out, cmd, args...)) exit(exit_code);
    }

    //--------------------------------------------------------------------------

    int which(buffer<char>& out, const char* cmd) {
        #if defined(_WIN32)
            return shell::run(out, "where ", cmd);
        #else
            return shell::run(out, "which ", cmd);
        #endif
    }

} // namespace cxe::shell