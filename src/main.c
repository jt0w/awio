#define CHIMERA_IMPLEMENTATION
#define CHIMERA_STRIP_PREFIX
#include <chimera.h>
#undef log

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define UNUSED(x) ((void)x)
#include "renderer.h"

#define VELOCITY 500

#define WIDTH 680
#define HEIGHT 420

#define START_X WIDTH / 2
#define START_Y HEIGHT / 2

#define SIDE_LENGTH 100

const static uint32_t colors[] = {
  0xFFFF0000,
  0xFF00FF00,
  0xFF0000FF,
};
const static size_t colors_c = sizeof (colors) / sizeof (colors[0]);
struct {
  bool quit;
  int x;
  int y;
  int dx;
  int dy;
  size_t ci;
} state = {
  .x = START_X,
  .y = START_Y,
  .dx = VELOCITY,
  .dy = VELOCITY,
};

void update(int dt) {
    int cx = state.x + state.dx * (dt / 1000);
    int cy = state.y + state.dy * (dt / 1000);
    if (cx + SIDE_LENGTH >= WIDTH  || cx <= 0) {
      state.dx *= -1;
      if (state.ci == colors_c - 1) state.ci = 0;
      else state.ci++;
    }
    if (cy + SIDE_LENGTH >= HEIGHT || cy <= 0) {
      state.dy *= -1;
      if (state.ci == colors_c - 1) state.ci = 0;
      else state.ci++;
    }
    state.x += state.dx * dt / 1000;
    state.y += state.dy * dt / 1000;
}

void render(uint32_t *pixels) {
  Olivec_Canvas oc = olivec_canvas(pixels, WIDTH, HEIGHT, WIDTH);
  olivec_fill(oc, 0xFFFFFFFF);
  olivec_rect(oc, state.x, state.y, SIDE_LENGTH, SIDE_LENGTH, colors[state.ci]);
}

int main(int argc, char **argv) {
  UNUSED(argc);
  UNUSED(argv);
  Renderer renderer = {
    .height = HEIGHT,
    .width =  WIDTH,
    .update = update,
    .render = render,
  };
  renderer_init(&renderer);
  while (renderer_should_run(&renderer)) {}
  return 0;
}
