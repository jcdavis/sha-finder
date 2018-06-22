#include <pcre.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef void* (*init_fn)(void);
typedef bool (*sha_contains_fn)(const char* buf, int len, void* data);
typedef void (*cleanup_fn)(void*);

typedef struct {
  init_fn init;
  sha_contains_fn contains;
  cleanup_fn cleanup;
  const char* name;
} impl_t;

typedef struct {
pcre* re;
const char* re_error;
int re_error_offset;
pcre_extra* re_extra;
pcre_jit_stack* jit_stack;
int ovector[30];
} regex_data;

void* init_regex() {
  regex_data* data = malloc(sizeof(regex_data));
  data->re = pcre_compile("[a-fA-F0-9]{40}", 0, &data->re_error, &data->re_error_offset, NULL);
  data->re_extra = pcre_study(data->re, PCRE_STUDY_JIT_COMPILE, &data->re_error);
  data->jit_stack = pcre_jit_stack_alloc(32*1024, 512*1024);
  pcre_assign_jit_stack(data->re_extra, NULL, data->jit_stack);
  return (void*)data;
}

bool contains_regex(const char* str, int len, void* data) {
  regex_data* rd = (regex_data*)data;
  return pcre_jit_exec(rd->re, rd->re_extra, str, len, 0, 0, rd->ovector, 30, rd->jit_stack) >= 0; 
}

void cleanup_regex(void* data) {
  //TODO
}

impl_t regex_impl = {init_regex, contains_regex, cleanup_regex, "PCRE-JIT"};

bool contains_sha(const char* str, int len, void* data) {
  int run = 0;
  for(int i = 0; i < len; i++) {
    if ((str[i] >= 'a' && str[i] <= 'f') ||
      (str[i] >= 'A' && str[i] <= 'F') ||
      (str[i] >= '0' && str[i] <= '9'))
      run++;
    else
      run = 0;
    if (run == 40)
      return true;
  }
  return false;
}

void* init_table() {
  char* table = malloc(256);
  for(int i = 0; i < 256; i++)
    table[i] = 0;
  for(char c = 'a'; c <= 'f'; c++)
    table[c] = 0xff;
  for(char c = 'A'; c <= 'F'; c++)
    table[c] = 0xff;
  for(char c = '0'; c <= '9'; c++)
    table[c] = 0xff;
  return (void*)table;
}

bool contains_table(const char* str, int len, void* data) {
  const char* table = (const char*)data;
  char run = 0;
  for(int i = 0; i < len; i++) {
    run = (run - table[str[i]])&table[str[i]];
    if (run == 40)
      return true;
  }
  return false;
}

void cleanup_table(void* data) {
 //TODO
}

impl_t branchfreelut_impl = {init_table, contains_table, cleanup_table, "BranchfreeLUT"};

const int shalen = 40;
bool contains_BM(const char* str, int len, void* data) {
  const char* table = (const char*)data;
  int i = shalen-1;
  while(i < len) {
    if (table[str[i]] != 0) {
      int start = i-(shalen-1);
      while(i >= start && table[str[i]] != 0)
        i--;
      if (i < start)
        return true;
    }
    i += shalen;
  }
  return false;
}

impl_t boyermoore_impl = {init_table, contains_BM, cleanup_table, "Boyer-Moore"};

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

long time_impl(impl_t* impl, const char* string, int len) {
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
  //char* random_data = str2;
  //const int len = strlen(str2);
  
  time_impl(&regex_impl, random_data, len);
  time_impl(&branchfreelut_impl, random_data, len);
  time_impl(&boyermoore_impl, random_data, len);
  return 0;
}
