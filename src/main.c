#define CHIMERA_IMPLEMENTATION
#define CHIMERA_STRIP_PREFIX
#include <chimera.h>
#undef log

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#include <wayland-client.h>
#include <xdg-shell-protocol.h>

#define UNUSED(x) ((void)x)

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static void randname(char *buf) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  long r = ts.tv_nsec;
  for (int i = 0; i < 6; ++i) {
    buf[i] = 'A' + (r & 15) + (r & 16) * 2;
    r >>= 5;
  }
}

static int create_shm_file(void) {
  int retries = 100;
  do {
    char name[] = "/wl_shm-XXXXXX";
    randname(name + sizeof(name) - 7);
    --retries;
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      shm_unlink(name);
      return fd;
    }
  } while (retries > 0 && errno == EEXIST);
  return -1;
}

int allocate_shm_file(size_t size) {
  int fd = create_shm_file();
  if (fd < 0)
    return -1;
  int ret;
  do {
    ret = ftruncate(fd, size);
  } while (ret < 0 && errno == EINTR);
  if (ret < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

struct {
  bool quit;

  struct wl_display *wl_display;
  struct wl_compositor *wl_compositor;
  struct wl_shm *wl_shm;
  struct wl_surface *wl_surface;

  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct xdg_wm_base *xdg_wm_base;
} state;

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
  /* Sent by the compositor when it's no longer using this buffer */
  UNUSED(data);
  wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

#define WIDTH 640
#define HEIGHT 480

static struct wl_buffer *draw_frame() {
  int stride = WIDTH * 4;
  int size = stride * HEIGHT;

  int fd = allocate_shm_file(size);
  if (fd == -1) {
    return NULL;
  }

  uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(state.wl_shm, fd, size);
  struct wl_buffer *buffer = wl_shm_pool_create_buffer(
      pool, 0, WIDTH, HEIGHT, stride, WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  Olivec_Canvas oc = olivec_canvas(data, WIDTH, HEIGHT, WIDTH);

  olivec_fill(oc, 0xFF181818);
  olivec_circle(oc, WIDTH / 2, HEIGHT / 2, 100, 0xFFFFFFFF);

  munmap(data, size);
  wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
  return buffer;
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  uint32_t serial) {
  UNUSED(data);
  xdg_surface_ack_configure(xdg_surface, serial);

  struct wl_buffer *buffer = draw_frame();
  wl_surface_attach(state.wl_surface, buffer, 0, 0);
  wl_surface_commit(state.wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data,
                                   struct xdg_toplevel *xdg_toplevel,
                                   int32_t width, int32_t height,
                                   struct wl_array *states) {
  UNUSED(data);
  UNUSED(xdg_toplevel);
  UNUSED(width);
  UNUSED(height);
  UNUSED(states);
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
  UNUSED(data);
  UNUSED(xdg_toplevel);
  state.quit = true;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
                             uint32_t serial) {
  UNUSED(data);
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void registry_global(void *data, struct wl_registry *wl_registry,
                            uint32_t name, const char *interface,
                            uint32_t version) {
  UNUSED(data);
#ifdef AWIO_DEBUG
  chimera_log(CHIMERA_INFO, "interface: '%s', version: %d, name: %d", interface,
              version, name);
#endif
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state.wl_compositor =
        wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state.wl_shm =
        wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    state.xdg_wm_base =
        wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(state.xdg_wm_base, &xdg_wm_base_listener, NULL);
  }
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry,
                                   uint32_t name) {
  UNUSED(data);
  UNUSED(wl_registry);
  UNUSED(name);
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

int main(int argc, char **argv) {
  UNUSED(argc);
  UNUSED(argv);

  state.wl_display = wl_display_connect(NULL);
  if (state.wl_display == NULL) {
    chimera_log(CHIMERA_ERROR, "failed to create display");
    return 1;
  }

  struct wl_registry *registry = wl_display_get_registry(state.wl_display);
  wl_registry_add_listener(registry, &wl_registry_listener, NULL);
  wl_display_roundtrip(state.wl_display);
  state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
  state.xdg_surface =
      xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);
  xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, NULL);
  state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
  xdg_toplevel_add_listener(state.xdg_toplevel, &xdg_toplevel_listener, NULL);
  xdg_toplevel_set_title(state.xdg_toplevel, "awio");
  wl_surface_commit(state.wl_surface);

  while (wl_display_dispatch(state.wl_display) && !state.quit) {}

  return 0;
}
