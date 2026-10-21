#include "stub.h"

#include "../apps/tarot/main.c"

// Writes a 1-bit BMP header of w x h (the pixels are never read here).
static void bmp(const char* path, int w, int h) {
  unsigned char hdr[62] = {'B', 'M'};
  memcpy(hdr + 10, &(uint32_t){62}, 4);
  memcpy(hdr + 14, &(uint32_t){40}, 4);
  memcpy(hdr + 18, &w, 4);
  memcpy(hdr + 22, &h, 4);
  FILE* f = fopen(path, "wb");
  fwrite(hdr, 1, sizeof(hdr), f);
  fclose(f);
}

int main(void) {
  stub_app("tarot");
  // The shipped deck: 78 cards, every column filled.
  stub_dir = "apps/tarot";
  init();
  CHECK(rows == 78);
  for (int i = 0; i < rows; ++i) {
    loadCard(i);
    for (int c = 0; c < COLUMNS; ++c) CHECK(cols[c][0] != 0);
  }
  loadCard(0);
  CHECK(strcmp(cols[FILE_], "00_Fool") == 0);

  // A copy with a BOM and CRLF line ends reads the same.
  system("mkdir -p build/tests/tarot/cards");
  FILE* in = fopen("apps/tarot/deck.tsv", "rb");
  FILE* out = fopen("build/tests/tarot/deck.tsv", "wb");
  fputs("\xEF\xBB\xBF", out);
  for (int c; (c = fgetc(in)) != EOF;) {
    if (c == '\n') fputc('\r', out);
    fputc(c, out);
  }
  fclose(in);
  fclose(out);
  bmp("build/tests/tarot/cards/00_Fool.bmp", 300, 520);
  stub_dir = "build/tests/tarot";
  init();
  CHECK(rows == 78);
  loadCard(77);
  CHECK(!strchr(cols[REV_TH], '\r'));

  // Spreads: distinct cards, arrows wrap, Confirm flips, hold goes back.
  for (uint32_t seed = 1; seed <= 2000; ++seed) {
    stub_seed = seed;
    reading = 0;
    choice = 0;
    event(X4_DOWN);
    CHECK(event(X4_CONFIRM) && reading && count == 3);
    CHECK(drawn[0] != drawn[1] && drawn[1] != drawn[2] && drawn[0] != drawn[2]);
    stub_lang = (int)(seed & 1);
    RENDER();
    event(X4_RIGHT);
    CHECK(current == 1);
    event(X4_LEFT);
    event(X4_LEFT);
    CHECK(current == 2);
    event(X4_CONFIRM);
    CHECK(showMeaning);
    RENDER();
    event(X4_RIGHT);
    CHECK(!showMeaning && current == 0);
    event(X4_CONFIRM_HOLD);
    CHECK(!reading);
    event(X4_UP);
    event(X4_CONFIRM);
    CHECK(count == 1 && event(X4_RIGHT) == 0);
  }
  // A 300 px card is centred in the 432 px box; a missing image draws the card back.
  stub_lang = 0;
  drawn[0] = 0;
  loadCard(0);
  showMeaning = 0;
  RENDER();
  CHECK(stub_images == 1 && stub_image_x == 90 && stub_drew("The Fool") && stub_refresh == X4_HALF);
  CHECK(strcmp(stub_title, "Card of the day") == 0 && strcmp(stub_hint, "Meaning") == 0);
  drawn[0] = 1;
  loadCard(1);
  RENDER();
  CHECK(stub_images == 0 && stub_drew("The Magician"));
  // No deck: a message and no reading.
  stub_dir = "build/tests/none";
  reading = 0;
  init();
  CHECK(rows == 0 && event(X4_CONFIRM) == 0);
  RENDER();
  CHECK(stub_drew("No tarot pack. Copy deck.tsv and the cards folder to /apps/tarot"));
  return stub_failures != 0;
}
