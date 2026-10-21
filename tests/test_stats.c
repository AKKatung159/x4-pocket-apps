#include "stub.h"

#include "../apps/stats/main.c"

int main(void) {
  stub_app("stats");
  // Nothing read yet.
  stub_have_stats = 0;
  init();
  RENDER();
  CHECK(stub_drew("No reading recorded yet") && strcmp(stub_title, "Reading stats") == 0);
  // A week of reading, today being Wednesday 21 October 2026.
  stub_have_stats = 1;
  memset(&stub_stats, 0, sizeof(stub_stats));
  stub_stats.total_sec = 7 * 3600 + 5;
  stub_stats.streak = 3;
  stub_stats.finished = 2;
  stub_stats.clock_known = 1;
  for (int i = 0; i < 7; ++i) snprintf(stub_stats.week[i].key, sizeof(stub_stats.week[i].key), "2026-10-%02d", 15 + i);
  stub_stats.week[6].sec = 85 * 60;
  stub_stats.week[6].pages = 42;
  stub_stats.week[5].sec = 20;  // under a minute still gets a bar
  stub_stats.last_sec = 25 * 60;
  stub_stats.last_percent = 37;
  snprintf(stub_stats.last_title, sizeof(stub_stats.last_title), "%s", "Alice ในแดนมหัศจรรย์");
  CHECK(event(X4_CONFIRM) == 1 && have);
  RENDER();
  CHECK(stub_drew("1 h 25 min") && stub_drew("42 pages") && stub_drew("3") && stub_drew("2") && stub_drew("7"));
  CHECK(stub_drew("We") && stub_drew("Th") && stub_drew("85") && stub_drew("0"));
  CHECK(stub_drew("Alice ในแดนมหัศจรรย์") && stub_drew("25 min · 37%"));
  stub_lang = 1;
  RENDER();
  CHECK(stub_drew("1 ชม. 25 นาที") && stub_drew("พ") && stub_drew("พฤ") && stub_drew("วันต่อเนื่อง"));
  // Without the clock: no daily numbers.
  stub_stats.clock_known = 0;
  stub_stats.last_title[0] = 0;
  stub_lang = 0;
  event(X4_CONFIRM);
  RENDER();
  CHECK(stub_drew("–") && stub_drew("Join Wi-Fi once to set the clock for daily stats") && !stub_drew("Last book"));
  CHECK(event(X4_LEFT) == 0);
  CHECK(strcmp(weekday("2026-01-01"), "Th") == 0 && strcmp(weekday("bad"), "") == 0);
  return stub_failures != 0;
}
