
benchmark:
	mkdir -p build
	clang -O2 -lpcre -march=haswell -Wall -Wextra -Wno-unused-parameter -Wno-char-subscripts -o build/sha-finder-benchmark *.c
