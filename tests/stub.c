#include "stub.h"

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

const char* stub_dir = ".";
int stub_lang = 0;
uint32_t stub_seed = 1;
unsigned char stub_save[1024];
int stub_save_len = -1, stub_saves = 0;
x4_stats_t stub_stats;
int stub_have_stats = 0, stub_have_dashboard = 0, stub_sync_ok = 0, stub_syncs = 0;
char stub_title[64], stub_hint[64];
int stub_images = 0, stub_image_x = -1, stub_refresh = 0;
int stub_failures = 0;
static int saveBytes = 0;  // the manifest's "save"

void stub_app(const char* id) {
  char path[256], text[1024];
  snprintf(path, sizeof(path), "apps/%s/manifest.json", id);
  FILE* f = fopen(path, "rb");
  const size_t n = f ? fread(text, 1, sizeof(text) - 1, f) : 0;
  if (f) fclose(f);
  text[n] = 0;
  const char* key = strstr(text, "\"save\"");
  saveBytes = key ? atoi(strchr(key, ':') + 1) : 0;
}

#define MAX_TEXTS 256
static char texts[MAX_TEXTS][512];
static int textCount;

static void note(const char* s) {
  if (!s) {
    printf("  FAIL: null string drawn\n");
    ++stub_failures;
    return;
  }
  if (textCount < MAX_TEXTS) snprintf(texts[textCount++], sizeof(texts[0]), "%s", s);
}

void stub_begin_render(void) {
  textCount = 0;
  stub_images = 0;
  stub_image_x = -1;
  stub_refresh = X4_FAST;
  stub_title[0] = stub_hint[0] = 0;
}

int stub_drew(const char* text) {
  for (int i = 0; i < textCount; ++i)
    if (strcmp(texts[i], text) == 0) return 1;
  return 0;
}

// Same rule as the firmware's app_path::valid.
static int validPath(const char* rel) {
  size_t len = 0, seg = 0;
  int dotsOnly = 1;
  for (const char* p = rel;; ++p, ++len) {
    const char c = *p;
    if (c == '/' || c == 0) {
      if (seg == 0 || (dotsOnly && seg <= 2)) return 0;
      if (c == 0) return len < 64;
      seg = 0;
      dotsOnly = 1;
      continue;
    }
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' ||
          c == '-') ||
        len >= 63)
      return 0;
    dotsOnly &= c == '.';
    ++seg;
  }
}

static FILE* openApp(const char* rel) {
  char path[512];
  if (!validPath(rel)) return NULL;
  snprintf(path, sizeof(path), "%s/%s", stub_dir, rel);
  return fopen(path, "rb");
}

