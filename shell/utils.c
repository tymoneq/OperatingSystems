#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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

static void child_proccess(cmd_struct* parsed_line) {
  int status = execvp(parsed_line->progname, parsed_line->args);
  if (status == -1) 
    printf("Command not found %s\n", parsed_line->progname);
  
}
static void parent_proccess() {
  wait(NULL);
}

void run_command(cmd_struct* parsed_line) {
  if (strcmp("cd", parsed_line->progname) == 0) {
    int success_code = chdir(parsed_line->args[1]);

    if (success_code == -1)
      printf("wrong path\n");

  } else {
    pid_t pid = fork();

    //  child proccess if pid == 0
    if (pid == 0)
      child_proccess(parsed_line);

    if (pid != 0)
      parent_proccess();
  }
}