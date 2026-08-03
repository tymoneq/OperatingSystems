#pragma once
#define TOKEN_SEP " \t\n\r"
#define true 1
#define false 0
#define bool int32_t
#define MAX_LEN 1024

typedef struct {
  /** The name of the executable. */
  char *progname;
  int num_of_args;

  /**
   * IO redirections; redirect[i] should be used as fd i in the child.
   * A value of -1 indicates no redirect.
   */
  int redirect[2];

  /** The arguments; must be NULL-terminated. */
  char *args[];
} cmd_struct;

typedef struct {
  int n_cmds;
  cmd_struct *cmds[];
} pipline_struct;

char *read_input();
pipline_struct *parse_line(char *line);
void run_command(pipline_struct *parsed_line);
void free_pipline(pipline_struct *pipline);