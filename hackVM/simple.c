#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
  char* s;
  s = strdup("Holberton");
  if (s == NULL) {
    fprintf(stderr, "can't allocate mem with malloc");
    return 1;
  }
  uint64_t i = 0;

  while (s) {
    printf("[%lu] %s (%p)\n", i, s, (void*)s);
    sleep(1);
    i++;
  }
  free(s);
  return 0;
}