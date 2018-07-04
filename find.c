#include <immintrin.h>
#include <inttypes.h>
#include <pcre.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <tmmintrin.h>
#include <time.h>

typedef void* (*init_fn)(void);
typedef bool (*sha_contains_fn)(const char* buf, int len, void* data);
typedef void (*cleanup_fn)(void*);

typedef struct {
  const init_fn init;
  const sha_contains_fn contains;
  const cleanup_fn cleanup;
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

impl_t baseline_impl = {init_table, contains_sha, cleanup_table, "Baseline loop"};
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

typedef struct {
  __m256i doublemask;
  __m256i doubleshift;
  const char* table;
} vector_data;

void* init_vectorized() {
  vector_data* data;
  posix_memalign((void**)&data, 32, sizeof(vector_data));
  data->table = (const char*)init_table();
  const __m128i mask = _mm_setr_epi8(
    0b00001000,
    0b01011000, 0b01011000, 0b01011000, 0b01011000, 0b01011000, 0b01011000,
    0b00001000, 0b00001000, 0b00001000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000
  );
  const __m128i shift = _mm_setr_epi8(
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  );
  data->doublemask = _mm256_broadcastsi128_si256(mask);
  data->doubleshift = _mm256_broadcastsi128_si256(shift);
  return data; 
}

uint32_t check_vector(const __m256i data, const __m256i doublemask, const __m256i doubleshift) {
  /*const __m128i mask = _mm_setr_epi8(
    0b00001000,
    0b01011000, 0b01011000, 0b01011000, 0b01011000, 0b01011000, 0b01011000,
    0b00001000, 0b00001000, 0b00001000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000
  );
  const __m128i shift = _mm_setr_epi8(
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  );
  const __m256i doublemask = _mm256_broadcastsi128_si256(mask);
  const __m256i doubleshift = _mm256_broadcastsi128_si256(shift);*/

  __m256i hi_nibble = _mm256_srli_epi32(data, 4) & _mm256_set1_epi8(0x0f);
  __m256i lo_nibble = data & _mm256_set1_epi8(0x0f);
  __m256i masks = _mm256_shuffle_epi8(doublemask, lo_nibble);
  __m256i shifted_bits = _mm256_shuffle_epi8(doubleshift, hi_nibble);
  __m256i matches = _mm256_cmpeq_epi8(_mm256_and_si256(masks, shifted_bits), _mm256_setzero_si256());
  return _mm256_movemask_epi8(matches);
}

bool contains_vectorized(const char* str, int len, void* data) {
  const vector_data* vd = (const vector_data*)data;
  int pos = 0, run = 0;
  while(pos + sizeof(__m256i) < len) {
    uint32_t bits = check_vector(_mm256_loadu_si256((__m256i*)(str+pos)),
      vd->doublemask, vd->doubleshift);
    run += _tzcnt_u32(bits);
    if (run >= 40)
      return true;
    if (bits != 0)
      run = _lzcnt_u32(bits);
    pos += sizeof(__m256i);
  }
  //Finish the remaining charracters
  while(pos < len) {
    run = (run - vd->table[str[pos]])&vd->table[str[pos]];
    if (run == 40)
      return true;
    pos++;
  }
  return false;
}

impl_t vectorized_impl = {init_vectorized, contains_vectorized, cleanup_table, "Vectorized"};

bool contains_vectorized_BM(const char* str, int len, void* data) {
  const vector_data* vd = (const vector_data*)data;
  int i = shalen-1;
  while(i < len) {
    uint32_t bits = check_vector(_mm256_loadu_si256((__m256i*)(str+i-sizeof(__m256i))),
      vd->doublemask, vd->doubleshift);
    int lzcnt = _lzcnt_u32(bits);
    i -= lzcnt;
    if (lzcnt == 32) {
      int start = i-7; 
      while(i >= start && vd->table[str[i]] != 0)
        i--;
      if (i < start)
        return true;
    }
    i += shalen;
  }
  return false;
}

impl_t vectorized_bm_impl = {init_vectorized, contains_vectorized_BM, cleanup_table, "Vectorized-BM"};

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
  
  time_impl(&baseline_impl, random_data, len);
  time_impl(&regex_impl, random_data, len);
  time_impl(&branchfreelut_impl, random_data, len);
  time_impl(&boyermoore_impl, random_data, len);
  time_impl(&vectorized_impl, random_data, len);
  time_impl(&vectorized_bm_impl, random_data, len);
  return 0;
}
