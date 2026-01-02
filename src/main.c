#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#define EPS 1e-6

typedef struct {
  float x;
  float y;
} V2;

#define V2(x, y) ((V2){(x), (y)})
#define V2add(A, B) ((V2){((A.x) + (B.x)), ((A.y) + B.y)})
#define V2sub(A, B) ((V2){((A.x) - (B.x)), ((A.y) - B.y)})
#define V2mul(A, B) ((V2){((A.x) * (B.x)), ((A.y) * B.y)})
#define V2div(A, B) ((V2){((A.x) / (B.x)), ((A.y) / B.y)})

#define ASSERT(_x, ...)                                                        \
  do {                                                                         \
    if (!(_x)) {                                                               \
      fprintf(stderr, __VA_ARGS__);                                            \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define MINI_MAP_PLAYER_SIZE 20

static const V2 SCREEN_SIZE = {620, 400};
#define GRID_WIDTH 8
#define GRID_HEIGHT 8
static const V2 GRID_SIZE = {GRID_WIDTH, GRID_HEIGHT};
static const V2 GRID_CELL_SIZE = V2div(SCREEN_SIZE, GRID_SIZE);

static const uint8_t MAP[GRID_HEIGHT][GRID_WIDTH] = {
    {1, 1, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  bool quit;

  float mouse_x;
  float mouse_y;
  V2 pos;
  V2 dir;
} state = {
    .quit = false,
    .pos = {6, 6},
};

void drawRect(V2 pos, V2 size);

void update() {
  SDL_GetMouseState(&state.dir.x, &state.dir.y);
  state.dir = V2div(state.dir, GRID_CELL_SIZE);
  // reference: y = mx + b
  //            m = dy / dx
  //            b = y - mx
  //            x = (y - b) / m
}

// TODO: better name
// not just V2 of v2
typedef struct {
  V2 p1;
  V2 p2;
} V2v2;

V2v2 cast_ray(V2 p1, V2 p2) {
  V2 d = V2sub((p2), (p1));
  V2v2 result = {0};

  float m = d.y / d.x;
  float b = p1.y - m * p1.x;
  if (d.x != 0) {
    if (d.x > 0)
      result.p1.x = ceil(p2.x + EPS);
    if (d.x < 0)
      result.p1.x = floor(p2.x - EPS);
    result.p1.y = m * result.p1.x + b;
  }

  if (d.y != 0) {
    if (d.y > 0)
      result.p2.y = ceil(p2.y);
    if (d.y < 0)
      result.p2.y = floor(p2.y);
    result.p2.x = (result.p2.y - b) / m;
  }

  return result;
}

void draw_minimap() {
  SDL_SetRenderDrawColor(state.renderer, 0xFF, 0, 0, 0xFF);
  for (size_t x = 0; x < GRID_SIZE.x; ++x) {
    SDL_RenderLine(state.renderer, x * GRID_CELL_SIZE.x, 0,
                   x * GRID_CELL_SIZE.x, SCREEN_SIZE.y);
  }

  for (size_t y = 0; y < GRID_SIZE.y; ++y) {
    SDL_RenderLine(state.renderer, 0, y * GRID_CELL_SIZE.y, SCREEN_SIZE.x,
                   y * GRID_CELL_SIZE.y);
  }

  SDL_SetRenderDrawColor(state.renderer, 0x00, 0x00, 0xFF, 0xFF);
  for (size_t x = 0; x < GRID_SIZE.x; ++x) {
    for (size_t y = 0; y < GRID_SIZE.y; ++y) {
      if (MAP[y][x] == 1) {
        drawRect(V2(x, y), GRID_CELL_SIZE);
      }
    }
  }

  SDL_SetRenderDrawColor(state.renderer, 0x00, 0xFF, 0x00, 0xFF);
  drawRect(state.pos, (V2){MINI_MAP_PLAYER_SIZE, MINI_MAP_PLAYER_SIZE});

  SDL_SetRenderDrawColor(state.renderer, 0x00, 0x00, 0xFF, 0xFF);
  drawRect(state.dir, (V2){10, 10});

  SDL_SetRenderDrawColor(state.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderLine(state.renderer,
                 state.pos.x * GRID_CELL_SIZE.x + (MINI_MAP_PLAYER_SIZE / 2),
                 state.pos.y * GRID_CELL_SIZE.y + (MINI_MAP_PLAYER_SIZE / 2),
                 state.dir.x * GRID_CELL_SIZE.x,
                 state.dir.y * GRID_CELL_SIZE.y);

  V2v2 points = cast_ray(state.pos, state.dir);
  // while ((MAP[(int)points.p1.y][(int)points.p1.x] != 1 ||
  //         MAP[(int)points.p2.y][(int)points.p2.x] != 1) &&
  //        points.p1.x >= 0 && points.p1.x <= GRID_SIZE.x && points.p1.y >= 0
  //        && points.p1.y <= GRID_SIZE.y) {
  //   points = cast_ray(points.p1, points.p2);
  // }

  V2 p1 = V2(floor(fabs(points.p1.x)), floor(fabs(points.p1.y)));
  V2 p2 = V2(floor(fabs(points.p2.x)), floor(fabs(points.p2.y)));

  SDL_SetRenderDrawColor(state.renderer, 0xFF, 0x00, 0xFF, 0xFF);
  // drawRect(p1, V2(5, 5));
  // drawRect(p2, V2(5, 5));

  SDL_SetRenderDrawColor(state.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  if (!isnan(p1.x) && !isnan(p1.y)) {
    if (MAP[(int)p1.y][((int)p1.x)] == 1 || MAP[(int)p1.y][(int)p1.x-1]) {
      drawRect(points.p1, V2(5, 5));
    }
  }

  if (!isnan(p2.x) && !isnan(p2.y)) {
    if (MAP[(int)p2.y][(int)p2.x] || MAP[(int)p2.y-1][(int)p2.x] == 1) {
      drawRect(points.p2, V2(5, 5));
    }
  }
}

void render() { draw_minimap(); }

int main() {
  breakpoint();
  ASSERT(SDL_Init(SDL_INIT_VIDEO), "SDL failed to initialize: %s\n",
         SDL_GetError());

  state.window = SDL_CreateWindow("awio", SCREEN_SIZE.x, SCREEN_SIZE.y, 0);
  ASSERT(state.window, "SDL failed to create window: %s\n", SDL_GetError());

  state.renderer = SDL_CreateRenderer(state.window, NULL);
  ASSERT(state.renderer, "SDL failed to create renderer: %s\n", SDL_GetError());

  while (!state.quit) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_EVENT_QUIT:
        state.quit = true;
        break;
      }
    }
    update();

    SDL_SetRenderDrawColor(state.renderer, 0x18, 0x18, 0x18, 0xFF);
    SDL_RenderClear(state.renderer);
    render();
    SDL_RenderPresent(state.renderer);
  }

  return 0;
}

// pos is in gridPos
void drawRect(V2 pos, V2 size) {
  SDL_FRect rect = {
      .x = pos.x * GRID_CELL_SIZE.x,
      .y = pos.y * GRID_CELL_SIZE.y,
      .w = size.x,
      .h = size.y,
  };
  SDL_RenderFillRect(state.renderer, &rect);
}
