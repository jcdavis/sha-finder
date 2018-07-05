
benchmark:
	clang -O2 -lcre2 -lpcre -march=haswell -o sha-finder-benchmark *.c
