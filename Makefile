
benchmark:
	mkdir -p build
	clang -O2 -lpcre -march=haswell -Wall -Wextra -Wno-unused-parameter -o build/sha-finder-benchmark *.c
