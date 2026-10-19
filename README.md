# x4-pocket-apps

แอปสำหรับ Xteink X4 ที่ใช้เฟิร์มแวร์ [crosspoint-x4](https://github.com/AKKatung159/crosspoint-x4) (Pocket SDK v1)
แต่ละแอปเป็นไฟล์ `.xpa` (zip) — แตกไฟล์ที่ `/apps/` บน SD แล้วเปิดจาก Home → แอป

| แอป | สถานะ |
|---|---|
| 2048 | ✅ |
| Minesweeper, Nonogram, Flashcards, Tarot, Reading stats, Dashboard | กำลังย้ายจากเฟิร์มแวร์ |

## ทำแอปเอง

```sh
pip install ziglang          # หรือติดตั้ง zig
scripts/build.sh apps/2048   # -> dist/2048.xpa
```

แอปคือโฟลเดอร์ `apps/<id>/` ที่มี `manifest.json`, ไฟล์ C และข้อมูลอื่น ๆ API อยู่ใน [sdk/x4.h](sdk/x4.h)
ตัวอย่างที่สั้นที่สุด: [apps/2048](apps/2048)

<details><summary>English</summary>

Apps for the Xteink X4 running the crosspoint-x4 firmware (Pocket SDK v1). Each app ships as a `.xpa`
(a zip): unzip it into `/apps/` on the SD card and open it from Home → Apps.

To build: `pip install ziglang` (or install zig), then `scripts/build.sh [apps/<id>]` writes `dist/<id>.xpa`.
An app is `apps/<id>/` with `manifest.json`, C sources and any data files; the API is [sdk/x4.h](sdk/x4.h).
Tagging a commit publishes every `.xpa` as a GitHub release.

</details>
