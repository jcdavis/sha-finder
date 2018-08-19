BASE_CCOPTS = -std=gnu11 -O3 -march=haswell -Wall -Wextra -Wno-unused-parameter
#GCC wants the libs after sources
LIB_OPTS=-lpcre
benchmark:
	mkdir -p build
	$(CC) $(BASE_CCOPTS) -o build/sha-finder-benchmark impls.c benchmark.c $(LIB_OPTS)
	build/sha-finder-benchmark
ifeq ($(CC),clang)
  SAN_OPTS=-fsanitize=memory -fsanitize=undefined
else
  SAN_OPTS=
endif

tests:
	mkdir -p build
	$(CC) $(BASE_CCOPTS) $(SAN_OPTS) -o build/tests impls.c tests.c $(LIB_OPTS)
	build/tests
