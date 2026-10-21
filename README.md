# x4-pocket-apps

แอปสำหรับ Xteink X4 ที่ใช้เฟิร์มแวร์ [crosspoint-x4](https://github.com/AKKatung159/crosspoint-x4) (Pocket SDK)
แต่ละแอปเป็นไฟล์ `.xpa` (zip) — แตกไฟล์ที่ `/apps/` บน SD แล้วเปิดจาก Home → แอป
แอปที่ id ตรงกับแอปในเฟิร์มแวร์จะแสดงแทนตัวในเฟิร์มแวร์

| แอป | SDK | ปุ่ม |
|---|---|---|
| 2048 | v1 | ลูกศรทั้งสี่ทิศ · Confirm เกมใหม่เมื่อเดินไม่ได้ |
| Minesweeper (เกมกู้ระเบิด) | v1 | Confirm เปิด · กดค้าง ปักธง |
| Nonogram (โนโนแกรม) | v2 | Confirm ระบาย · กดค้าง กากบาท · Confirm บนโจทย์ที่เสร็จแล้ว กลับรายการ |
| Tarot (ไพ่ทาโรต์) | v2 | Confirm รูป/ความหมาย · ลูกศร ใบถัดไป · กดค้าง เปิดไพ่ใหม่ |
| Flashcards (แฟลชการ์ด) | v2 | Confirm พลิก · ลูกศร เลื่อนการ์ด · กดค้าง กลับรายการชุด |
| Reading stats (สถิติการอ่าน) | v2 | Confirm โหลดใหม่ |
| Dashboard (แดชบอร์ด) | v2 | Confirm ต่อ Wi-Fi แล้วโหลดแดชบอร์ดใหม่ |

ปุ่ม Back ออกจากแอปเสมอ แอปที่ใช้ SDK v2 ต้องใช้เฟิร์มแวร์ที่มี SDK v2

เนื้อหาบน SD (UTF-8 คั่นด้วย TAB, ข้ามบรรทัดว่างและบรรทัดที่ขึ้นต้นด้วย `#`):

- **แฟลชการ์ด:** ไฟล์ `.tsv` ใน `/apps/flashcards/` ไฟล์ละชุด บรรทัดละใบ `หน้า<TAB>หลัง` (`\n` = ขึ้นบรรทัดใหม่)
  ชื่อไฟล์ใช้ได้แค่ `A-Z a-z 0-9 . _ -` ชุดละไม่เกิน 2,000 ใบ
- **โนโนแกรม:** `/apps/nonogram/puzzles.tsv` บรรทัดละโจทย์ `ชื่อ<TAB>แถว/แถว/...` `#` = ช่องที่ระบาย ไม่เกิน 15×15
- **ไพ่ทาโรต์:** `deck.tsv` และรูปไพ่ 78 ใบมากับแพ็ก (ภาพสแกน Rider-Waite-Smith 1909 จาก
  Wikimedia Commons, public domain แปลงด้วย `scripts/make_tarot_cards.py <โฟลเดอร์สแกน>`)

ไอคอนของแต่ละแอปคือ `apps/<id>/icon.bmp` (32×32, 1 บิต) สร้างด้วย `scripts/make_icons.py`

## ทำแอปเอง

```sh
pip install ziglang          # หรือติดตั้ง zig
scripts/build.sh apps/2048   # -> dist/2048.xpa
```

แอปคือโฟลเดอร์ `apps/<id>/` ที่มี `manifest.json`, ไฟล์ C และข้อมูลอื่น ๆ API อยู่ใน [sdk/x4.h](sdk/x4.h)
(คัดลอกมาจาก crosspoint-x4 `sdk/include/x4.h` ซึ่งเป็นต้นฉบับ) ตัวอย่างที่สั้นที่สุด: [apps/2048](apps/2048)

## เทส

```sh
tests/run.sh                                   # เทสบนคอม: กติกาและหน้าจอของทุกแอป ผ่าน stub ของ x4 API
tests/emu.sh <eink-qemu> <firmware.bin> [dir]  # Linux: ทุกแอปบน eink-qemu กับเฟิร์มแวร์ x4_th_test
```

<details><summary>English</summary>

Apps for the Xteink X4 running the crosspoint-x4 firmware (Pocket SDK). Each app ships as a `.xpa`
(a zip): unzip it into `/apps/` on the SD card and open it from Home → Apps. An app whose id matches a
built-in one replaces it in the list. 2048 and Minesweeper use SDK v1; Nonogram, Tarot, Flashcards, Reading stats and
Dashboard need SDK v2 (TSV row index, directory listing, reading stats, the dashboard image and Wi-Fi sync).
Back always leaves an app; holding Confirm returns to an app's own list where it has one.
Each app's hub icon is `apps/<id>/icon.bmp` (32x32, 1-bit, from `scripts/make_icons.py`). The tarot pack
ships the 78 Rider-Waite-Smith 1909 scans from Wikimedia Commons (public domain), converted by
`scripts/make_tarot_cards.py`.

To build: `pip install ziglang` (or install zig), then `scripts/build.sh [apps/<id>]` writes `dist/<id>.xpa`.
An app is `apps/<id>/` with `manifest.json`, C sources and any data files; the API is [sdk/x4.h](sdk/x4.h).
Tagging a commit publishes every `.xpa` as a GitHub release.

Tests: `tests/run.sh` drives every app on the host through a stub of the x4 API (CI runs it);
`tests/emu.sh <eink-qemu> <firmware.bin>` runs them all in eink-qemu against crosspoint-x4's `x4_th_test` build.

</details>
