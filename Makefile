BASE_CCOPTS = -std=gnu11 -O2 -lpcre -march=haswell -Wall -Wextra -Wno-unused-parameter
benchmark:
	mkdir -p build
	clang $(BASE_CCOPTS) -o build/sha-finder-benchmark impls.c benchmark.c
	build/sha-finder-benchmark
tests:
	mkdir -p build
	clang $(BASE_CCOPTS) -fsanitize=memory -fsanitize=undefined -o build/tests impls.c tests.c
	build/tests
