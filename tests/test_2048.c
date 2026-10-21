#include "stub.h"

#include "../apps/2048/main.c"

static void row(const uint8_t* v) {
  memset(&g, 0, sizeof(g));
  for (int c = 0; c < N; ++c) g.cells[c] = v[c];
}

int main(void) {
  stub_app("2048");
  // Pairs merge once, toward the edge.
  row((const uint8_t[]){1, 1, 1, 1});
  CHECK(slide(0));
  CHECK(g.cells[0] == 2 && g.cells[1] == 2 && g.cells[2] == 0 && g.score == 8);
  // A merged tile does not merge again: 4 2 2 -> 4 4.
  row((const uint8_t[]){2, 1, 1, 0});
  slide(0);
  CHECK(g.cells[0] == 2 && g.cells[1] == 2);
  // Right moves toward the right edge: 2 _ 2 4 -> _ _ 4 4.
  row((const uint8_t[]){1, 0, 1, 2});
  slide(1);
  CHECK(g.cells[3] == 2 && g.cells[2] == 2 && g.cells[1] == 0);
  // Up and down work on columns.
  memset(&g, 0, sizeof(g));
  g.cells[3 * N] = 1;
  g.cells[1 * N] = 1;
  CHECK(slide(2) && g.cells[0] == 2);
  CHECK(slide(3) && g.cells[3 * N] == 2);
  // No move reports false.
  row((const uint8_t[]){1, 2, 0, 0});
  CHECK(!slide(0));
  // Game over, then a free cell.
  for (int i = 0; i < N * N; ++i) g.cells[i] = (uint8_t)(i % 2 + (i / 4) % 2 * 2 + 1);
  CHECK(!canMove());
  g.cells[5] = 0;
  CHECK(canMove());
  // A fresh start has two tiles; the best score survives a new game and a reload.
  stub_save_len = -1;
  init();
  int tiles = 0;
  for (int i = 0; i < N * N; ++i) tiles += g.cells[i] != 0;
  CHECK(tiles == 2);
  g.best = 500;
  for (int i = 0; i < N * N; ++i) g.cells[i] = (uint8_t)(i % 2 + (i / 4) % 2 * 2 + 1);
  CHECK(event(X4_CONFIRM) == 1 && g.best == 500 && g.score == 0);
  exit_();
  memset(&g, 0, sizeof(g));
  init();
  CHECK(g.best == 500);
  RENDER();
  return stub_failures != 0;
}
