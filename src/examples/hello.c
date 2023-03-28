#include <stdio.h>

/*cxe{
    -std=c11
    -Wall -Werror
    -if (--target=[darwin]) { # compiling for macOS
        -pre { mkdir -p ../../bin/macos }
        -o ../../bin/macos/hello
    }
    -if (--target=[windows]) { # compiling for Windows
        -pre { mkdir -p ../../bin/windows }
        -o ../../bin/windows/hello.exe
    }
}*/

int main(int argc, const char* argv[]) {
    puts("hello world!");
    return 0;
}
