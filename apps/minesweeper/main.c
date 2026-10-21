// Minesweeper for the Pocket SDK: arrows move the cursor, Confirm opens a cell,
// holding Confirm flags it. Mines are laid on the first reveal, away from that
// cell and its neighbours, so the first move never loses. Saved on exit.
#include <string.h>

#include "x4.h"

#define W 9
#define H 9
#define MINES 10
#define SAVE_SIZE 84

enum { MINE = 0x10, OPEN = 0x20, FLAG = 0x40, COUNT = 0x0F };
enum { FRESH, PLAYING, WON, LOST };

static struct {
  uint8_t cells[W * H];
  uint8_t state;
  uint8_t cx, cy;
} g;
_Static_assert(sizeof(g) == SAVE_SIZE, "manifest.json \"save\" must match");

static int dirty;

static int inside(int x, int y) { return x >= 0 && x < W && y >= 0 && y < H; }
static int over(void) { return g.state == WON || g.state == LOST; }

static void reset(void) {
  memset(g.cells, 0, sizeof(g.cells));
  g.state = FRESH;
}

static void layMines(int sx, int sy) {
  // A random start, then the next free cell: always terminates, even with a
  // stuck or weak RNG.
  for (int placed = 0; placed < MINES; ++placed) {
    int i = (int)(x4_rnd() % (W * H));
    for (;; i = (i + 1) % (W * H)) {
      const int x = i % W, y = i / W;
      const int nearStart = x >= sx - 1 && x <= sx + 1 && y >= sy - 1 && y <= sy + 1;
      if (!nearStart && !(g.cells[i] & MINE)) break;
    }
    g.cells[i] |= MINE;
  }
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      int n = 0;
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) n += inside(x + dx, y + dy) && (g.cells[(y + dy) * W + x + dx] & MINE);
      g.cells[y * W + x] |= (uint8_t)n;
    }
}

static void reveal(int x, int y) {
  if (over()) return;
  if (g.state == FRESH) {
    layMines(x, y);
    g.state = PLAYING;
  }
  uint8_t* c = &g.cells[y * W + x];
  if (*c & (OPEN | FLAG)) return;
  if (*c & MINE) {
    *c |= OPEN;
    g.state = LOST;
    return;
  }
  // Open the cell and flood through empty (count 0) neighbours. Each cell is
  // pushed at most once (marked OPEN when pushed), so W * H slots suffice.
  uint8_t stack[W * H];
  int top = 0;
  *c |= OPEN;
  stack[top++] = (uint8_t)(y * W + x);
  while (top > 0) {
    const int i = stack[--top];
    if (g.cells[i] & COUNT) continue;
    for (int dy = -1; dy <= 1; ++dy)
      for (int dx = -1; dx <= 1; ++dx) {
        const int nx = i % W + dx, ny = i / W + dy;
        if (!inside(nx, ny) || (g.cells[ny * W + nx] & (OPEN | FLAG | MINE))) continue;
        g.cells[ny * W + nx] |= OPEN;
        stack[top++] = (uint8_t)(ny * W + nx);
      }
  }
  int closedSafe = 0;
  for (int i = 0; i < W * H; ++i) closedSafe += !(g.cells[i] & (OPEN | MINE));
  if (closedSafe == 0) g.state = WON;
}

static int flags(void) {
  int n = 0;
  for (int i = 0; i < W * H; ++i) n += (g.cells[i] & FLAG) != 0;
  return n;
}

static void fmt(char* out, const char* label, int v) {
  char d[12];
  int n = 0;
  const int neg = v < 0;
  unsigned u = neg ? (unsigned)-v : (unsigned)v;
  do d[n++] = (char)('0' + u % 10); while ((u /= 10) > 0);
  while (*label) *out++ = *label++;
  if (neg) *out++ = '-';
  while (n) *out++ = d[--n];
  *out = 0;
}

X4_EXPORT(init) void init(void) {
  if (x4_load(&g, sizeof(g)) != sizeof(g) || g.state > LOST || !inside(g.cx, g.cy)) {
    reset();
    g.cx = W / 2;
    g.cy = H / 2;
  }
}

X4_EXPORT(exit) void exit_(void) {
  if (dirty) x4_save(&g, sizeof(g));
}

