// Nonogram (picross) for the Pocket SDK. The list shows the puzzles in
// puzzles.tsv, a puzzle's name only once it is solved. In a puzzle the arrows
// move, Confirm fills or clears a cell and holding Confirm marks a cross;
// Confirm on a solved puzzle goes back to the list. The marks of the last
// puzzle played and the solved ticks are saved on exit.
//
// puzzles.tsv: name<TAB>grid, the grid's rows separated by '/', '#' for a
// filled cell, up to 15x15. Clues are computed from the solution.
#include <string.h>

#include "x4.h"

#define FILE_NAME "puzzles.tsv"
#define MAX 15
#define MAX_CLUES ((MAX + 1) / 2)
#define MAX_PUZZLES 64
#define MAX_LINE 1024
#define NAME_LEN 48
#define SAVE_SIZE 236

enum { EMPTY, FILLED, CROSSED };

static struct {
  uint32_t solved[MAX_PUZZLES / 32];
  uint16_t current;  // puzzle whose marks are saved, 0xFFFF for none
  uint8_t marks[MAX * MAX];
} prog;
_Static_assert(sizeof(prog) == SAVE_SIZE, "manifest.json \"save\" must match");

static uint32_t rowAt[MAX_PUZZLES];  // byte offset of each puzzle row
static char names[MAX_PUZZLES][NAME_LEN];
static int count;
static int dirty;

static int playing, sel, first;  // list: cursor and first visible row
static int w, h, cx, cy, solvedNow;
static uint16_t solution[MAX];  // bit x of row y
static uint8_t* const marks = prog.marks;

static int th(void) { return x4_lang() == 1; }
static int isSolved(int i) { return (prog.solved[i / 32] >> (i % 32)) & 1u; }
static int filled(int x, int y) { return (solution[y] >> x) & 1u; }

static void fmtInt(char* out, const char* label, int v) {
  char d[12];
  int n = 0;
  unsigned u = (unsigned)v;
  do d[n++] = (char)('0' + u % 10); while ((u /= 10) > 0);
  while (*label) *out++ = *label++;
  while (n) *out++ = d[--n];
  *out = 0;
}

// Reads row i into buf and returns its grid (after the TAB), or 0.
static char* readRow(int i, char* buf) {
  const int got = x4_read(FILE_NAME, (int)rowAt[i], buf, MAX_LINE);
  if (got <= 0) return 0;
  buf[got] = 0;
  char* nl = strchr(buf, '\n');
  if (nl) *nl = 0;
  const size_t n = strlen(buf);
  if (n && buf[n - 1] == '\r') buf[n - 1] = 0;
  char* tab = strchr(buf, '\t');
  if (!tab) return 0;
  *tab = 0;
  char* end = strchr(tab + 1, '\t');
  if (end) *end = 0;
  return tab + 1;
}

static void indexPuzzles(void) {
  static char row[MAX_LINE + 1];
  count = x4_rows(FILE_NAME, rowAt, MAX_PUZZLES);
  if (count < 0) count = 0;
  for (int i = 0; i < count; ++i) {
    names[i][0] = 0;
    if (!readRow(i, row)) continue;
    const size_t n = strlen(row);
    if (n < NAME_LEN) memcpy(names[i], row, n + 1);
  }
}

// False for an empty, ragged or oversized grid.
static int parse(const char* grid) {
  w = h = 0;
  memset(solution, 0, sizeof(solution));
  int x = 0;
  for (const char* p = grid;; ++p) {
    if (*p == '/' || *p == 0) {
      if (h == 0) w = x;
      if (x != w || w == 0 || h == MAX) return 0;
      ++h;
      x = 0;
      if (*p == 0) return 1;
      continue;
    }
    if (x == MAX) return 0;
    if (*p == '#') solution[h] |= (uint16_t)(1u << x);
    ++x;
  }
}

static int clues(int index, int column, uint8_t out[MAX_CLUES]) {
  int n = 0, run = 0;
  const int len = column ? h : w;
  for (int i = 0; i <= len; ++i) {
    const int on = i < len && (column ? filled(index, i) : filled(i, index));
    if (on) {
      ++run;
    } else if (run > 0) {
      out[n++] = (uint8_t)run;
      run = 0;
    }
  }
  return n;
}

// Solved when the FILLED marks match the solution exactly (crosses are notes).
static int checkSolved(void) {
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      if ((marks[y * MAX + x] == FILLED) != filled(x, y)) return 0;
  return 1;
}

