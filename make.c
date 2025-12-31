#include <stdbool.h>

#define CHIMERA_IMPLEMENTATION
#define CHIMERA_STRIP_PREFIX
#include <chimera.h>

Cmd cmd = {0};

#define build_dir "build/"

typedef enum {
  MODE_ERROR,
  MODE_DEV,
  MODE_DEBUG,
  MODE_RELEASE,
} CompMode;

int main(int argc, char **argv) {
  rebuild_file(argv, argc);
  create_dir(build_dir);
  shift(argv, argc);

  Flag run = parse_boolean_flag("-run", "-r", false);
  Flag mode_flag = parse_str_flag("-mode", "-m", "dev");
  CompMode mode = MODE_ERROR;
  if (strcmp(mode_flag.as.str, "dev") == 0)
    mode = MODE_DEV;
  if (strcmp(mode_flag.as.str, "debug") == 0)
    mode = MODE_DEBUG;
  if (strcmp(mode_flag.as.str, "release") == 0)
    mode = MODE_RELEASE;
  if (mode == MODE_ERROR) {
    log(ERROR,
        "`%s` is not one of the following valid compilation modes: `debug` "
        "`release`",
        mode_flag.as.str);
    return 1;
  }
  Flag debugger = parse_str_flag("-debugger", "-d", "gf2");

  cmd_push(&cmd, CHIMERA_COMPILER);
  cmd_push(&cmd, "-Wall", "-Wextra", "-Wno-missing-braces",
           "-Wno-unused-parameter", "-ggdb", "-std=gnu23");
  switch (mode) {
  case MODE_DEV:
    break;
  case MODE_DEBUG:
    cmd_push(&cmd, "-DAWIO_DEBUG");
    break;
  case MODE_RELEASE:
    cmd_push(&cmd, "-DAWIO_RELEASE");
    break;
  default:
    log(ERROR, ": unknown compilation mode");
    return 1;
  }
  cmd_push(&cmd, "src/main.c");
  cmd_push(&cmd, "-lSDL3");
  cmd_push(&cmd, "-lm");
  cmd_push(&cmd, "-o", "build/awio");
  if (!cmd_exec(&cmd))
    return 1;

  if (run.as.boolean) {
    cmd.count = 0;
    if (mode == MODE_DEBUG)
      cmd_push(&cmd, debugger.as.str);
    cmd_push(&cmd, build_dir "awio");
    if (!cmd_exec(&cmd))
      return 1;
  }
  return 0;
}
