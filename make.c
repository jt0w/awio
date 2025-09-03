#include <stdbool.h>

#define CHIMERA_IMPLEMENTATION
#define CHIMERA_STRIP_PREFIX
#include <chimera.h>

Cmd cmd = {0};

#define build_dir "build/"
#define lib_dir "lib/"

#define xdg_shell_link  "https://gitlab.freedesktop.org/wayland/wayland-protocols/-/raw/main/stable/xdg-shell/xdg-shell.xml"
#define xdg_decoration_link "https://gitlab.freedesktop.org/wayland/wayland-protocols/-/raw/main/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"

bool generate_libs() {
  if (!file_exists("lib/xdg-shell.xml")) {
    cmd_push(&cmd, "curl", "-o", lib_dir "xdg-shell.xml", xdg_shell_link);
    if (!cmd_exec(&cmd))
      return false;
    cmd.count = 0;
  }

  if (!file_exists("lib/xdg-shell-protocol.c")) {
    cmd_push(&cmd, "wayland-scanner", "private-code");
    cmd_push(&cmd, "./lib/xdg-shell.xml");
    cmd_push(&cmd, "./lib/xdg-shell-protocol.c");
    if (!cmd_exec(&cmd))
      return false;
    cmd.count = 0;
  }

  if (!file_exists("lib/xdg-shell-protocol.h")) {
    cmd_push(&cmd, "wayland-scanner", "client-header");
    cmd_push(&cmd, "./lib/xdg-shell.xml");
    cmd_push(&cmd, "./lib/xdg-shell-protocol.h");
    if (!cmd_exec(&cmd))
      return false;
    cmd.count = 0;
  }

  if (!file_exists("lib/xdg-decoration.xml")) {
    cmd_push(&cmd, "curl", "-o", lib_dir "xdg-decoration.xml", xdg_decoration_link);
    if (!cmd_exec(&cmd))
      return false;
    cmd.count = 0;
  }


  if (!file_exists("lib/xdg-decoration.c")) {
    cmd_push(&cmd, "wayland-scanner", "public-code");
    cmd_push(&cmd, "./lib/xdg-decoration.xml");
    cmd_push(&cmd, "./lib/xdg-decoration.c");
    if (!cmd_exec(&cmd))
      return false;
    cmd.count = 0;
  }

  if (!file_exists("lib/xdg-decoration.h")) {
    cmd_push(&cmd, "wayland-scanner", "client-header");
    cmd_push(&cmd, "./lib/xdg-decoration.xml");
    cmd_push(&cmd, "./lib/xdg-decoration.h");
    if (!cmd_exec(&cmd))
      return false;
    cmd.count = 0;
  }
  return true;
}

int main(int argc, char **argv) {
  rebuild_file(argv, argc);
  create_dir(build_dir);
  create_dir(lib_dir);
  if (!generate_libs()) return 1;

  cmd_push(&cmd, CHIMERA_COMPILER);
  cmd_push(&cmd, "-Wall", "-Wextra", "-Wno-missing-braces","-ggdb");
  cmd_push(&cmd, "-DAWIO_DEBUG");
  cmd_push(&cmd, "src/main.c");
  cmd_push(&cmd, "src/renderer.c");
  cmd_push(&cmd, "lib/xdg-decoration.c");
  cmd_push(&cmd, "lib/xdg-shell-protocol.c");
  cmd_push(&cmd, "-o", "build/awio");
  cmd_push(&cmd, "-I./lib/");
  cmd_push(&cmd, "-lwayland-client");
  cmd_push(&cmd, "-lrt");
  if (!cmd_exec(&cmd))
    return 1;
}
