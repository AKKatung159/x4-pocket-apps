#!/usr/bin/env python3
"""pack.py <app src dir> <build dir> <out.xpa>: zips <id>/manifest.json, app.wasm and the
app's data files (everything but C sources) so that unzipping at /apps/ on the card installs it."""
import json, re, sys, zipfile
from pathlib import Path

src, build, out = map(Path, sys.argv[1:4])
manifest = json.loads((src / "manifest.json").read_text(encoding="utf-8"))
app_id = manifest["id"]
header = (Path(__file__).resolve().parents[1] / "sdk/x4.h").read_text(encoding="utf-8")
sdk = int(re.search(r"#define X4_SDK_VERSION (\d+)", header).group(1))
assert 1 <= manifest.get("sdk", 0) <= sdk, f"sdk must be 1..{sdk}"
files = {"app.wasm": build / "app.wasm"}
for p in sorted(src.rglob("*")):
    if p.is_file() and p.suffix not in (".c", ".h"):
        files[p.relative_to(src).as_posix()] = p
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for name, path in files.items():
        z.write(path, f"{app_id}/{name}")
print(f"{out.name}: {len(files)} files, {out.stat().st_size} bytes")
