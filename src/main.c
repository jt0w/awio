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

#define RADIUS 100
struct {
  bool quit;
  int x;
  int y;
  int dx;
  int dy;
} state = {
  .x = START_X,
  .y = START_Y,
  .dx = VELOCITY,
  .dy = VELOCITY,
};

void update(int dt) {
    int cx = state.x + state.dx * (dt / 1000);
    int cy = state.y + state.dy * (dt / 1000);
    if (cx + RADIUS >= WIDTH  || cx - RADIUS <= 0) state.dx *= -1;
    if (cy + RADIUS >= HEIGHT || cy - RADIUS <= 0) state.dy *= -1;
    state.x += state.dx * dt / 1000;
    state.y += state.dy * dt / 1000;
    println("updating");
}

void render(uint32_t *pixels) {
  Olivec_Canvas oc = olivec_canvas(pixels, WIDTH, HEIGHT, WIDTH);
  olivec_fill(oc, 0xFFFFFFFF);
  olivec_circle(oc, state.x, state.y, RADIUS, 0xFFFF0000);
  println("rendering");
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
