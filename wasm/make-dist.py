#!/usr/bin/env python3
"""Assemble the deployable site from a Qt wasm build, with hashed file names.

GitHub Pages serves everything with Cache-Control: max-age=600, so a browser
can keep using the previous SolarSystemGazer.js/.wasm for ten minutes after a
deploy. Putting the content hash in the file name makes every new build a new
URL, so only index.html (the entry point, which cannot be renamed) can be
served stale, and it always points at assets that match it.

    python3 wasm/make-dist.py <build-dir> <out-dir>
"""

import hashlib
import sys
from pathlib import Path

APP = "SolarSystemGazer"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()[:8]


def main(build_dir: str, out_dir: str) -> int:
    build = Path(build_dir)
    out = Path(out_dir)

    missing = [f for f in (f"{APP}.js", f"{APP}.wasm", "qtloader.js", "index.html")
               if not (build / f).is_file()]
    if missing:
        print(f"error: missing build output: {', '.join(missing)}", file=sys.stderr)
        return 1

    out.mkdir(parents=True, exist_ok=True)

    # ── wasm ───────────────────────────────────────────────────────────────
    wasm = (build / f"{APP}.wasm").read_bytes()
    wasm_name = f"{APP}.{digest(wasm)}.wasm"
    (out / wasm_name).write_bytes(wasm)

    # ── emscripten glue: it loads the wasm by name, so patch before hashing ─
    js = (build / f"{APP}.js").read_text(encoding="utf-8")
    if f'"{APP}.wasm"' not in js:
        print(f"error: {APP}.js does not reference {APP}.wasm as expected",
              file=sys.stderr)
        return 1
    js = js.replace(f'"{APP}.wasm"', f'"{wasm_name}"')
    js_name = f"{APP}.{digest(js.encode('utf-8'))}.js"
    (out / js_name).write_text(js, encoding="utf-8")

    # ── Qt loader ──────────────────────────────────────────────────────────
    loader = (build / "qtloader.js").read_bytes()
    loader_name = f"qtloader.{digest(loader)}.js"
    (out / loader_name).write_bytes(loader)

    # ── page shell ─────────────────────────────────────────────────────────
    html = (build / "index.html").read_text(encoding="utf-8")
    html = html.replace(f'src="{APP}.js"', f'src="{js_name}"')
    html = html.replace('src="qtloader.js"', f'src="{loader_name}"')
    (out / "index.html").write_text(html, encoding="utf-8")

    for f in sorted(out.iterdir()):
        print(f"{f.stat().st_size / 1024:10.1f} KiB  {f.name}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1], sys.argv[2]))
