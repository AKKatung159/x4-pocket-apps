// Tarot for the Pocket SDK. Pick a spread (one card, or past/present/future);
// each card is drawn once, upright or reversed. Confirm switches between the
// card image and its meaning, the arrows move through a three-card spread and
// holding Confirm goes back to the spread list (Back always leaves the app).
//
// deck.tsv: file<TAB>name EN<TAB>name TH<TAB>upright EN<TAB>upright TH<TAB>reversed EN<TAB>reversed TH
// Images: cards/<file>.bmp (scripts/make_tarot_cards.py); without one a plain card back is drawn.
#include <string.h>

#include "x4.h"

#define DECK "deck.tsv"
#define MAX_ROWS 128
#define MAX_LINE 1024
#define MAX_CARDS 3
#define SIDE 24

enum { FILE_, NAME_EN, NAME_TH, UP_EN, UP_TH, REV_EN, REV_TH, COLUMNS };

static uint32_t rowAt[MAX_ROWS];  // byte offset of each card row in deck.tsv
static int rows;

static int reading;  // 0 the spread list, 1 a reading
static int choice;   // spread list cursor: 0 one card, 1 three
static int count, current, showMeaning;
static int drawn[MAX_CARDS], reversed[MAX_CARDS];

static char row[MAX_LINE + 1];
static const char* cols[COLUMNS];

static int th(void) { return x4_lang() == 1; }

static void indexDeck(void) {
  rows = x4_rows(DECK, rowAt, MAX_ROWS);
  if (rows < 0) rows = 0;
}

// Reads card `index` and splits it at TABs; missing columns are "".
static void loadCard(int index) {
  for (int i = 0; i < COLUMNS; ++i) cols[i] = "";
  const int got = x4_read(DECK, (int)rowAt[index], row, MAX_LINE);
  if (got <= 0) {
    x4_log("tarot: bad deck row");
    return;
  }
  row[got] = 0;
  char* end = strchr(row, '\n');
  if (end) *end = 0;
  const size_t n = strlen(row);
  if (n && row[n - 1] == '\r') row[n - 1] = 0;
  char* p = memcmp(row, "\xEF\xBB\xBF", 3) == 0 ? row + 3 : row;
  for (int i = 0; i < COLUMNS; ++i) {
    cols[i] = p;
    char* t = i + 1 < COLUMNS ? strchr(p, '\t') : 0;
    if (!t) break;
    *t = 0;
    p = t + 1;
  }
}

static int contains(const int* a, int n, int v) {
  for (int i = 0; i < n; ++i)
    if (a[i] == v) return 1;
  return 0;
}

static void startReading(int cards) {
  count = cards;
  current = 0;
  showMeaning = 0;
  // Distinct cards; a repeat takes the next card, so this ends even with a stuck RNG.
  for (int i = 0; i < count; ++i) {
    drawn[i] = (int)(x4_rnd() % (uint32_t)rows);
    while (contains(drawn, i, drawn[i])) drawn[i] = (drawn[i] + 1) % rows;
    reversed[i] = (int)(x4_rnd() & 1u);
  }
  loadCard(drawn[0]);
  reading = 1;
}

X4_EXPORT(init) void init(void) { indexDeck(); }

X4_EXPORT(event) int event(int ev) {
  if (!reading) {
    if (ev == X4_UP || ev == X4_DOWN || ev == X4_LEFT || ev == X4_RIGHT) {
      choice ^= 1;
      return 1;
    }
    if (ev != X4_CONFIRM || rows < MAX_CARDS) return 0;
    startReading(choice == 0 ? 1 : MAX_CARDS);
    return 1;
  }
  if (ev == X4_CONFIRM_HOLD) {
    reading = 0;
  } else if (ev == X4_CONFIRM) {
    showMeaning = !showMeaning;
  } else if (count > 1 && ev >= X4_LEFT && ev <= X4_DOWN) {
    const int step = ev == X4_RIGHT || ev == X4_DOWN ? 1 : -1;
    current = (current + count + step) % count;
    showMeaning = 0;
    loadCard(drawn[current]);
  } else {
    return 0;
  }
  return 1;
}

static void renderList(void) {
  const int w = x4_width();
  if (rows < MAX_CARDS) {
    const int lh = x4_lineh(X4_UI12);
    x4_textbox(SIDE, (x4_top() + x4_bottom()) / 2 - 2 * lh, w - 2 * SIDE, 6,
               th() ? "ยังไม่มีชุดไพ่ คัดลอก deck.tsv และโฟลเดอร์ cards ไปที่ /apps/tarot"
                    : "No tarot pack. Copy deck.tsv and the cards folder to /apps/tarot",
               X4_UI12, X4_CENTER);
    x4_hints(0, 0, 0, 0);
    return;
  }
  const char* labels[2] = {th() ? "ไพ่ประจำวัน" : "Card of the day",
                           th() ? "อดีต · ปัจจุบัน · อนาคต" : "Past · Present · Future"};
  const int rowH = 60, lh = x4_lineh(X4_UI12);
  int y = x4_top() + 16;
  for (int i = 0; i < 2; ++i, y += rowH + 8) {
    if (i == choice) x4_rfill(SIDE, y, w - 2 * SIDE, rowH, 12, X4_BLACK);
    x4_text(SIDE + 20, y + (rowH - lh) / 2, labels[i], X4_UI12, i == choice ? X4_INVERT : 0);
  }
  x4_hints(0, th() ? "เลือก" : "Select", "<", ">");
}

