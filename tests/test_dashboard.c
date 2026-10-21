#include "stub.h"

#include "../apps/dashboard/main.c"

int main(void) {
  stub_app("dashboard");
  stub_have_dashboard = 0;
  RENDER();
  CHECK(stub_images == 0 && strcmp(stub_title, "Dashboard") == 0 && strcmp(stub_hint, "Update") == 0);
  CHECK(stub_drew("No dashboard yet. Set Settings → Display → Dashboard URL, then press Confirm"));
  CHECK(event(X4_LEFT) == 0 && stub_syncs == 0);
  // A failed update says so and keeps the old screen.
  stub_sync_ok = 0;
  CHECK(event(X4_CONFIRM) == 1 && stub_syncs == 1);
  RENDER();
  CHECK(stub_drew("Update failed, try again"));
  // A good one shows the image, centred: 480x800 scaled into the app area.
  stub_sync_ok = 1;
  event(X4_CONFIRM);
  RENDER();
  const int boxH = x4_bottom() - x4_top() - (x4_lineh(X4_UI10) + 8);
  CHECK(stub_images == 1 && stub_image_x == (480 - 480 * boxH / 800) / 2 && !stub_drew("Update failed, try again"));
  CHECK(stub_refresh == X4_HALF);
  stub_lang = 1;
  RENDER();
  CHECK(strcmp(stub_title, "แดชบอร์ด") == 0);
  return stub_failures != 0;
}
