benchmark:
	mkdir -p build
	clang -std=gnu11 -O2 -lpcre -march=haswell -Wall -Wextra -Wno-unused-parameter -o build/sha-finder-benchmark *.c
