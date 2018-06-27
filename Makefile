
benchmark:
	clang -O2 -lpcre -march=haswell -o sha-finder-benchmark *.c
