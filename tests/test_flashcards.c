#include "stub.h"

#include "../apps/flashcards/main.c"

static void write(const char* path, const char* text) {
  FILE* f = fopen(path, "wb");
  fputs(text, f);
  fclose(f);
}

int main(void) {
  stub_app("flashcards");
  system("rm -rf build/tests/flash && mkdir -p build/tests/flash/sub");
  system("cp apps/flashcards/english-thai.tsv build/tests/flash/");
  write("build/tests/flash/b-deck.TSV",
        "\xEF\xBB\xBF# comment\r\n"
        "\r\n"
        "dog\tสุนัข\tnote\r\n"
        "no tab here\r\n"
        "run\tวิ่ง\\nI run every day.\r\n"
        "word\t\r\n"
        "last\tไม่มีขึ้นบรรทัด");
  write("build/tests/flash/notes.txt", "a\tb\n");
  write("build/tests/flash/.hidden.tsv", "a\tb\n");
  write("build/tests/flash/manifest.json", "{}");
  stub_dir = "build/tests/flash";
  stub_save_len = -1;
  init();
  // Only .tsv files (any case), sorted; folders and dot files are not listed.
  CHECK(deckCount == 2 && strcmp(decks[0], "b-deck.TSV") == 0 && strcmp(decks[1], "english-thai.tsv") == 0);
  RENDER();
  CHECK(strcmp(stub_title, "Flashcards") == 0 && stub_drew("b-deck.TSV"));

  // Cards: comments, blanks and lines without a TAB skipped; BOM, CR and a third column dropped.
  event(X4_CONFIRM);
  CHECK(studying && cards == 4 && cur == 0);
  CHECK(strcmp(front, "dog") == 0 && strcmp(back, "สุนัข") == 0);
  event(X4_DOWN);
  CHECK(strcmp(back, "วิ่ง\nI run every day.") == 0);
  event(X4_DOWN);
  CHECK(strcmp(front, "word") == 0 && strcmp(back, "") == 0);
  event(X4_DOWN);
  CHECK(strcmp(back, "ไม่มีขึ้นบรรทัด") == 0);
  event(X4_DOWN);
  CHECK(cur == 0);  // wraps
  event(X4_UP);
  CHECK(cur == 3);
  // Confirm flips; a move turns it back.
  RENDER();
  CHECK(!stub_drew("ไม่มีขึ้นบรรทัด") && strcmp(stub_hint, "Flip") == 0 && stub_drew("4 / 4"));
  event(X4_CONFIRM);
  RENDER();
  CHECK(stub_drew("ไม่มีขึ้นบรรทัด"));
  // Hold goes back to the list; the position is kept per deck and saved on exit.
  event(X4_CONFIRM_HOLD);
  CHECK(!studying);
  event(X4_DOWN);
  event(X4_CONFIRM);
  CHECK(cards == 15 && cur == 0);
  event(X4_DOWN);
  event(X4_DOWN);
  exit_();
  memset(&prog, 0, sizeof(prog));
  studying = 0;
  init();
  sel = 1;
  event(X4_CONFIRM);
  CHECK(cur == 2);
  event(X4_CONFIRM_HOLD);
  sel = 0;
  event(X4_CONFIRM);
  CHECK(cur == 3);
  // Eight decks are remembered; the ninth reuses the oldest slot.
  for (uint32_t i = 0; i < 9; ++i) {
    deck = 0;
    snprintf(decks[0], NAME_LEN, "deck%u.tsv", (unsigned)i);
    cur = (int)i;
    rememberPosition();
  }
  CHECK(slotOf(nameHash("deck0.tsv")) < 0 && prog.pos[slotOf(nameHash("deck8.tsv"))] == 8);
  // An empty deck and an empty folder.
  system("rm -rf build/tests/flash2 && mkdir -p build/tests/flash2");
  write("build/tests/flash2/empty.tsv", "# nothing\n");
  stub_dir = "build/tests/flash2";
  stub_save_len = -1;
  studying = 0;
  init();
  event(X4_CONFIRM);
  CHECK(studying && cards == 0 && event(X4_DOWN) == 0);
  RENDER();
  CHECK(stub_drew("No cards in this deck"));
  stub_dir = "build/tests/none";
  studying = 0;
  init();
  CHECK(deckCount == 0 && event(X4_CONFIRM) == 0);
  stub_lang = 1;
  RENDER();
  CHECK(stub_drew("ยังไม่มีชุดการ์ด คัดลอกไฟล์ .tsv (หน้า<TAB>หลัง บรรทัดละใบ) ไปที่ /apps/flashcards"));
  return stub_failures != 0;
}