static void openPuzzle(int i) {
  static char row[MAX_LINE + 1];
  const char* grid = readRow(i, row);
  if (!grid || !parse(grid)) {
    x4_log("nonogram: unreadable puzzle");
    return;
  }
  if (prog.current != i) memset(prog.marks, 0, sizeof(prog.marks));
  prog.current = (uint16_t)i;
  cx = cy = 0;
  solvedNow = isSolved(i) && checkSolved();
  playing = 1;
}

X4_EXPORT(init) void init(void) {
  if (x4_load(&prog, sizeof(prog)) != sizeof(prog)) {
    memset(&prog, 0, sizeof(prog));
    prog.current = 0xFFFF;
  }
  indexPuzzles();
  if (prog.current < count) sel = prog.current;
}

X4_EXPORT(exit) void exit_(void) {
  if (dirty) x4_save(&prog, sizeof(prog));
}

static int listEvent(int ev) {
  if (count == 0) return 0;
  if (ev == X4_CONFIRM) {
    openPuzzle(sel);
    return 1;
  }
  if (ev == X4_UP || ev == X4_LEFT) sel = (sel + count - 1) % count;
  else if (ev == X4_DOWN || ev == X4_RIGHT) sel = (sel + 1) % count;
  else return 0;
  return 1;
}

X4_EXPORT(event) int event(int ev) {
  if (!playing) return listEvent(ev);
  if (solvedNow) {
    if (ev != X4_CONFIRM) return 0;
    playing = 0;
    return 1;
  }
  uint8_t* m = &marks[cy * MAX + cx];
  if (ev == X4_CONFIRM) {
    *m = *m == FILLED ? EMPTY : FILLED;
  } else if (ev == X4_CONFIRM_HOLD) {
    *m = *m == CROSSED ? EMPTY : CROSSED;
  } else if (ev >= X4_LEFT && ev <= X4_DOWN) {
    const int dx = ev == X4_LEFT ? -1 : ev == X4_RIGHT ? 1 : 0;
    const int dy = ev == X4_UP ? -1 : ev == X4_DOWN ? 1 : 0;
    cx = (cx + dx + w) % w;
    cy = (cy + dy + h) % h;
    return 1;
  } else {
    return 0;
  }
  dirty = 1;
  solvedNow = checkSolved();
  if (solvedNow) prog.solved[prog.current / 32] |= 1u << (prog.current % 32);
  return 1;
}

static void renderList(void) {
  const int t = th(), wd = x4_width(), side = 24;
  x4_title(t ? "โนโนแกรม" : "Nonogram");
  if (count == 0) {
    x4_textbox(side, (x4_top() + x4_bottom()) / 2 - 40, wd - 2 * side, 4,
               t ? "ยังไม่มีโจทย์ คัดลอก puzzles.tsv ไปที่ /apps/nonogram" : "No puzzles. Copy puzzles.tsv to /apps/nonogram",
               X4_UI12, X4_CENTER);
    x4_hints(0, 0, 0, 0);
    return;
  }
  const int rowH = 52, gap = 6, lh = x4_lineh(X4_UI12), top = x4_top() + 8;
  int visible = (x4_bottom() - top) / (rowH + gap);
  if (visible < 1) visible = 1;
  if (sel < first) first = sel;
  if (sel >= first + visible) first = sel - visible + 1;
  char label[16];
  for (int i = first; i < count && i < first + visible; ++i) {
    const int y = top + (i - first) * (rowH + gap), on = i == sel, done = isSolved(i);
    const int flags = on ? X4_INVERT : 0;
    if (on) x4_rfill(side, y, wd - 2 * side, rowH, 12, X4_BLACK);
    fmtInt(label, "#", i + 1);
    // The name gives the picture away, so it shows only once solved.
    x4_text(side + 16, y + (rowH - lh) / 2, done && names[i][0] ? names[i] : label, X4_UI12, flags);
    if (done) {
      const char* tick = t ? "เสร็จแล้ว" : "Solved";
      x4_text(wd - side - 16 - x4_textw(tick, X4_UI10, 0), y + (rowH - x4_lineh(X4_UI10)) / 2, tick, X4_UI10, flags);
    }
  }
  x4_hints(0, t ? "เล่น" : "Play", "<", ">");
}

