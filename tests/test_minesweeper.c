#include "stub.h"

#include "../apps/minesweeper/main.c"

int main(void) {
  stub_app("minesweeper");
  for (uint32_t seed = 1; seed <= 500; ++seed) {
    stub_seed = seed;
    stub_save_len = -1;
    dirty = 0;
    init();
    CHECK(g.state == FRESH && g.cx == 4 && g.cy == 4);
    event(X4_CONFIRM);  // the first reveal never loses and floods from the start
    CHECK(g.state == PLAYING || g.state == WON);
    int mines = 0;
    for (int i = 0; i < W * H; ++i) mines += (g.cells[i] & MINE) != 0;
    CHECK(mines == MINES);
    for (int dy = -1; dy <= 1; ++dy)
      for (int dx = -1; dx <= 1; ++dx) CHECK(!(g.cells[(4 + dy) * W + 4 + dx] & MINE));
    CHECK((g.cells[4 * W + 4] & COUNT) == 0);
    // Opening every safe cell wins; the game survives a save and reload.
    for (int i = 0; i < W * H && g.state == PLAYING; ++i)
      if (!(g.cells[i] & MINE)) {
        g.cx = (uint8_t)(i % W);
        g.cy = (uint8_t)(i / W);
        event(X4_CONFIRM);
      }
    CHECK(g.state == WON);
    RENDER();
    CHECK(stub_drew("You win!") && stub_refresh == X4_HALF);
    exit_();
    init();
    CHECK(g.state == WON);
    CHECK(event(X4_CONFIRM) == 1 && g.state == FRESH);
    // An open cell cannot be flagged; a flag blocks a reveal; a mine loses.
    event(X4_CONFIRM);
    event(X4_CONFIRM_HOLD);
    CHECK(!(g.cells[g.cy * W + g.cx] & FLAG));
    for (int i = 0; i < W * H; ++i)
      if (g.cells[i] & MINE) {
        g.cx = (uint8_t)(i % W);
        g.cy = (uint8_t)(i / W);
        event(X4_CONFIRM_HOLD);
        CHECK(g.cells[i] & FLAG);
        CHECK(flags() == 1);
        event(X4_CONFIRM);
        CHECK(g.state != LOST);
        event(X4_CONFIRM_HOLD);
        event(X4_CONFIRM);
        break;
      }
    CHECK(g.state == LOST);
    CHECK(event(X4_CONFIRM_HOLD) == 0);
    g.cx = 0;
    event(X4_LEFT);
    CHECK(g.cx == W - 1);  // wraps
    RENDER();
    CHECK(stub_drew("Boom!"));
  }
  // A stuck RNG still lays every mine.
  reset();
  for (int i = 0; i < W * H; ++i) g.cells[i] = 0;
  stub_seed = 0;
  layMines(0, 0);
  int mines = 0;
  for (int i = 0; i < W * H; ++i) mines += (g.cells[i] & MINE) != 0;
  CHECK(mines == MINES);
  // A corrupt save starts a fresh game.
  memset(stub_save, 0xFF, sizeof(stub_save));
  stub_save_len = SAVE_SIZE;
  init();
  CHECK(g.state == FRESH);
  // No move, no write.
  stub_save_len = -1;
  stub_saves = 0;
  dirty = 0;
  init();
  exit_();
  CHECK(stub_saves == 0);
  return stub_failures != 0;
}
