#include "renderer.h"
#define UNUSED(x) ((void)x)

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

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
  /* Sent by the compositor when it's no longer using this buffer */
  UNUSED(data);
  wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};


static struct wl_buffer *draw_frame(Renderer *renderer) {
  int stride = renderer->width * 4;
  int size = stride * renderer->height;

  int fd = allocate_shm_file(size);
  if (fd == -1) {
    return NULL;
  }

  uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(renderer->wl_shm, fd, size);
  struct wl_buffer *buffer = wl_shm_pool_create_buffer(
      pool, 0, renderer->width, renderer->height, stride, WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  renderer->render(data);

  munmap(data, size);
  wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
  return buffer;
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  uint32_t serial) {
  Renderer *renderer = data;

  xdg_surface_ack_configure(xdg_surface, serial);

  struct wl_buffer *buffer = draw_frame(renderer);
  wl_surface_attach(renderer->wl_surface, buffer, 0, 0);
  wl_surface_commit(renderer->wl_surface);
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
  Renderer *renderer = data;
  UNUSED(xdg_toplevel);
  renderer->quit = true;
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

static const struct wl_callback_listener wl_surface_frame_listener;

static void wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time) {
  Renderer *renderer = data;
  
  wl_callback_destroy(cb);

  cb = wl_surface_frame(renderer->wl_surface);
  wl_callback_add_listener(cb, &wl_surface_frame_listener, renderer);

  if (renderer->last_frame != 0) {
    renderer->dt = time - renderer->last_frame;
    renderer->update(renderer->dt);
  }

  struct wl_buffer *buffer = draw_frame(renderer);
  wl_surface_attach(renderer->wl_surface, buffer, 0, 0);
  wl_surface_damage_buffer(renderer->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
  wl_surface_commit(renderer->wl_surface);

  renderer->last_frame = time;
}

static const struct wl_callback_listener wl_surface_frame_listener = {
  .done = wl_surface_frame_done,
};


static void registry_global(void *data, struct wl_registry *wl_registry,
                            uint32_t name, const char *interface,
                            uint32_t version) {

  Renderer *renderer = data;
#ifdef AWIO_DEBUG
  chimera_log(CHIMERA_INFO, "interface: '%s', version: %d, name: %d", interface,
              version, name);
#endif
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    renderer->wl_compositor =
        wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    renderer->wl_shm =
        wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    renderer->xdg_wm_base =
        wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(renderer->xdg_wm_base, &xdg_wm_base_listener, renderer);
  } else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
  renderer->decoration_manager = wl_registry_bind(wl_registry, name, 
      &zxdg_decoration_manager_v1_interface, version);
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

bool renderer_init(Renderer *renderer) {
  renderer->wl_display = wl_display_connect(NULL);
  if (renderer->wl_display == NULL) {
    chimera_log(CHIMERA_ERROR, "failed to create display");
    return false;
  }

  struct wl_registry *registry = wl_display_get_registry(renderer->wl_display);
  wl_registry_add_listener(registry, &wl_registry_listener, renderer);
  wl_display_roundtrip(renderer->wl_display);
  renderer->wl_surface = wl_compositor_create_surface(renderer->wl_compositor);
  renderer->xdg_surface =
      xdg_wm_base_get_xdg_surface(renderer->xdg_wm_base, renderer->wl_surface);
  xdg_surface_add_listener(renderer->xdg_surface, &xdg_surface_listener, renderer);
  renderer->xdg_toplevel = xdg_surface_get_toplevel(renderer->xdg_surface);
  xdg_toplevel_add_listener(renderer->xdg_toplevel, &xdg_toplevel_listener, renderer);
  xdg_toplevel_set_title(renderer->xdg_toplevel, "awio");
  xdg_toplevel_set_min_size(renderer->xdg_toplevel, renderer->width, renderer->height);
  xdg_toplevel_set_max_size(renderer->xdg_toplevel,renderer->width, renderer->height);
  renderer->decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
      renderer->decoration_manager, renderer->xdg_toplevel);
  zxdg_toplevel_decoration_v1_set_mode(renderer->decoration, 
      ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
  wl_surface_commit(renderer->wl_surface);

  struct wl_callback *cb = wl_surface_frame(renderer->wl_surface);
  wl_callback_add_listener(cb, &wl_surface_frame_listener, renderer);

  return true;
}

bool renderer_should_run(Renderer *renderer) {
  return wl_display_dispatch(renderer->wl_display) && !renderer->quit;
}

