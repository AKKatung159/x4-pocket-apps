// Dashboard for the Pocket SDK (API v2): shows the last dashboard image the
// X4 fetched (the "Dashboard" sleep screen, Settings -> Display -> Dashboard
// URL). Confirm joins Wi-Fi and fetches a new one.
#include "x4.h"

// The server renders 480x800; the app area is shorter, so it is shown scaled.
#define IMG_W 480
#define IMG_H 800

static int status;  // 0 nothing tried, 1 updated, -1 the last update failed

X4_EXPORT(event) int event(int ev) {
  if (ev != X4_CONFIRM) return 0;
  status = x4_sync() ? 1 : -1;
  return 1;
}

X4_EXPORT(render) void render(void) {
  const int t = x4_lang() == 1, w = x4_width(), top = x4_top(), noteH = x4_lineh(X4_UI10) + 8;
  const int boxH = x4_bottom() - top - noteH;
  int dw = IMG_W * boxH / IMG_H;
  if (dw > w) dw = w;
  x4_title(t ? "แดชบอร์ด" : "Dashboard");
  x4_hints(0, t ? "อัปเดต" : "Update", 0, 0);
  const int shown = x4_dashboard((w - dw) / 2, top, dw, boxH);
  if (!shown) {
    x4_textbox(24, top + boxH / 2 - 40, w - 48, 4,
               t ? "ยังไม่มีแดชบอร์ด ตั้งที่อยู่ใน ตั้งค่า → หน้าจอ → ที่อยู่แดชบอร์ด แล้วกด Confirm"
                 : "No dashboard yet. Set Settings → Display → Dashboard URL, then press Confirm",
               X4_UI12, X4_CENTER);
  }
  if (status < 0) {
    x4_text(w / 2, top + boxH + 4, t ? "อัปเดตไม่สำเร็จ ลองใหม่อีกครั้ง" : "Update failed, try again", X4_UI10, X4_CENTER);
  }
  x4_refresh(X4_HALF);  // a picture leaves more ghosting than text
}