// Fits cards/<file>.bmp into the box, centred; the renderer only scales down,
// from the top-left, so the size comes from the BMP header.
static void renderImage(int top, int bottom) {
  const int w = x4_width();
  const int nameH = x4_lineh(X4_SANS16) + 2 * x4_lineh(X4_UI10) + 12;
  const int boxW = w - 2 * SIDE, boxH = bottom - top - nameH;
  char path[80] = "cards/";
  const size_t n = strlen(cols[FILE_]);
  int shown = 0;
  if (n > 0 && n + 11 < sizeof(path)) {
    memcpy(path + 6, cols[FILE_], n);
    memcpy(path + 6 + n, ".bmp", 5);
    uint8_t hdr[26];
    if (x4_read(path, 0, hdr, sizeof(hdr)) == (int)sizeof(hdr) && hdr[0] == 'B' && hdr[1] == 'M') {
      int32_t bw, bh;
      memcpy(&bw, hdr + 18, 4);
      memcpy(&bh, hdr + 22, 4);
      if (bh < 0) bh = -bh;
      int dw = bw;
      if (bw > boxW || bh > boxH) dw = bw * boxH / bh < boxW ? bw * boxH / bh : boxW;
      if (bw > 0 && bh > 0) shown = x4_image(path, (w - dw) / 2, top, boxW, boxH);
    }
  }
  if (!shown) {
    const int cw = boxH * 58 / 100;
    x4_rfill((w - cw) / 2, top, cw, boxH, 16, X4_LIGHT);
  }
  int y = bottom - nameH + 6;
  x4_text(w / 2, y, th() ? cols[NAME_TH] : cols[NAME_EN], X4_SANS16, X4_BOLD | X4_CENTER);
  y += x4_lineh(X4_SANS16);
  const char* side = reversed[current] ? (th() ? "ไพ่กลับหัว" : "Reversed") : (th() ? "ไพ่หัวตั้ง" : "Upright");
  x4_text(w / 2, y, side, X4_UI10, X4_CENTER);
  y += x4_lineh(X4_UI10);
  x4_text(w / 2, y, th() ? "กด Confirm ค้างเพื่อเปิดไพ่ใหม่" : "Hold Confirm for a new reading", X4_UI10, X4_CENTER);
}

static void renderMeaning(int top, int bottom) {
  const int innerW = x4_width() - 2 * SIDE, t = th(), rev = reversed[current];
  int y = top + 8;
  y += x4_lineh(X4_SANS18) * x4_textbox(SIDE, y, innerW, 2, t ? cols[NAME_TH] : cols[NAME_EN], X4_SANS18, X4_BOLD);
  x4_text(SIDE, y + 4, rev ? (t ? "ไพ่กลับหัว" : "Reversed") : (t ? "ไพ่หัวตั้ง" : "Upright"), X4_UI12, 0);
  y += x4_lineh(X4_UI12) + 16;
  x4_rfill(SIDE, y, innerW, 6, 3, X4_LIGHT);
  y += 24;
  // The UI language first, the other one under it, smaller.
  const char* primary = cols[rev ? (t ? REV_TH : REV_EN) : (t ? UP_TH : UP_EN)];
  const char* secondary = cols[rev ? (t ? REV_EN : REV_TH) : (t ? UP_EN : UP_TH)];
  y += x4_lineh(X4_SANS16) * x4_textbox(SIDE, y, innerW, 6, primary, X4_SANS16, 0);
  y += 16;
  const int lh = x4_lineh(X4_UI12), lines = (bottom - y) / lh;
  x4_textbox(SIDE, y, innerW, lines > 1 ? lines : 1, secondary, X4_UI12, 0);
}

X4_EXPORT(render) void render(void) {
  const int t = th();
  if (!reading) {
    x4_title(t ? "ไพ่ทาโรต์" : "Tarot");
    renderList();
    return;
  }
  static const char* const POS_EN[MAX_CARDS] = {"Past", "Present", "Future"};
  static const char* const POS_TH[MAX_CARDS] = {"อดีต", "ปัจจุบัน", "อนาคต"};
  x4_title(count == 1 ? (t ? "ไพ่ประจำวัน" : "Card of the day") : (t ? POS_TH : POS_EN)[current]);
  const int top = x4_top() + 12, bottom = x4_bottom() - 8;
  if (count > 1) {
    const char pos[6] = {(char)('1' + current), ' ', '/', ' ', (char)('0' + count), 0};
    x4_text(x4_width() - SIDE - x4_textw(pos, X4_UI10, 0), x4_top() + 4, pos, X4_UI10, 0);
  }
  if (showMeaning) {
    renderMeaning(top + 16, bottom);
  } else {
    renderImage(top + 16, bottom);
    x4_refresh(X4_HALF);  // a picture leaves more ghosting than text
  }
  x4_hints(0, showMeaning ? (t ? "ดูไพ่" : "Card") : (t ? "ความหมาย" : "Meaning"), count > 1 ? "<" : 0,
           count > 1 ? ">" : 0);
}
