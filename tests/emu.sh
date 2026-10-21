#!/usr/bin/env bash
# End-to-end run of every app in eink-qemu (Linux), no device needed.
#
#   tests/emu.sh <eink-qemu dir> <firmware.bin from crosspoint-x4's x4_th_test> [workdir]
#
# Builds all apps, unpacks them into /apps/ on a fresh card (plus placeholder
# tarot cards, reading stats and a dashboard image), then opens each app from
# Home -> Apps, uses it, leaves it and checks the firmware log: the app starts,
# never stops on a trap or the gas budget, saves where it should, reopens with
# its state, and gives its heap back. Needs python3 (Pillow), mtools and zig.
# Prints PASS/FAIL per check, leaves screenshots in the workdir, exits non-zero
# on any failure.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
EQ=$(realpath "$1") FW=$(realpath "$2") WORK=${3:-$(mktemp -d)}
mkdir -p "$WORK" && cd "$WORK"
export X4MACHINE=x4

"$ROOT/scripts/build.sh" >/dev/null
rm -rf sdroot sd.img && mkdir -p sdroot/apps sdroot/.crosspoint
for x in "$ROOT"/dist/*.xpa; do python3 -m zipfile -e "$x" sdroot/apps/; done
mkdir -p sdroot/apps/tarot/cards
python3 - <<'EOF'
import json
from PIL import Image, ImageDraw
# Placeholder tarot cards: a frame and the card's file name.
for line in open("sdroot/apps/tarot/deck.tsv", encoding="utf-8"):
    if not line.strip() or line.startswith("#"):
        continue
    name = line.split("\t", 1)[0]
    img = Image.new("1", (300, 520), 1)
    d = ImageDraw.Draw(img)
    d.rectangle((4, 4, 295, 515), outline=0, width=6)
    d.text((20, 250), name, fill=0)
    img.save(f"sdroot/apps/tarot/cards/{name}.bmp")
# Reading stats as the firmware writes them (no clock in the emulator, so no days).
json.dump({"v": 1, "sec": 7 * 3600 + 300, "pages": 1234, "sessions": 40, "finished": 3, "seq": 2,
           "lastBook": "/Books/alice.epub",
           "books": {"/Books/alice.epub": {"t": "Alice ในแดนมหัศจรรย์", "sec": 5400, "pct": 37, "last": 2}},
           "days": {}, "synced": True},
          open("sdroot/.crosspoint/reading-stats.json", "w"), ensure_ascii=False)
# A 2,000-card deck: indexing it must not run into the gas budget.
with open("sdroot/apps/flashcards/big.tsv", "w", encoding="utf-8") as f:
    f.write("# generated\n")
    for i in range(2000):
        f.write(f"word {i + 1}\tคำที่ {i + 1} ความหมายยาวพอให้ไฟล์ใหญ่ขึ้นหน่อย\n")
# A dashboard image as x4-hub sends it (server/x4hub/bmp2.py): 480x800, 2 bpp, the panel's four greys.
import struct
LEVELS = (0, 85, 170, 255)
img = Image.new("L", (480, 800), 255)
d = ImageDraw.Draw(img)
d.rectangle((0, 0, 479, 160), fill=0)
d.rectangle((20, 200, 460, 400), fill=170)
d.rectangle((20, 440, 460, 640), fill=85)
d.text((40, 60), "DASHBOARD 21:35", fill=255)
w, h = img.size
stride = ((w * 2 + 31) // 32) * 4
px = img.load()
rows = bytearray()
for y in range(h - 1, -1, -1):
    row = bytearray(stride)
    for x in range(w):
        row[x >> 2] |= min(range(4), key=lambda i: abs(LEVELS[i] - px[x, y])) << (6 - 2 * (x & 3))
    rows += row
palette = b"".join(bytes((v, v, v, 0)) for v in LEVELS)
offset = 14 + 40 + len(palette)
with open("sdroot/.crosspoint/dashboard.bmp", "wb") as f:
    f.write(struct.pack("<2sIHHI", b"BM", offset + len(rows), 0, 0, offset))
    f.write(struct.pack("<IiiHHIIiiII", 40, w, h, 1, 2, 0, len(rows), 2835, 2835, 4, 4) + palette + bytes(rows))
EOF
python3 "$EQ/eink-qemu/mkflash.py" --machine x4 "$FW" flash.bin >/dev/null
python3 "$EQ/eink-qemu/mksd.py" sd.img 2050 sdroot >/dev/null
off=$(python3 -c "import struct;print(struct.unpack('<I',open('sd.img','rb').read(512)[454:458])[0]*512)")

drive() { python3 "$EQ/eink-qemu/drive.py" flash.bin sd.img "$@" >/dev/null; }
fails=0
pass() { echo "PASS $1"; }
fail() { echo "FAIL $1"; fails=$((fails + 1)); }
check() {  # check <name> <log> <pattern>
  if grep -q -- "$3" "$2"; then pass "$1"; else fail "$1 (no '$3' in $2)"; fi
}
refute() {  # refute <name> <log> <pattern>
  if grep -q -- "$3" "$2"; then fail "$1 ('$3' in $2)"; else pass "$1"; fi
}
saved() {  # saved <name> <app id>: the app's save file exists on the card
  if mdir -i "sd.img@@$off" "::/.crosspoint/apps/sdk-$2.sav" >/dev/null 2>&1; then pass "$1"; else fail "$1"; fi
}
# Every start of <id> logs "heap A -> B", every exit "heap C": the exit must give
# back what the start took (C >= A - 1 KB), judged from the second run on.
heap() {  # heap <log> <id>
  python3 - "$1" "$2" <<'EOF'
import re, sys
log, app = sys.argv[1], sys.argv[2]
starts, exits = [], []
for line in open(log, errors="replace"):
    m = re.search(rf"start {re.escape(app)}: ok, heap (\d+) -> (\d+)", line)
    if m:
        starts.append(int(m.group(1)))
    m = re.search(rf"exit {re.escape(app)}, heap (\d+)", line)
    if m:
        exits.append(int(m.group(1)))
pairs = list(zip(starts, exits))[1:] or list(zip(starts, exits))
if not pairs:
    print(f"FAIL {app} heap: no start/exit pair"); sys.exit(1)
worst = max(a - c for a, c in pairs)
print(f"{'PASS' if worst <= 1024 else 'FAIL'} {app} heap back on exit (worst {worst} B of {len(pairs)} runs)")
sys.exit(worst > 1024)
EOF
}

# Hub rows (x4_th_test): the built-in Counter, then SDK apps by English name.
# The SDK apps replace the built-ins with the same ids.
open_hub="press:down wait:2 press:down wait:2 press:confirm wait:6"
downs() { for _ in $(seq "$1"); do printf 'press:down wait:2 '; done; }
open_app() { echo "$open_hub $(downs "$1")press:confirm wait:8"; }
back="press:back wait:6"

# 2048: four moves, Back saves, reopen.
drive r2048.txt wait:35 $(open_app 1) shot:2048-start.png press:left wait:4 press:up wait:4 press:right wait:4 \
  press:down wait:4 shot:2048-moved.png $back press:confirm wait:8 shot:2048-reopened.png $back
check "hub replaces the built-ins" r2048.txt "7 SDK apps, 1 built-in"
check "2048 starts" r2048.txt "start 2048: ok"
saved "2048 saved" 2048
heap r2048.txt 2048 || fails=$((fails + 1))

# Minesweeper: open the centre, move, flag (hold), Back saves; reopen; then sleep and wake.
drive rmines.txt wait:35 $(open_app 4) press:confirm wait:5 shot:mines-opened.png press:right wait:3 press:right wait:3 \
  press:right wait:3 press:right wait:3 press:confirm:900 wait:5 shot:mines-flag.png $back press:confirm wait:8 \
  shot:mines-reopened.png press:power:1500 wait:20
check "minesweeper starts" rmines.txt "start minesweeper: ok"
saved "minesweeper saved" minesweeper
check "minesweeper exits on sleep" rmines.txt "exit minesweeper"
heap rmines.txt minesweeper || fails=$((fails + 1))
drive rwake.txt wait:35 shot:wake.png $(open_app 4) shot:mines-after-sleep.png $back
check "minesweeper reopens after sleep" rwake.txt "start minesweeper: ok"

# Nonogram: open Heart, fill two cells, cross one, Back saves, reopen keeps the marks.
drive rnono.txt wait:35 $(open_app 5) shot:nono-list.png press:confirm wait:6 shot:nono-start.png press:down wait:3 \
  press:right wait:3 press:confirm wait:3 press:right wait:3 press:confirm wait:3 press:right wait:3 \
  press:confirm:900 wait:4 shot:nono-marked.png $back press:confirm wait:8 shot:nono-list-again.png \
  press:confirm wait:6 shot:nono-resumed.png $back
check "nonogram starts" rnono.txt "start nonogram: ok"
saved "nonogram saved" nonogram
heap rnono.txt nonogram || fails=$((fails + 1))

# Flashcards: the deck list (big.tsv, english-thai.tsv), the 2,000-card deck, then the sample deck:
# flip, move, back to the list (hold), reopen resumes.
drive rflash.txt wait:35 $(open_app 3) shot:flash-list.png press:confirm wait:10 shot:flash-big.png \
  press:up wait:4 shot:flash-big-last.png press:confirm:900 wait:5 press:down wait:3 press:confirm wait:6 \
  shot:flash-front.png press:confirm wait:4 shot:flash-back.png press:down wait:4 press:down wait:4 press:down wait:4 \
  press:confirm:900 wait:5 shot:flash-list-again.png $back press:confirm wait:8 press:down wait:3 press:confirm wait:6 \
  shot:flash-resumed.png $back
check "flashcards starts" rflash.txt "start flashcards: ok"
saved "flashcards saved" flashcards
heap rflash.txt flashcards || fails=$((fails + 1))

# Tarot: three cards (image, meaning, next), hold back to the list, a daily card.
drive rtarot.txt wait:35 $(open_app 7) shot:tarot-list.png press:down wait:2 press:confirm wait:10 shot:tarot-1.png \
  press:confirm wait:5 shot:tarot-1-meaning.png press:right wait:10 shot:tarot-2.png press:confirm:900 wait:5 \
  press:up wait:2 press:confirm wait:10 shot:tarot-daily.png $back press:confirm wait:10 $back
check "tarot starts" rtarot.txt "start tarot: ok"
heap rtarot.txt tarot || fails=$((fails + 1))

# Reading stats (v2): the stats screen, reload.
drive rstats.txt wait:35 $(open_app 6) shot:stats.png press:confirm wait:5 $back press:confirm wait:8 $back
check "stats starts" rstats.txt "start stats: ok"
heap rstats.txt stats || fails=$((fails + 1))

# Dashboard (v2): the stored image, then an update (no network in the emulator: it must fail cleanly).
drive rdash.txt wait:35 $(open_app 2) shot:dashboard.png press:confirm wait:40 shot:dashboard-update.png $back
check "dashboard starts" rdash.txt "start dashboard: ok"
check "dashboard update returns" rdash.txt "sync: wifi="
check "dashboard exits" rdash.txt "exit dashboard"

for f in r*.txt; do
  refute "no app stopped in $f" "$f" "\[SDK\].*\(trap\|gas\|out of\|must export\|parse:\|load:\)"
done
grep -h "\[SDK\]\|\[HUB\]" r*.txt | sed -E 's/^\[[0-9]+\] //' | sort | uniq -c | sort -rn | head -40
echo "screenshots and logs in $WORK"
exit $fails
