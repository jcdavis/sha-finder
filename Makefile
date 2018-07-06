
benchmark:
	mkdir -p build
	clang -O2 -lpcre -march=haswell -o build/sha-finder-benchmark *.c
