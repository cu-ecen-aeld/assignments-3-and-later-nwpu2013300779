CROSS=aarch64-none-linux-gnu-
CC=${CROSS}gcc
echo $CC
${CC} -v > cross-compile.txt 2>&1
${CC} -print-sysroot >> cross-compile.txt 2>&1

