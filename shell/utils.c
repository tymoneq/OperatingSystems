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

pipline_struct* parse_line(char* line) {
  char* copy = strndup(line, MAX_LEN);
  char* token;
  int i = 0;

  pipline_struct* ret =
      calloc(1, sizeof(pipline_struct) + (MAX_LEN * sizeof(cmd_struct*)));
  ret->cmds[0] = calloc(1, sizeof(cmd_struct) + MAX_LEN * sizeof(char*));

  while (token = next_non_empty(&copy)) {
    // create new pipe element
    if (strcmp("|", token) == 0) {
      ret->cmds[ret->n_cmds]->progname = ret->cmds[ret->n_cmds]->args[0];
      ret->cmds[ret->n_cmds]->redirect[0] =
          ret->cmds[ret->n_cmds]->redirect[1] = -1;
      ++ret->n_cmds;
      i = 0;
      ret->cmds[ret->n_cmds] =
          calloc(1, sizeof(cmd_struct) + MAX_LEN * sizeof(char*));
    } else {
      ret->cmds[ret->n_cmds]->args[i++] = token;
    }
  }
  ret->cmds[ret->n_cmds]->progname = ret->cmds[ret->n_cmds]->args[0];
  ret->cmds[ret->n_cmds]->redirect[0] = ret->cmds[ret->n_cmds]->redirect[1] =
      -1;
  ++ret->n_cmds;

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

static void execute_cmd(cmd_struct* parsed_line) {
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

void run_command(pipline_struct* parsed_line) {
  if (parsed_line->n_cmds == 1) {
    execute_cmd(parsed_line->cmds[0]);
  } else {
  }
}