static void renderPuzzle(void) {
  const int t = th(), pageW = x4_width(), side = 16;
  char title[80];
  if (solvedNow && names[prog.current][0]) {
    x4_title(names[prog.current]);
  } else {
    fmtInt(title, t ? "โนโนแกรม #" : "Nonogram #", prog.current + 1);
    x4_title(title);
  }
  const int top = x4_top(), bottom = x4_bottom() - 28;

  // Clue bands: row clues to the left, column clues stacked above.
  uint8_t c[MAX_CLUES];
  int maxRow = 1, maxCol = 1;
  for (int y = 0; y < h; ++y) {
    const int n = clues(y, 0, c);
    if (n > maxRow) maxRow = n;
  }
  for (int x = 0; x < w; ++x) {
    const int n = clues(x, 1, c);
    if (n > maxCol) maxCol = n;
  }
  const int numH = x4_lineh(X4_UI10), digitW = x4_textw("00", X4_UI10, 0) + 4;
  const int rowClueW = maxRow * digitW + 6, colClueH = maxCol * numH + 6;
  int cell = (pageW - 2 * side - rowClueW) / w;
  if ((bottom - top - colClueH) / h < cell) cell = (bottom - top - colClueH) / h;
  const int gridW = cell * w, gridH = cell * h;
  const int gx = (pageW - rowClueW - gridW) / 2 + rowClueW, gy = top + colClueH;

  char num[4];
  for (int y = 0; y < h; ++y) {
    int n = clues(y, 0, c);
    if (n == 0) c[n++] = 0;  // an empty line shows "0"
    const int style = y == cy ? X4_BOLD : 0;
    int x = gx - 6;
    for (int i = n - 1; i >= 0; --i) {
      fmtInt(num, "", c[i]);
      x -= digitW;
      x4_text(x + digitW - x4_textw(num, X4_UI10, style), gy + y * cell + (cell - numH) / 2, num, X4_UI10, style);
    }
  }
  for (int x = 0; x < w; ++x) {
    int n = clues(x, 1, c);
    if (n == 0) c[n++] = 0;
    const int style = x == cx ? X4_BOLD : 0;
    for (int i = 0; i < n; ++i) {
      fmtInt(num, "", c[i]);
      x4_text(gx + x * cell + cell / 2, gy - 4 - (n - i) * numH, num, X4_UI10, style | X4_CENTER);
    }
  }

  // Cells: filled black, crosses as an X, the cursor a frame inside its cell.
  // Grid lines every cell, heavier every five.
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      const int px = gx + x * cell, py = gy + y * cell;
      const uint8_t m = marks[y * MAX + x];
      if (m == FILLED) x4_fill(px, py, cell, cell, X4_BLACK);
      if (m == CROSSED) {
        x4_line(px + cell / 4, py + cell / 4, px + cell * 3 / 4, py + cell * 3 / 4, 2, X4_BLACK);
        x4_line(px + cell * 3 / 4, py + cell / 4, px + cell / 4, py + cell * 3 / 4, 2, X4_BLACK);
      }
      if (x == cx && y == cy && !solvedNow) {
        x4_frame(px + 3, py + 3, cell - 6, cell - 6, 3, m == FILLED ? X4_WHITE : X4_BLACK);
      }
    }
  for (int x = 0; x <= w; ++x) x4_fill(gx + x * cell - (x % 5 == 0), gy, x % 5 == 0 ? 3 : 1, gridH, X4_BLACK);
  for (int y = 0; y <= h; ++y) x4_fill(gx, gy + y * cell - (y % 5 == 0), gridW, y % 5 == 0 ? 3 : 1, X4_BLACK);
  if (solvedNow) {
    x4_text(pageW / 2, gy + gridH + 14, t ? "ชนะแล้ว!" : "You win!", X4_UI12, X4_BOLD | X4_CENTER);
    x4_refresh(X4_HALF);
  } else {
    x4_text(pageW / 2, gy + gridH + 14, t ? "กด Confirm ค้างเพื่อกากบาท" : "Hold Confirm to mark a cross", X4_UI10,
            X4_CENTER);
  }
  x4_hints(0, solvedNow ? (t ? "เสร็จ" : "Done") : (t ? "ระบาย" : "Fill"), "<", ">");
}

X4_EXPORT(render) void render(void) {
  if (playing) {
    renderPuzzle();
  } else {
    renderList();
  }
}
