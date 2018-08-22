#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "impls.h"

typedef struct {
  const char* str;
  bool expected;
} test_case;

const test_case cases[] = {
  {"", false},
  {"a", false},
  {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", false},
  {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", true},
  {"abcdefABCDEF0123456789abcdefABCDEF012345", true},
  {"zabcdefABCDEF0123456789abcdefABCDEF012345", true},
  {"zabcdefABCDEF0123456789abcdefABCDEF01234", false},
  {"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzabcdefABCDEF0123456789abcdefABCDEF012345", true}, // Check trailing from first 32
  {"\u00A3\uC97D", false}
};
const int num_cases = sizeof(cases) / sizeof(test_case);

int main(int argc, char** argv) {
  int fail = 0;
  for(int i = 0; i < num_impls; i++) {
    void* data = impls[i]->init();
    printf("Checking %s\n", impls[i]->name);
    for (int j = 0; j < num_cases; j++) {
      bool result = impls[i]->contains(cases[j].str, strlen(cases[j].str), data);
      if (result != cases[j].expected) {
        fail = 1;
        printf("%s: for %s expected %d but got %d!\n", impls[i]->name, cases[j].str, cases[j].expected, result);
      }
    }
    impls[i]->cleanup(data);
  }
  return fail;
}
