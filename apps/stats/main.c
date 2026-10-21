// Reading stats for the Pocket SDK (API v2): today's reading time, streak,
// books finished and hours in total, the last seven days as bars, and the last
// book. The numbers come from the firmware (x4_stats); Confirm reloads them.
#include <string.h>

#include "x4.h"

#define SIDE 24
#define CARD_H 108
#define CHART_H 150

static x4_stats_t s;
static int have;

static int th(void) { return x4_lang() == 1; }

// Appends the decimal of v to out; returns the new end.
static char* putInt(char* out, unsigned v) {
  char d[12];
  int n = 0;
  do d[n++] = (char)('0' + v % 10); while ((v /= 10) > 0);
  while (n) *out++ = d[--n];
  *out = 0;
  return out;
}

static char* putStr(char* out, const char* s) {
  while (*s) *out++ = *s++;
  *out = 0;
  return out;
}

// "1 ชม. 25 นาที" / "25 min"
static void duration(char* out, uint32_t sec) {
  const int t = th();
  const unsigned min = sec / 60;
  if (min >= 60) {
    out = putInt(out, min / 60);
    out = putStr(out, t ? " ชม. " : " h ");
  }
  out = putInt(out, min % 60);
  putStr(out, t ? " นาที" : " min");
}

// Short weekday from "YYYY-MM-DD" (Sakamoto).
static const char* weekday(const char* key) {
  static const char* const TH[] = {"อา", "จ", "อ", "พ", "พฤ", "ศ", "ส"};
  static const char* const EN[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
  static const int T[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (strlen(key) != 10 || key[4] != '-' || key[7] != '-') return "";
  int y = 0, m = 0, d = 0;
  for (int i = 0; i < 4; ++i) y = y * 10 + key[i] - '0';
  m = (key[5] - '0') * 10 + key[6] - '0';
  d = (key[8] - '0') * 10 + key[9] - '0';
  if (m < 1 || m > 12) return "";
  if (m < 3) y -= 1;
  const int w = (y + y / 4 - y / 100 + y / 400 + T[m - 1] + d) % 7;
  return th() ? TH[w] : EN[w];
}

// A grey card with a big value and a small caption under it.
static void card(int x, int y, int w, const char* value, const char* caption) {
  x4_rfill(x, y, w, CARD_H, 14, X4_LIGHT);
  x4_textbox(x + 6, y + 10, w - 12, 1, value, X4_SANS18, X4_BOLD | X4_CENTER);
  x4_textbox(x + 6, y + 72, w - 12, 1, caption, X4_UI10, X4_CENTER);
}

static void load(void) { have = x4_stats(&s) == (int)sizeof(s); }

X4_EXPORT(init) void init(void) { load(); }

X4_EXPORT(event) int event(int ev) {
  if (ev != X4_CONFIRM) return 0;
  load();
  return 1;
}

X4_EXPORT(render) void render(void) {
  const int t = th(), pageW = x4_width(), innerW = pageW - 2 * SIDE;
  int y = x4_top() + 4;
  char buf[64];
  x4_title(t ? "สถิติการอ่าน" : "Reading stats");
  x4_hints(0, t ? "โหลดใหม่" : "Reload", 0, 0);
  if (!have) {
    x4_text(pageW / 2, y + 200, t ? "ยังไม่มีสถิติการอ่าน" : "No reading recorded yet", X4_UI12, X4_CENTER);
    return;
  }

  // Today: the one big number.
  x4_text(SIDE, y, t ? "วันนี้" : "Today", X4_UI12, 0);
  y += 30;
  if (s.clock_known) {
    duration(buf, s.week[6].sec);
  } else {
    putStr(buf, "–");
  }
  x4_text(SIDE, y, buf, X4_SANS18, X4_BOLD);
  if (s.clock_known) {
    putStr(putInt(buf, s.week[6].pages), t ? " หน้า" : " pages");
    x4_text(pageW - SIDE - x4_textw(buf, X4_UI12, 0), y + 8, buf, X4_UI12, 0);
  }
  y += 52;

  // Three cards: streak, finished, all time.
  const int gap = 12, cw = (innerW - 2 * gap) / 3;
  char streak[12], finished[12], total[12];
  putInt(streak, s.streak);
  putInt(finished, s.finished);
  putInt(total, s.total_sec / 3600);
  card(SIDE, y, cw, streak, t ? "วันต่อเนื่อง" : "day streak");
  card(SIDE + cw + gap, y, cw, finished, t ? "เล่มที่อ่านจบ" : "finished");
  card(SIDE + 2 * (cw + gap), y, cw, total, t ? "ชม. ทั้งหมด" : "hours total");
  y += CARD_H + 24;

  // Last 7 days: bars, today solid black.
  x4_text(SIDE, y, t ? "7 วันที่ผ่านมา" : "Last 7 days", X4_UI12, 0);
  y += 34;
  if (s.clock_known) {
    uint32_t maxSec = 60;
    for (int i = 0; i < 7; ++i)
      if (s.week[i].sec > maxSec) maxSec = s.week[i].sec;
    const int slot = innerW / 7, barW = slot - 18;
    for (int i = 0; i < 7; ++i) {
      const uint32_t sec = s.week[i].sec;
      const int x = SIDE + i * slot + (slot - barW) / 2, today = i == 6;
      int h = sec == 0 ? 0 : (int)((uint64_t)sec * CHART_H / maxSec);
      if (sec && h < 6) h = 6;
      x4_fill(x, y, barW, CHART_H, X4_LIGHT);
      if (h > 0) x4_rfill(x, y + CHART_H - h, barW, h, 6, today ? X4_BLACK : X4_DARK);
      if (sec > 0) {
        putInt(buf, sec / 60);
        x4_text(x + barW / 2, y + CHART_H - h - 18, buf, X4_SMALL, X4_CENTER);
      }
      x4_text(x + barW / 2, y + CHART_H + 6, weekday(s.week[i].key), X4_UI10, X4_CENTER | (today ? X4_BOLD : 0));
    }
  } else {
    x4_rfill(SIDE, y, innerW, CHART_H, 14, X4_LIGHT);
    x4_textbox(SIDE + 10, y + CHART_H / 2 - x4_lineh(X4_UI10), innerW - 20, 2,
               t ? "ต่อ Wi-Fi สักครั้งเพื่อตั้งนาฬิกา แล้วจะเห็นสถิติรายวัน"
                 : "Join Wi-Fi once to set the clock for daily stats",
               X4_UI10, X4_CENTER);
  }
  y += CHART_H + 44;

  // Last book: title, time in it, progress.
  if (s.last_title[0]) {
    x4_text(SIDE, y, t ? "เล่มล่าสุด" : "Last book", X4_UI12, 0);
    y += 32;
    x4_textbox(SIDE, y, innerW, 1, s.last_title, X4_SANS16, X4_BOLD);
    y += 40;
    duration(buf, s.last_sec);
    if (s.last_percent >= 0) putStr(putInt(putStr(buf + strlen(buf), " · "), (unsigned)s.last_percent), "%");
    x4_text(SIDE, y, buf, X4_UI10, 0);
    y += 30;
    x4_fill(SIDE, y, innerW, 10, X4_LIGHT);
    const int pct = s.last_percent > 100 ? 100 : s.last_percent;
    if (pct > 0) x4_fill(SIDE, y, innerW * pct / 100, 10, X4_BLACK);
  }
}