void x4_fill(int x, int y, int w, int h, int c) {}
void x4_rfill(int x, int y, int w, int h, int r, int c) {}
void x4_frame(int x, int y, int w, int h, int t, int c) {}
void x4_line(int a, int b, int c, int d, int w, int col) {}
void x4_text(int x, int y, const char* s, int f, int fl) { note(s); }
int x4_textbox(int x, int y, int w, int maxLines, const char* s, int f, int fl) {
  note(s);
  if (!s || !*s || maxLines <= 0) return 0;
  int lines = 1;
  for (const char* p = s; *p; ++p) lines += *p == '\n';
  return lines < maxLines ? lines : maxLines;
}
int x4_image(const char* path, int x, int y, int mw, int mh) {
  FILE* f = openApp(path);
  if (!f) return 0;
  fclose(f);
  ++stub_images;
  stub_image_x = x;
  return 1;
}
void x4_refresh(int mode) { stub_refresh = mode; }
int x4_textw(const char* s, int f, int fl) { return (int)strlen(s) * 10; }
int x4_lineh(int f) { return 20 + 4 * f; }
int x4_width(void) { return 480; }
int x4_top(void) { return 80; }
int x4_bottom(void) { return 740; }
void x4_title(const char* s) { snprintf(stub_title, sizeof(stub_title), "%s", s ? s : ""); }
void x4_hints(const char* a, const char* b, const char* c, const char* d) {
  snprintf(stub_hint, sizeof(stub_hint), "%s", b ? b : "");
}
int x4_save(const void* d, int len) {
  if (len < 0 || len > saveBytes) return 0;
  memcpy(stub_save, d, (size_t)len);
  stub_save_len = len;
  ++stub_saves;
  return 1;
}
int x4_load(void* d, int cap) {
  if (saveBytes == 0 || cap < saveBytes || stub_save_len < 0) return -1;
  memcpy(d, stub_save, (size_t)stub_save_len);
  return stub_save_len;
}
int x4_fsize(const char* path) {
  FILE* f = openApp(path);
  if (!f) return -1;
  fseek(f, 0, SEEK_END);
  const int n = (int)ftell(f);
  fclose(f);
  return n;
}
int x4_read(const char* path, int off, void* out, int len) {
  FILE* f = openApp(path);
  if (!f || off < 0 || len < 0) {
    if (f) fclose(f);
    return -1;
  }
  fseek(f, off, SEEK_SET);
  const int n = (int)fread(out, 1, (size_t)len, f);
  fclose(f);
  return n;
}
uint32_t x4_rnd(void) { return stub_seed = stub_seed * 1664525u + 1013904223u; }
uint32_t x4_millis(void) { return 0; }
int x4_lang(void) { return stub_lang; }
void x4_log(const char* s) { printf("  log: %s\n", s); }

int x4_list(int n, char* out, int cap) {
  DIR* d = opendir(stub_dir);
  if (!d || n < 0) {
    if (d) closedir(d);
    return -1;
  }
  int i = 0;
  struct dirent* e;
  while ((e = readdir(d))) {
    char path[512];
    struct stat st;
    snprintf(path, sizeof(path), "%s/%s", stub_dir, e->d_name);
    if (e->d_name[0] == '.' || stat(path, &st) || !S_ISREG(st.st_mode) || !validPath(e->d_name)) continue;
    if (i++ < n) continue;
    const int len = (int)strlen(e->d_name);
    closedir(d);
    if (len >= cap) return -1;
    memcpy(out, e->d_name, (size_t)len + 1);
    return len;
  }
  closedir(d);
  return -1;
}
int x4_stats(x4_stats_t* out) {
  if (!stub_have_stats) return -1;
  *out = stub_stats;
  return (int)sizeof(*out);
}
int x4_dashboard(int x, int y, int mw, int mh) {
  if (!stub_have_dashboard) return 0;
  ++stub_images;
  stub_image_x = x;
  return 1;
}
int x4_sync(void) {
  ++stub_syncs;
  if (stub_sync_ok) stub_have_dashboard = 1;
  return stub_sync_ok;
}
int x4_rows(const char* path, uint32_t* out, int max) {
  FILE* f = openApp(path);
  if (!f || max < 0) {
    if (f) fclose(f);
    return -1;
  }
  int rows = 0, c;
  uint32_t offset = 0, start = 0;
  unsigned char head[4];
  size_t len = 0;
  int tab = 0;
  for (;;) {
    c = fgetc(f);
    const int eof = c == EOF;
    if (!eof && c != '\n') {
      if (len < sizeof(head)) head[len] = (unsigned char)c;
      tab |= c == '\t';
      ++len;
      ++offset;
      continue;
    }
    const size_t skip = len >= 3 && memcmp(head, "\xEF\xBB\xBF", 3) == 0 ? 3 : 0;
    if (tab && len > skip && head[skip] != '#') {
      if (rows == max) break;
      out[rows++] = start + (uint32_t)skip;
    }
    if (eof) break;
    ++offset;
    start = offset;
    len = 0;
    tab = 0;
  }
  fclose(f);
  return rows;
}
