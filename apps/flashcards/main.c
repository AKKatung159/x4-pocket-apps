// Flashcards for the Pocket SDK. Every .tsv file in the app's folder is a deck,
// one "front<TAB>back" card per line ("\n" is a line break, '#' lines are
// notes). In a deck Confirm flips, the arrows move and holding Confirm goes
// back to the deck list. The position in the last eight decks is saved on exit.
#include <string.h>

#include "x4.h"

#define MAX_DECKS 32
#define NAME_LEN 64
#define MAX_CARDS 2000
#define MAX_LINE 1024
#define RECENT 8
#define SAVE_SIZE 68

static struct {
  uint32_t hash[RECENT];  // FNV-1a of the deck's file name
  uint32_t pos[RECENT];
  uint32_t next;  // slot reused by the next new deck
} prog;
_Static_assert(sizeof(prog) == SAVE_SIZE, "manifest.json \"save\" must match");

static char decks[MAX_DECKS][NAME_LEN];
static int deckCount;
static uint32_t cardAt[MAX_CARDS];  // byte offset of each card in the open deck
static int cards;

static int studying, sel, first, deck, cur, flipped, dirty;
static char line[MAX_LINE + 1];
static const char* front = "";
static const char* back = "";

static int th(void) { return x4_lang() == 1; }

static uint32_t nameHash(const char* s) {
  uint32_t h = 2166136261u;
  for (; *s; ++s) h = (h ^ (uint8_t)*s) * 16777619u;
  return h;
}

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

static void listDecks(void) {
  char name[NAME_LEN];
  deckCount = 0;
  for (int n = 0; deckCount < MAX_DECKS; ++n) {
    const int len = x4_list(n, name, sizeof(name));
    if (len < 0) break;
    if (len <= 4 || name[len - 4] != '.' || lower(name[len - 3]) != 't' || lower(name[len - 2]) != 's' ||
        lower(name[len - 1]) != 'v') {
      continue;
    }
    // Insertion sort by name, so the list does not depend on the card's order.
    int i = deckCount++;
    while (i > 0 && strcmp(decks[i - 1], name) > 0) {
      memcpy(decks[i], decks[i - 1], NAME_LEN);
      --i;
    }
    memcpy(decks[i], name, (size_t)len + 1);
  }
}

static void indexDeck(const char* file) {
  cards = x4_rows(file, cardAt, MAX_CARDS);
  if (cards < 0) cards = 0;
}

// "\n" (backslash, n) becomes a line break, in place.
static void unescape(char* s) {
  char* w = s;
  for (const char* r = s; *r; ++r, ++w) {
    if (r[0] == '\\' && r[1] == 'n') {
      *w = '\n';
      ++r;
    } else {
      *w = *r;
    }
  }
  *w = 0;
}

static void loadCard(void) {
  front = back = "";
  const int got = x4_read(decks[deck], (int)cardAt[cur], line, MAX_LINE);
  if (got <= 0) return;
  line[got] = 0;
  char* nl = strchr(line, '\n');
  if (nl) *nl = 0;
  char* tab = strchr(line, '\t');
  if (!tab) return;
  *tab = 0;
  char* end = strchr(tab + 1, '\t');  // a third column is ignored
  if (!end) end = tab + 1 + strlen(tab + 1);
  *end = 0;
  if (end > tab + 1 && end[-1] == '\r') end[-1] = 0;
  unescape(line);
  unescape(tab + 1);
  front = line;
  back = tab + 1;
}

static int slotOf(uint32_t h) {
  for (int i = 0; i < RECENT; ++i)
    if (prog.hash[i] == h) return i;
  return -1;
}

static void openDeck(int d) {
  deck = d;
  indexDeck(decks[d]);
  const int slot = slotOf(nameHash(decks[d]));
  cur = slot >= 0 && prog.pos[slot] < (uint32_t)cards ? (int)prog.pos[slot] : 0;
  flipped = 0;
  if (cards > 0) loadCard();
  studying = 1;
}

static void rememberPosition(void) {
  const uint32_t h = nameHash(decks[deck]);
  int slot = slotOf(h);
  if (slot < 0) {
    slot = (int)(prog.next % RECENT);
    prog.next = (prog.next + 1) % RECENT;
  }
  prog.hash[slot] = h;
  prog.pos[slot] = (uint32_t)cur;
}

X4_EXPORT(init) void init(void) {
  if (x4_load(&prog, sizeof(prog)) != sizeof(prog)) memset(&prog, 0, sizeof(prog));
  listDecks();
}

X4_EXPORT(exit) void exit_(void) {
  if (studying && dirty) rememberPosition();
  if (dirty) x4_save(&prog, sizeof(prog));
}

