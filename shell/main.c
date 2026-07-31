#include "utils.h"

void run_shell() {
  char* line = read_input();
  pipline_struct* parsed_line = parse_line(line);
  run_command(parsed_line);
}

int main(int argc, char** argv) {
  while (1) {
    run_shell();
  }
  return 0;
}