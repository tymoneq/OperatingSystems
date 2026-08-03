#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"
volatile sig_atomic_t runing = 1;

void handle_sigint(int sig) {
  runing = 0;
}

void run_shell() {
  char* line = read_input();
  pipline_struct* parsed_line = parse_line(line);
 
  if (parsed_line == NULL) {
    runing = 0;
    free_pipline(parsed_line);
    free(line);
    return;
  }
  if (parsed_line->n_cmds == 0) {
    free(line);
    free_pipline(parsed_line);
    return;
  }
  run_command(parsed_line);
  free_pipline(parsed_line);
  free(line);
}

int main(int argc, char** argv) {
  struct sigaction sa;
  sa.sa_handler = &handle_sigint;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("Error with sigaction\n");
    return 1;
  }
  while (runing) {
    run_shell();
  }
  return 0;
}