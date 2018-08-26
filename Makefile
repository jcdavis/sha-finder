CFLAGS= -std=gnu11 -O3 -march=haswell -Wall -Wextra -Wno-unused-parameter
LD_LIBS=-lpcre
benchmark:
	mkdir -p build
	$(CC) $(CFLAGS) -o build/sha-finder-benchmark impls.c benchmark.c $(LD_LIBS)
	build/sha-finder-benchmark
ifeq ($(CC),clang)
  SAN_OPTS=-fsanitize=address -fsanitize=undefined
else
  SAN_OPTS=
endif

tests:
	mkdir -p build
	$(CC) $(CFLAGS) $(SAN_OPTS) -o build/tests impls.c tests.c $(LD_LIBS)
	build/tests
