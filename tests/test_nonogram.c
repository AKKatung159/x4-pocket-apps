#include "stub.h"

#include "../apps/nonogram/main.c"

static void fillSolution(void) {
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) marks[y * MAX + x] = filled(x, y) ? FILLED : (x + y) % 3 == 0 ? CROSSED : EMPTY;
}

int main(void) {
  stub_app("nonogram");
  // Rules: clues from the solution, bad grids rejected, crosses ignored.
  CHECK(parse(".##.##./#######/.#####./..###../...#..."));
  CHECK(w == 7 && h == 5);
  uint8_t c[MAX_CLUES];
  CHECK(clues(0, 0, c) == 2 && c[0] == 2 && c[1] == 2);
  CHECK(clues(3, 1, c) == 1 && c[0] == 4);
  CHECK(!parse("") && !parse("##/#") && !parse("################"));
  CHECK(parse("..../.##.") && clues(0, 0, c) == 0);
  CHECK(parse("#./.#"));
  memset(prog.marks, 0, sizeof(prog.marks));
  CHECK(!checkSolved());
  marks[0] = FILLED;
  marks[MAX + 1] = FILLED;
  marks[1] = CROSSED;
  CHECK(checkSolved());
  marks[MAX] = FILLED;
  CHECK(!checkSolved());

  // The shipped puzzles; names stay hidden until solved.
  stub_dir = "apps/nonogram";
  stub_save_len = -1;
  init();
  CHECK(count == 8 && strcmp(names[0], "Heart") == 0 && strcmp(names[7], "Note") == 0);
  RENDER();
  CHECK(stub_drew("#1") && !stub_drew("Heart") && strcmp(stub_title, "Nonogram") == 0);
  CHECK(event(X4_CONFIRM) && playing && w == 10 && h == 10);
  RENDER();
  CHECK(strcmp(stub_title, "Nonogram #1") == 0 && strcmp(stub_hint, "Fill") == 0);
  // Moving wraps; Confirm fills, hold crosses, a second press clears.
  event(X4_LEFT);
  CHECK(cx == w - 1);
  event(X4_CONFIRM);
  CHECK(marks[cx] == FILLED);
  event(X4_CONFIRM);
  CHECK(marks[cx] == EMPTY);
  event(X4_CONFIRM_HOLD);
  CHECK(marks[cx] == CROSSED);
  // Solving it ticks it, shows the name, and Confirm goes back to the list.
  fillSolution();
  marks[0] = filled(0, 0) ? EMPTY : FILLED;  // one cell off, then the last move solves it
  cx = cy = 0;
  event(X4_CONFIRM);
  CHECK(solvedNow && isSolved(0));
  RENDER();
  CHECK(strcmp(stub_title, "Heart") == 0 && stub_drew("You win!") && stub_refresh == X4_HALF);
  CHECK(event(X4_LEFT) == 0);
  CHECK(event(X4_CONFIRM) && !playing);
  RENDER();
  CHECK(stub_drew("Heart") && stub_drew("Solved"));
  // Marks and ticks survive a reload; the list reopens on the last puzzle.
  event(X4_DOWN);
  event(X4_CONFIRM);
  marks[5] = FILLED;
  exit_();
  memset(&prog, 0, sizeof(prog));
  sel = 0;
  playing = 0;
  init();
  CHECK(isSolved(0) && !isSolved(1) && prog.current == 1 && sel == 1 && marks[5] == FILLED);
  event(X4_CONFIRM);
  CHECK(prog.marks[5] == FILLED);
  // Opening another puzzle starts it blank.
  playing = 0;
  sel = 2;
  event(X4_CONFIRM);
  CHECK(prog.current == 2 && prog.marks[5] == EMPTY);
  // Thai text.
  stub_lang = 1;
  playing = 0;
  RENDER();
  CHECK(strcmp(stub_title, "โนโนแกรม") == 0 && stub_drew("เสร็จแล้ว"));
  stub_lang = 0;
  // No puzzles: a message and no puzzle opens.
  stub_dir = "build/tests/none";
  stub_save_len = -1;
  playing = 0;
  init();
  CHECK(count == 0 && event(X4_CONFIRM) == 0);
  RENDER();
  CHECK(stub_drew("No puzzles. Copy puzzles.tsv to /apps/nonogram"));
  return stub_failures != 0;
}
