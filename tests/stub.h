// Host-side stand-in for the firmware's x4_* imports, so an app's main.c can be
// compiled and driven by a test (see run.sh). Files come from stub_dir.
#pragma once
#define X4_IMPORT(name)
#define X4_EXPORT(name)
#include "../sdk/x4.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char* stub_dir;  // the app's folder
extern int stub_lang;         // what x4_lang() returns
extern uint32_t stub_seed;    // x4_rnd() state
// Reads apps/<id>/manifest.json for the "save" size; x4_save and x4_load then
// follow the firmware's rules (len <= save, cap >= save). Call first.
void stub_app(const char* id);
// x4_save / x4_load
extern unsigned char stub_save[1024];
extern int stub_save_len;  // -1: nothing saved
extern int stub_saves;     // x4_save calls
// x4_stats / x4_dashboard / x4_sync
extern x4_stats_t stub_stats;
extern int stub_have_stats, stub_have_dashboard, stub_sync_ok, stub_syncs;
// What the last render did.
extern char stub_title[64], stub_hint[64];
extern int stub_images, stub_image_x, stub_image_flags, stub_refresh;
void stub_begin_render(void);
int stub_drew(const char* text);  // whether a text call in the last render had exactly this string

extern int stub_failures;
#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);            \
      ++stub_failures;                                                    \
    }                                                                     \
  } while (0)
// Calls the app's render() with the per-render state cleared.
#define RENDER()           \
  do {                     \
    stub_begin_render();   \
    render();              \
  } while (0)
