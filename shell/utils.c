#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
const char* SHELL_NAME = "my_shell";
static int pipe_fds[2] = {3, 4};

static void strip_quotes(char* str) {
  char* read_ptr = str;
  char* write_ptr = str;

  while (*read_ptr != '\0') {
    if (*read_ptr != '"' && *read_ptr != '\'') {
      *write_ptr = *read_ptr;
      write_ptr++;
    }
    read_ptr++;
  }
  *write_ptr = '\0';
}

char* read_input() {
  printf("%s>> ", SHELL_NAME);

  char* line = NULL;
  size_t capacity = 0;
  getline(&line, &capacity, stdin);

  return line;
}
static char* next_non_empty(char** line) {
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
      strip_quotes(token);
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
static void wait_for_child(pid_t child_pid) {
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

static void run_child(cmd_struct* cmd,
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

static inline void init_new_pipe() {
  pipe(pipe_fds);
}

static void init_child(pipline_struct* pipline, const int current_cmd) {
  if (current_cmd == 0) {
    close(pipe_fds[0]);
    run_child(pipline->cmds[0], pipline->cmds[current_cmd]->redirect[0],
              pipline->cmds[current_cmd]->redirect[1]);
  } else if (current_cmd == pipline->n_cmds - 1) {
    run_child(pipline->cmds[current_cmd],
              pipline->cmds[current_cmd]->redirect[0],
              pipline->cmds[current_cmd]->redirect[1]);
  } else {
    run_child(pipline->cmds[current_cmd],
              pipline->cmds[current_cmd]->redirect[0],
              pipline->cmds[current_cmd]->redirect[1]);
  }
}

static void set_redirect(cmd_struct* cmd, int input, int output) {
  cmd->redirect[0] = input;
  cmd->redirect[1] = output;
}

static void redirect(pipline_struct* pipline,
                     int current_cmd,
                     int input,
                     int output,
                     int old_pipe_read) {
  if (current_cmd == 0)
    set_redirect(pipline->cmds[0], -1, output);
  else if (current_cmd == pipline->n_cmds - 1)
    set_redirect(pipline->cmds[current_cmd], input, -1);
  else
    set_redirect(pipline->cmds[current_cmd], old_pipe_read, output);
}

static void run_pipe(pipline_struct* pipline, int current_cmd) {
  if (current_cmd >= pipline->n_cmds)
    return;
  int old_pipe_read = -1;
  if (current_cmd != 0 && current_cmd != pipline->n_cmds - 1) {
    old_pipe_read = pipe_fds[0];
    init_new_pipe();
  }

  redirect(pipline, current_cmd, pipe_fds[0], pipe_fds[1], old_pipe_read);

  pid_t left_child_pid = fork();
  if (left_child_pid == 0) {
    init_child(pipline, current_cmd);
  } else {
    wait_for_child(left_child_pid);
    if (old_pipe_read != -1)
      close(old_pipe_read);
    close(pipe_fds[1]);
    run_pipe(pipline, current_cmd + 1);
  }
}
static void one_arg(pipline_struct* pipline) {
  pid_t pid = fork();

  if (pid == 0) {
    run_child(pipline->cmds[0], -1, -1);
  } else {
    wait_for_child(pid);
  }
}
void run_command(pipline_struct* parsed_line) {
  if (parsed_line->n_cmds == 1) {
    one_arg(parsed_line);
  } else {
    init_new_pipe();
    run_pipe(parsed_line, 0);
  }
}