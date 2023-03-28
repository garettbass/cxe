# `cxe` - compile, compose, and run C/C++ programs

```sh
USAGE: cxe <file> [options] [-- [run-options]]
```

`cxe` is a small command line utility that makes it easy to compile, compose, and execute C and C++ programs.  The primary goal is fast iteration on small projects without needing to configure an elaborate build system.

## Compilation

To compile your C or C++ program with `cxe`, provide the path to the ***main source file***, (`hello.c` in this example):

```sh
$ cxe src/hello.c
/path/to/clang hello.c
```

This has the same result as passing `hello.c` directly to `clang`, and an executable named `a` will be produced alongside `hello.c`.

Similar to `clang`, we can pass additional arguments when using `cxe`, but we always pass them after the ***main source file***:

```sh
$ cxe src/hello.c -fsanitize=address
/path/to/clang -fsanitize=address hello.c
```

However, when using `cxe` instead of `clang`, we can also add `clang` arguments directly to the body of `hello.c`, within a special comment block:

```c
#include <stdio.h>

/*cxe{
    -std=c11
    -Wall -Werror
    -o ../bin/hello.exe
}*/

int main(int argc, const char* argv[]) {
    puts("hello world!");
    return 0;
}
```

Now, when we run `cxe`, all of the `clang` arguments from the `/*cxe{...}*/` comment block will be added to the `clang` invocation:

```sh
$ cxe src/hello.c
/path/to/clang -std=c11 -Wall -Werror -o ../bin/hello.exe hello.c
```

Beyond passing arguments to `clang`, `cxe` has the following special arguments that it consumes:

* `-if (...) {...}` - A conditional set of arguments, discussed in greater detail in the section [Conditional Compilation](#conditional-compilation).
* `-pre {...}` - A shell command to run before compiling the ***main source file***
* `-post {...}` - A shell command to run after compiling the ***main source file***
* `-- ...` - A final argument typically provided on the command line which indicates that any subsequent arguments should be passed to the executable that results from compiling the ***main source file***.  Whenever the `--` argument is provided, the executable will be run, even if there are no subsequent arguments:

```sh
$ cxe src/hello.c --
/path/to/clang -std=c11 -Wall -Werror -o ../bin/hello.exe hello.c
hello world!
```

### Conditional Compilation

Since we likely want different `-o ...` arguments to `clang` on different platforms.  We can conditionally define which arguments `cxe` should pass along to `clang` as follows:

```c
#include <stdio.h>

/*cxe{
    -std=c11
    -Wall -Werror
    -if (--target=[darwin]) { # compiling for macOS
        -o ../bin/macos/hello
    }
    -if (--target=[windows]) { # compiling for Windows
        -o ../bin/windows/hello.exe
    }
}*/

int main(int argc, const char* argv[]) {
    puts("hello world!");
    return 0;
}
```

The `-if` argument evaluates a condition surrounded in parentheses `(...)`, and when that condition is `true`, the `clang` arguments surrounded by braces `{...}` are added to the `clang` command line.  In the example above, the `-if` argument is evaluating whether an argument with the prefix `--target=` was provided, and whether the remainder of that argument contains the text `darwin` or `windows`.

In general, the `-if` argument can evaluate the presence of any argument that is provided before it appears, either when invoking `cxe`, or within the `/*cxe{...}*/` comment block.  Some examples:

```c
/*cxe{
    -if (-g) {
        # -g was specified, let's also define DEBUG=1
        -DDEBUG=1
    }
    -if (-DRELEASE) {
        # -DRELEASE was specified, let's also optimize
        -O3
    }
    -if (--target=[darwin]) {
        # found an argument starting with `--target=`
        # and containing the text `darwin`
    }
}*/
```

In the special case of the `--target=` argument, `cxe` will synthesize the value of this argument by invoking `clang -print-effective-triple` when the `--target=` argument is not provided.

The `-if` condition can also contain boolean operators `&&`, `and`, `||`, `or`, but currently does not support parenthesized sub-expressions.

```c
/*cxe{
    -if (--target=[msvc] && -fsanitize=address) {
        # compiling for msvc standard library and ASAN
        # avoid an lld-link error:
        # "/INFERASANLIBS is not allowed in .drectve"
        -D_DISABLE_VECTOR_ANNOTATION
    }
}*/
```

### Comments

As shown in some of the preceeding examples, the `/*cxe{...}*/` comment block can contain single-line comments.  A comment begins with either `#` or `//`, and continues until the end of the line.

## Composition

Using the `-pre` argument, we can easily chain together compilation of dependencies.  Consider a C++ program that depends on `glfw3`:

```cpp
// hello.cpp
/*cxe{

    # first, compile glfw3.c, see details of glfw3.c below
    -pre { $CXE glfw3.c }

    -std=c++11
    -Wall -Werror

    # link with libglfw3.a or glfw3.lib
    -lglfw3

    -if (--target=[darwin]) {
        -lstdc++ # must explicitly link stdc++ on macOS

        # link with GLFW3 dependencies on macOS:
        -framework Cocoa -framework OpenGL -framework IOKit

        # add macOS-specific library search path
        # this is where we will find libglfw3.a
        -L../lib/macos
    }

    -if (--target=[windows]) {
        # link with GLFW3 dependencies on Windows
        -lgdi32 -lshell32 -luser32

        # add Windows-specific library search path
        # this is where we will find glfw3.lib
        -L../lib/windows
    }

}*/

#include "glfw3.h" // let's assume it is adjacent to hello.cpp

int main(int argc, const char* argv[]) {
    glfwInit();
    //...
    return 0;
}
```

We may reference the GLFW3 source code as a git submodule, or by copying just the parts we need, then we can easily adapt GLFW3 to be compiled by `cxe` as follows:

```c
// glfw3.c
/*cxe{
    -std=c99
    -Wall -Werror

    -if (--target=[darwin]) {
        # ensure macOS-specific library path exists
        -pre { mkdir -p ../lib/macos }

        # compile output to libglfw3.a
        -o ../lib/macos/libglfw3.a

        # suppress some warnings in GLFW3
        -Wno-deprecated-declarations

        # compile `glfw3.c` as Objective-C on macOS
        -x objective-c 
    }

    -if (--target=[windows]) {
        # ensure Windows-specific library path exists
        -pre { mkdir -p ../lib/windows }

        # compile output to glfw3.lib
        -o ../lib/windows/glfw3.lib

        # suppress some warnings in GLFW3
        -D_CRT_SECURE_NO_WARNINGS
    }

    -c # compile a static library
}*/

#if defined(_WIN32)

    #define _GLFW_WIN32
    #include "../submodules/glfw/src/monitor.c"
    #include "../submodules/glfw/src/init.c"
    // Include all Windows-specific sources...

#elif defined(__APPLE__)

    #include <TargetConditionals.h>
    #ifdef TARGET_OS_MAC

        #define _GLFW_COCOA
        #include "../submodules/glfw/src/monitor.c"
        #include "../submodules/glfw/src/init.c"
        // Include all macOS-specific sources...

    #else

        #error "unsupported Apple platform

    #endif

#endif
```

## Building `cxe`

Assuming you have a Unix or git-bash-like shell, you can build `cxe` for your runtime platform by running the command:

```sh
$ sh src/cxe.cpp
```

Which will produce the relevant binaries under `bin/<platform>/`.

## Disclaimer (YMMV)

I created `cxe` because I wished for something like this to exist for my convenience when iterating on small, simple projects.

This program has only been casually tested on macOS and Windows, and likely contains a variety of bugs and unexplored edge cases.

