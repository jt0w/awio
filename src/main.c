#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float x;
  float y;
} V2;

#define V2add(A, B) ((V2){((A.x) + (B.x)), ((A.y) + B.y)})
#define V2sub(A, B) ((V2){((A.x) - (B.x)), ((A.y) - B.y)})
#define V2mul(A, B) ((V2){((A.x) * (B.x)), ((A.y) * B.y)})
#define V2div(A, B) ((V2){((A.x) / (B.x)), ((A.y) / B.y)})

#define ASSERT(_x, ...)                       \
  do {                                        \
    if (!(_x)) {                              \
      fprintf(stderr, __VA_ARGS__);           \
      exit(1);                                \
    }                                         \
  } while (0)

#define MINI_MAP_PLAYER_SIZE 20

static const V2 SCREEN_SIZE = {620, 400};

static const V2 GRID_SIZE = {8, 8};
static const V2 GRID_CELL_SIZE = V2div(SCREEN_SIZE, GRID_SIZE);

struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  bool quit;

  float mouse_x;
  float mouse_y;
  V2 pos;
  V2 p1;
  V2 p2;
  V2 p3;
} state = {
    .quit = false,
    .pos = {4.5, 4.5},
};

void drawRect(V2 pos, V2 size);

void update() {
  SDL_GetMouseState(&state.p1.x, &state.p1.y);
  state.p1 = V2div(state.p1, GRID_CELL_SIZE);
  printf("p1(%lf|%lf)\n", state.p1.x, state.p1.y);
  V2 d = V2sub(state.p1, state.pos);
  // printf("dx = %lf\n", d.x);
  // reference: y = mx + b
  //            m = dy / dx
  //            b = y - mx
  //            x = (y - b) / m
  float m = d.y / d.x;
  float b = state.pos.y - m * state.pos.x;
  if (d.x != 0) {
    if (d.x > 0)
      state.p2.x = ceil(state.p1.x);
    if (d.x < 0)
      state.p2.x = floor(state.p1.x);
    state.p2.y = m * state.p2.x + b;
  }
  if (d.y != 0) {
    if (d.y > 0)
      state.p3.y = ceil(state.p1.y);
    if (d.y < 0)
      state.p3.y = floor(state.p1.y);
    state.p3.x = (state.p3.y - b) / m;
  }
}

void render() {
  for (size_t x = 0; x < GRID_SIZE.x; ++x) {
    SDL_SetRenderDrawColor(state.renderer, 0xFF, 0, 0, 0xFF);
    SDL_RenderLine(state.renderer, x * GRID_CELL_SIZE.x, 0,
                   x * GRID_CELL_SIZE.x, SCREEN_SIZE.y);
  }
  for (size_t y = 0; y < GRID_SIZE.y; ++y) {
    SDL_SetRenderDrawColor(state.renderer, 0xFF, 0, 0, 0xFF);
    SDL_RenderLine(state.renderer, 0, y * GRID_CELL_SIZE.y, SCREEN_SIZE.x,
                   y * GRID_CELL_SIZE.y);
  }

  SDL_SetRenderDrawColor(state.renderer, 0x00, 0xFF, 0x00, 0xFF);
  drawRect(state.pos, (V2){MINI_MAP_PLAYER_SIZE, MINI_MAP_PLAYER_SIZE});

  SDL_SetRenderDrawColor(state.renderer, 0x00, 0x00, 0xFF, 0xFF);
  drawRect(state.p1, (V2){10, 10});

  SDL_SetRenderDrawColor(state.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderLine(state.renderer,
                 state.pos.x * GRID_CELL_SIZE.x + (MINI_MAP_PLAYER_SIZE / 2),
                 state.pos.y * GRID_CELL_SIZE.y + (MINI_MAP_PLAYER_SIZE / 2),
                 state.p1.x  * GRID_CELL_SIZE.x, state.p1.y * GRID_CELL_SIZE.y);

  SDL_SetRenderDrawColor(state.renderer, 0xFF, 0x00, 0xFF, 0xFF);
  drawRect(state.p2, (V2){5, 5});
  drawRect(state.p3, (V2){5, 5});
}

int main() {
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
