#include "utils.h"

void run_shell() {
  char* line = read_input();
  cmd_struct* parsed_line = parse_line(line);
}

int main(int argc, char** argv) {
  while (1) {
    run_shell();
  }
  return 0;
}