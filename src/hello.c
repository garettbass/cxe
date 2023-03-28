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
    -if (--target=[msvc] && -fsanitize=address) {
        # lld-link: error: /INFERASANLIBS is not allowed in .drectve
        -D_DISABLE_VECTOR_ANNOTATION
    }
}*/

int main(int argc, const char* argv[]) {
    puts("hello world!");
    return 0;
}
