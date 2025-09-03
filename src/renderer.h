#ifndef _RENDERER_H_
#define _RENDERER_H_

#define CHIMERA_STRIP_PREFIX
#include <chimera.h>
#undef log

#include <wayland-client.h>
#include <xdg-shell-protocol.h>
#include <xdg-decoration.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>


typedef struct Renderer Renderer;

typedef void (*Update_F)(int dt);
typedef void (*Render_F)(uint32_t* pixels);

struct Renderer {
  const int height;
  const int width;

  bool quit;
  int dt;
  uint32_t last_frame;

  struct wl_display *wl_display;
  struct wl_compositor *wl_compositor;
  struct wl_shm *wl_shm;
  struct wl_surface *wl_surface;

  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct xdg_wm_base *xdg_wm_base;

  struct zxdg_decoration_manager_v1 *decoration_manager;
  struct zxdg_toplevel_decoration_v1 *decoration;

  struct wl_buffer *buffer;
   
  Update_F update;
  Render_F render;
};

bool renderer_init(Renderer *renderer);
bool renderer_should_run(Renderer *renderer);
#endif // _RENDERER_H_
