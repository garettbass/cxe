#!/usr/bin/env

CMDLINE="$0 $@"

#-------------------------------------------------------------------------------

function usage {
    echo "Usage:"
    echo ""
    echo "  sh $0 [options] <sources> [-- ...]"
    echo ""
    echo "Options:"
    echo ""
    echo "  -std=<...>      Set C/C++ standard, e.g -std=c++20"
    echo "  -D<...>[=...]   Define preprocessor macro, e.g.: -DNDEBUG=1"
    echo "  -I<...>         Add an include path"
    echo "  -W<...>         Configure warning level"
    echo "  -O<...>         Set optimization level, e.g.: -O0, -O1, -O2, -O3, -Ofast, -Os"
    echo "  -g              Generate debug symbols."
    echo "  -o <dir>        Set output directory."
    echo "  -v              Enable verbose output."
    echo "  -x <ext>        Specify source language, e.g.: -x c, -x c++"
    echo "  --clean         Delete build artifacts."
    echo "  --macos "..."   Specify macOS-specific flags"
    echo "  --windows "..." Specify windows-specific flags"
    echo "  --            Run the compiled app, passing along remaining arguments."
    echo ""
}

#-------------------------------------------------------------------------------

function realpath {
    local path="${1//\\//}"
    if [ "$path" == "." ]; then
        echo "$(pwd)"
    elif [ "$path" == ".." ]; then
        echo "$(dirname "$(pwd)")"
    else
        echo "$(cd "$(dirname "$path")"; pwd)/$(basename "$path")"
    fi
}

#-------------------------------------------------------------------------------

LIB_NAMES=()
C_CPP_SOURCES=()
MACOS_FLAGS=()
WINDOWS_FLAGS=()
while [ $# -gt 0 ]; do
    case $1 in
        -h|--help)
            usage
            exit 1
        ;;
        -D*|-I*|-O*|-std=*|-W*|-f*|-l*)
            CFLAGS="$CFLAGS $1"
            shift
        ;;
        -c)
            COMPILE_LIB=YES
            CFLAGS="$CFLAGS $1"
            shift
        ;;
        -x)
            CFLAGS="$CFLAGS $1 $2"
            shift
            shift
        ;;
        --macos)
            MACOS_FLAGS+=($2)
            shift
            shift
        ;;
        --windows)
            WINDOWS_FLAGS+=($2)
            shift
            shift
        ;;
        -g)
            DEBUG=YES
            CFLAGS="$CFLAGS $1"
            shift
        ;;
        -v)
            VERBOSE=YES
            CFLAGS="$CFLAGS -DVERBOSE=1"
            shift
        ;;
        -t)
            TIME=YES
            shift
        ;;
        --clean)
            CLEAN=YES
            shift
        ;;
        --)
            RUN=YES
            shift
            break
        ;;
        -*)
            echo "unrecognized option: $1"
            echo ""
            usage
            exit 1
        ;;
        *.cpp|*.cxx|*.c++|*.cc|*.c)
            C_CPP_SOURCE="$(realpath $1)"
            C_CPP_SOURCES+=("$C_CPP_SOURCE")
            shift
        ;;
        *.lib)
            LIB_NAME="$(basename $1 .lib)"
            LIB_NAMES+=("$LIB_NAME")
            shift
        ;;
        *.a)
            LIB_NAME="$(basename $1 .a)"
            LIB_NAMES+=("$LIB_NAME")
            shift
        ;;
    esac
done

if [ ! $C_CPP_SOURCES ]; then
    usage
    exit 1
fi

#-------------------------------------------------------------------------------

function verbose {
    if [ $VERBOSE ]; then
        echo "$@"
    fi
}

#-------------------------------------------------------------------------------

function execute {
    verbose "$@"
    if [ $TIME ]; then
        time "$@" || exit $?
    else
        "$@" || exit $?
    fi
    verbose ""
}

#-------------------------------------------------------------------------------

verbose ""
verbose "$CMDLINE"
verbose ""

