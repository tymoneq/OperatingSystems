#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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

static void child_proccess(cmd_struct* cmd) {
  int status = execvp(cmd->progname, cmd->args);
  if (status == -1)
    printf("Command not found %s\n", cmd->progname);
}
static void parent_proccess(pid_t child_pid) {
  int status;
  waitpid(child_pid, &status, 0);
}

static void run_cd(cmd_struct* cmd) {
  int success_code = chdir(cmd->args[1]);

  if (success_code == -1)
    printf("wrong path\n");
}

static void execute_cmd(cmd_struct* cmd) {
  if (strcmp("cd", cmd->progname) == 0)
    run_cd(cmd);
  else {
    child_proccess(cmd);
  }
}

static void create_child(cmd_struct* cmd,
                         const int pipe_read,
                         const int pipe_write) {
  if (pipe_read != -1) {
    dup2(pipe_read, STDIN_FILENO);
    cmd->redirect[0] = pipe_read;
    close(pipe_read);
  }

  if (pipe_write != -1) {
    dup2(pipe_write, STDOUT_FILENO);
    cmd->redirect[1] = pipe_write;
    close(pipe_write);
  }
  execute_cmd(cmd);
}

static void run_pipe(pipline_struct* pipline) {
  int pipe_fds[2];
  pipe_fds[0] = 4;
  pipe_fds[1] = 5;
  if (pipline->n_cmds == 1) {
    pipe_fds[0] = 0;
    pipe_fds[1] = 1;
  } else {
    pipe(pipe_fds);
  }

  pid_t pid = fork();

  if (pipline->n_cmds == 1) {
    if (pid == 0) {
      create_child(pipline->cmds[0], -1, -1);
    } else {
      parent_proccess(pid);
    }
  } else if (pid == 0) {
    create_child(pipline->cmds[0], -1, pipe_fds[1]);
  } else {
    pid_t pid2 = fork();
    if (pid == 0) {
      create_child(pipline->cmds[1], pipe_fds[0], -1);
    } else {
      close(pipe_fds[0]);
      close(pipe_fds[1]);
      parent_proccess(pid2);
    }
  }
}

void run_command(pipline_struct* parsed_line) {
  run_pipe(parsed_line);
}