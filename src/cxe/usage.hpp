#pragma once

static constexpr const char USAGE[] = R"(
OVERVIEW: C/C++ meta compiler/executor

USAGE: cxe <file> [options] [file...] [-- [run-options]]

CONFIGURATION:
Options embedded in the <file> argument are interpreted as though appended to
the [options] provided when running cxe.  To embed cxe options within a C/C++
source file, use the following format:

    /*cxe{ [cc/cxx/cxe options] }*/

Example:

    /*cxe{
        -std=c++20
        -I$VULKAN_SDK/include
        -if (--target=[windows]) { # options if compiling for Windows
            -lgdi32 -luser32 -lshell32
            -o ../bin/windows/hello.exe
        }
        -if (--target=[darwin]) { # options if compiling for macOS
            -lstdc++
            -framework Cocoa -framework OpenGL -framework IOKit
            -o ../bin/macos/hello
        }
        -lglfw3
    }*/

New lines and # or //-prefixed comments within this block are ignored by cxe,
so the above example is interpreted as though these options were provided on the
command line:

    -std=c++20 -I$VULKAN_SDK/include -lglfw3 <file>

If target "--target=" option was provided on the command line, or the
effective compilation target determined by CC or CXX, contains the substring
"windows", then the full set of options becomes:

    -std=c++20 -I$VULKAN_SDK/include -lgdi32 -luser32 -lshell32 \
    --ouput=../bin/windows/hello.exe -lglfw3 <file>

OPTIONS:
-help           Print this message.
-if (...) {...} Conditional compilation options.
                Conditionals only evaluate options that were specified
                before the -if option.

                Example: Check whether -DRELEASE option was specified

                    -if (-DRELEASE) { -O3 }

                Example: Check whether an option with the prefix "--target="
                was specified and contains the substring "windows".

                    -if (--target=[windows]) {
                        -lgdi32
                        -Wl,--subsystem,windows
                    }

                Note: When the "--target=" option is not explicitly specified,
                cxe will invoke the compiler's "-print-effective-triple" option
                to determine the default compilation target.

-pre {...}      Run a shell command before compiling.
                If a pre-compile shell command fails, cxe will abort further
                compilation and return the same exit code that was returned
                by the failed command.
                An environment variable CXE=<path to this cxe executable> is
                defined when running such commands, so that you can easily
                run the same cxe executable on other dependencies.
-post {...}     Run a shell command after compiling successfully.
                An environment variable CXE=<path to this cxe executable> is
                defined when running such commands, so that you can easily
                run the same cxe executable on other dependencies.
--              If the compiled artifact is executable, execute it and
                pass any subsequent options to the executable.
)";