#-------------------------------------------------------------------------------

ROOT_DIR="$(realpath $(dirname $(dirname "$0")))"
# echo "ROOT_DIR:"$ROOT_DIR

ROOT_BIN_DIR="$(realpath $ROOT_DIR/bin)"
ROOT_LIB_DIR="$(realpath $ROOT_DIR/lib)"

#-------------------------------------------------------------------------------

CMD_MAIN="${C_CPP_SOURCES[0]}"
BIN_NAME="$(basename ${CMD_MAIN%.*})"

case $(uname | tr '[:upper:]' '[:lower:]') in
    darwin*)
        BUILD_OS=macos
        CMD_DIR="$ROOT_BIN_DIR/macos"
        CMD_BIN="$CMD_DIR/$BIN_NAME"
        CMD_DEP="$CMD_DIR/$BIN_NAME.dep"
        LIB_DIR="$ROOT_LIB_DIR/macos"
        LIB_EXT=".a"
        LIB_BIN="$LIB_DIR/$BIN_NAME$LIB_EXT"
        LIB_DEP="$LIB_DIR/$BIN_NAME.dep"

        for MACOS_FLAG in "${MACOS_FLAGS[@]}"; do
            CFLAGS="${CFLAGS} $MACOS_FLAG"
        done
    ;;
    linux*)
        BUILD_OS=linux
        echo "unsupported operating system"
        exit 1
    ;;
    msys*|mingw*)
        BUILD_OS=windows
        CMD_DIR="$ROOT_BIN_DIR/windows"
        CMD_BIN="$CMD_DIR/$BIN_NAME.exe"
        CMD_DEP="$CMD_DIR/$BIN_NAME.dep"
        LIB_DIR="$ROOT_LIB_DIR/windows"
        LIB_EXT=".lib"
        LIB_BIN="$LIB_DIR/$BIN_NAME$LIB_EXT"
        LIB_DEP="$LIB_DIR/$BIN_NAME.dep"
        CFLAGS="${CFLAGS} -D_CRT_SECURE_NO_WARNINGS"
        CLANG_CFLAGS=" -fuse-ld=lld"

        for WINDOWS_FLAG in "${WINDOWS_FLAGS[@]}"; do
            CFLAGS="${CFLAGS} $WINDOWS_FLAG"
        done
    ;;
    *)
        echo "unsupported operating system"
        exit 1
    ;;
esac


#-------------------------------------------------------------------------------

CC="${CC:=$(command -v clang || command -v gcc || command -v cc)}"

case "$CC" in
    *clang*)
        CFLAGS="${CFLAGS}${CLANG_CFLAGS}"
    ;;
esac

case $("$CC" --version | tr '[:upper:]' '[:lower:]') in
    *msys*)
        CFLAGS="${CFLAGS} -Wl,--subsystem,windows"
    ;;
esac

#-------------------------------------------------------------------------------

for C_CPP_SOURCE in "${C_CPP_SOURCES[@]}"; do
    CFLAGS="$CFLAGS $C_CPP_SOURCE"
done

for LIB_NAME in "${LIB_NAMES[@]}"; do
    CFLAGS="$CFLAGS $LIB_DIR/$LIB_NAME$LIB_EXT"
done

#-------------------------------------------------------------------------------

if [ $COMPILE_LIB ]; then

    execute mkdir -p "$LIB_DIR"
    execute "$CC" $CFLAGS -o "$LIB_BIN" -MMD -MF "$LIB_DEP"

else

    execute mkdir -p "$CMD_DIR"
    execute "$CC" $CFLAGS -o "$CMD_BIN" -MMD -MF "$CMD_DEP"

    if [ $RUN ]; then
        echo $''
        execute "$CMD_BIN" "$@"
        echo "$CMD_BIN" returned "$?"
    fi

fi

#-------------------------------------------------------------------------------

if [ $CLEAN ]; then
    execute rm -rf  "$CMD_DIR"
fi

#-------------------------------------------------------------------------------

exit

#-------------------------------------------------------------------------------