X4_EXPORT(event) int event(int ev) {
  if (ev == X4_CONFIRM && over()) {
    reset();
  } else if (ev == X4_CONFIRM) {
    reveal(g.cx, g.cy);
  } else if (ev == X4_CONFIRM_HOLD) {
    uint8_t* c = &g.cells[g.cy * W + g.cx];
    if (over() || (*c & OPEN)) return 0;
    *c ^= FLAG;
  } else if (ev >= X4_LEFT && ev <= X4_DOWN) {
    const int dx = ev == X4_LEFT ? -1 : ev == X4_RIGHT ? 1 : 0;
    const int dy = ev == X4_UP ? -1 : ev == X4_DOWN ? 1 : 0;
    g.cx = (uint8_t)((g.cx + dx + W) % W);
    g.cy = (uint8_t)((g.cy + dy + H) % H);
  } else {
    return 0;
  }
  dirty = 1;
  return 1;
}

// A flag: a pennant filled row by row (the SDK has no polygon fill) on a pole.
static void drawFlag(int tx, int ty, int cell) {
  const int x0 = tx + cell / 3, top = ty + cell / 5, h = cell * 2 / 5, len = cell * 3 / 4 - cell / 3;
  for (int r = 0; r <= h; ++r) {
    const int d = r <= h / 2 ? r : h - r;
    const int w = len * d * 2 / (h ? h : 1);
    if (w > 0) x4_fill(x0, top + r, w, 1, X4_BLACK);
  }
  x4_fill(x0 - 2, top, 3, cell * 3 / 5, X4_BLACK);
}

X4_EXPORT(render) void render(void) {
  const int th = x4_lang() == 1;
  const int w = x4_width(), side = 24;
  int y = x4_top() + 16;
  char line[48];
  fmt(line, th ? "ระเบิด " : "Mines ", MINES - flags());
  x4_text(side, y, line, X4_UI12, X4_BOLD);
  const char* status = g.state == WON ? (th ? "ชนะแล้ว!" : "You win!") : g.state == LOST ? (th ? "ตูม!" : "Boom!") : "";
  x4_text(w - side - x4_textw(status, X4_UI12, X4_BOLD), y, status, X4_UI12, X4_BOLD);
  y += x4_lineh(X4_UI12) + 16;

  // Closed cells grey, open cells white with their count, the cursor a thick
  // black frame inside its cell. A loss shows every mine as a black disc.
  const int gap = 3;
  int board = x4_bottom() - y - 40;
  if (board > w - 2 * side) board = w - 2 * side;
  const int cell = (board - (W + 1) * gap) / W, x0 = (w - board) / 2;
  const int numH = x4_lineh(X4_UI12);
  x4_fill(x0, y, board, board, X4_BLACK);
  for (int r = 0; r < H; ++r)
    for (int c = 0; c < W; ++c) {
      const int tx = x0 + gap + c * (cell + gap), ty = y + gap + r * (cell + gap);
      const uint8_t v = g.cells[r * W + c];
      const int shown = (v & OPEN) || (g.state == LOST && (v & MINE));
      x4_fill(tx, ty, cell, cell, X4_WHITE);
      if (!shown) x4_fill(tx + 2, ty + 2, cell - 4, cell - 4, X4_LIGHT);
      if (shown && (v & MINE)) {
        const int d = cell / 2;
        x4_rfill(tx + (cell - d) / 2, ty + (cell - d) / 2, d, d, d / 2, X4_BLACK);
      } else if (shown && (v & COUNT)) {
        const char n[2] = {(char)('0' + (v & COUNT)), 0};
        x4_text(tx + cell / 2, ty + (cell - numH) / 2, n, X4_UI12, X4_BOLD | X4_CENTER);
      } else if (!shown && (v & FLAG)) {
        drawFlag(tx, ty, cell);
      }
      if (c == g.cx && r == g.cy) x4_frame(tx + 1, ty + 1, cell - 2, cell - 2, 4, X4_BLACK);
    }
  y += board + 16;
  if (!over()) x4_text(w / 2, y, th ? "กด Confirm ค้างเพื่อปักธง" : "Hold Confirm to place a flag", X4_UI10, X4_CENTER);

  x4_hints(0, over() ? (th ? "เกมใหม่" : "New game") : (th ? "เปิด" : "Open"), "<", ">");
  if (over()) x4_refresh(X4_HALF);
}
