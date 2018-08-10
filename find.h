typedef void* (*init_fn)(void);
typedef bool (*sha_contains_fn)(const char* buf, int len, void* data);
typedef void (*cleanup_fn)(void*);

typedef struct {
  const init_fn init;
  const sha_contains_fn contains;
  const cleanup_fn cleanup;
  const char* name;
} impl_t;

extern const impl_t* impls[];
extern const int num_impls;