X4_EXPORT(event) int event(int ev) {
  if (!studying) {
    if (deckCount == 0) return 0;
    if (ev == X4_CONFIRM) openDeck(sel);
    else if (ev == X4_UP || ev == X4_LEFT) sel = (sel + deckCount - 1) % deckCount;
    else if (ev == X4_DOWN || ev == X4_RIGHT) sel = (sel + 1) % deckCount;
    else return 0;
    return 1;
  }
  if (ev == X4_CONFIRM_HOLD) {
    if (dirty) rememberPosition();
    studying = 0;
  } else if (cards == 0) {
    return 0;
  } else if (ev == X4_CONFIRM) {
    flipped = !flipped;
  } else if (ev >= X4_LEFT && ev <= X4_DOWN) {
    const int step = ev == X4_RIGHT || ev == X4_DOWN ? 1 : -1;
    cur = (cur + cards + step) % cards;  // wraps at both ends
    flipped = 0;
    dirty = 1;
    loadCard();
  } else {
    return 0;
  }
  return 1;
}

static void fmtPos(char* out, int a, int b) {
  char d[12];
  int n;
  for (int k = 0; k < 2; ++k) {
    unsigned u = (unsigned)(k ? b : a);
    n = 0;
    do d[n++] = (char)('0' + u % 10); while ((u /= 10) > 0);
    while (n) *out++ = d[--n];
    if (!k) {
      memcpy(out, " / ", 3);
      out += 3;
    }
  }
  *out = 0;
}

static void renderList(void) {
  const int t = th(), wd = x4_width(), side = 24;
  x4_title(t ? "แฟลชการ์ด" : "Flashcards");
  if (deckCount == 0) {
    x4_textbox(side, (x4_top() + x4_bottom()) / 2 - 60, wd - 2 * side, 6,
               t ? "ยังไม่มีชุดการ์ด คัดลอกไฟล์ .tsv (หน้า<TAB>หลัง บรรทัดละใบ) ไปที่ /apps/flashcards"
                 : "No decks. Copy .tsv files (front<TAB>back per line) to /apps/flashcards",
               X4_UI12, X4_CENTER);
    x4_hints(0, 0, 0, 0);
    return;
  }
  const int rowH = 52, gap = 6, lh = x4_lineh(X4_UI12), top = x4_top() + 8;
  int visible = (x4_bottom() - top) / (rowH + gap);
  if (visible < 1) visible = 1;
  if (sel < first) first = sel;
  if (sel >= first + visible) first = sel - visible + 1;
  for (int i = first; i < deckCount && i < first + visible; ++i) {
    const int y = top + (i - first) * (rowH + gap);
    if (i == sel) x4_rfill(side, y, wd - 2 * side, rowH, 12, X4_BLACK);
    x4_textbox(side + 16, y + (rowH - lh) / 2, wd - 2 * side - 32, 1, decks[i], X4_UI12, i == sel ? X4_INVERT : 0);
  }
  x4_hints(0, t ? "เปิด" : "Open", "<", ">");
}

static void renderCard(void) {
  const int t = th(), wd = x4_width(), side = 24, innerW = wd - 2 * side;
  x4_title(decks[deck]);
  const int top = x4_top(), bottom = x4_bottom();
  if (cards == 0) {
    x4_text(wd / 2, (top + bottom) / 2, t ? "ไม่มีการ์ดในชุดนี้" : "No cards in this deck", X4_UI12, X4_CENTER);
    x4_hints(0, 0, 0, 0);
    return;
  }
  char pos[32];
  fmtPos(pos, cur + 1, cards);
  x4_text(wd - side - x4_textw(pos, X4_UI10, 0), top, pos, X4_UI10, 0);

  // Front centred in the upper half, back under a grey band in the lower half.
  const int mid = (top + bottom) / 2, frontH = x4_lineh(X4_SANS18);
  int maxLines = (mid - top - 40) / frontH;
  if (maxLines < 1) maxLines = 1;
  // x4_textbox cannot only measure: a first pass far above the screen (clipped) counts the lines.
  const int lines = x4_textbox(side, -10000, innerW, maxLines, front, X4_SANS18, X4_BOLD | X4_CENTER);
  const int y = top + 30 + (mid - top - 30 - lines * frontH) / 2;
  x4_textbox(side, y, innerW, maxLines, front, X4_SANS18, X4_BOLD | X4_CENTER);
  x4_rfill(side, mid, innerW, 6, 3, X4_LIGHT);
  if (flipped) {
    const int backH = x4_lineh(X4_SANS16), by = mid + 24;
    int backLines = (bottom - by) / backH;
    if (backLines < 1) backLines = 1;
    x4_textbox(side, by, innerW, backLines, back, X4_SANS16, X4_CENTER);
  }
  x4_hints(0, t ? "พลิก" : "Flip", "<", ">");
}

X4_EXPORT(render) void render(void) {
  if (studying) {
    renderCard();
  } else {
    renderList();
  }
}
