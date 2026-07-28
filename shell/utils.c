#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char* SHELL_NAME = "my_shell";

char* read_input() {
  printf("%s>> ", SHELL_NAME);

  char* line = NULL;
  size_t capacity = 0;
  getline(&line, &capacity, stdin);

  return line;
}
char* next_non_empty(char** line) {
  char* tok;
  while ((tok = strsep(line, TOKEN_SEP)) && !*tok)
    ;
  return tok;
}

cmd_struct* parse_line(char* line) {
  char* copy = strndup(line, MAX_LEN);
  char* token;
  int i = 0;

  cmd_struct* ret = calloc(sizeof(cmd_struct) + MAX_LEN * sizeof(char*), 1);
  while (token = next_non_empty(&copy)) {
    ret->args[i++] = token;
  }
  ret->progname = ret->args[0];
  ret->redirect[0] = ret->redirect[1] = -1;

  return ret;
}