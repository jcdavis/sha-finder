#include <immintrin.h>
#include <inttypes.h>
#include <pcre.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <tmmintrin.h>
#include <time.h>

#include "find.h"

static long timediff(struct timespec start, struct timespec end) {
  struct timespec temp;
  if ((end.tv_nsec - start.tv_nsec) < 0) {
    temp.tv_sec = end.tv_sec - start.tv_sec - 1;
    temp.tv_nsec = 1000000000L + end.tv_nsec - start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec - start.tv_sec;
    temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return temp.tv_sec * 1000000000L + temp.tv_nsec;
}

long time_impl(const impl_t* impl, const char* string, int len) {
  struct timespec start, end;
  void* data = impl->init();
  clock_gettime(CLOCK_MONOTONIC, &start);
  bool res = impl->contains(string, len, data);
  clock_gettime(CLOCK_MONOTONIC, &end);
  long diff = timediff(start, end);
  printf("%s %d %ld\n", impl->name, res, diff);
  impl->cleanup(data);
  return diff;
}

static char* generate_ascii(int len) {
  char* res = (char*)malloc(len);
  char start = ' ', end = '~';
  for(int i = 0; i < len; i++) {
    res[i] = (char)(start + (rand()%(end+1-start)));
  }
  /*for(int i = len-40; i < len; i++) {
    res[i] = '0';
  }*/
  return res;
}

int main(int argc, char** argv) {
  const int len = 1024*1024*1024;
  const char* random_data = generate_ascii(len);
  
  for (int i = 0; i < num_impls; i++)
    time_impl(impls[i], random_data, len);
  return 0;
}
