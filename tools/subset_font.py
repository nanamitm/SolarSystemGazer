#!/usr/bin/env python3
"""Regenerate resources/fonts/NotoSansJP-subset.ttf.

Qt for WebAssembly only ships DejaVu, so every Japanese label in the UI would
render as tofu. Shipping the full variable Noto Sans JP would add ~9.6 MB to
the download, so the build embeds a static Regular instance subset to exactly
the characters the UI source files use (~550 glyphs, ~115 KB).

Run this after adding new Japanese strings to the UI:

    python -m pip install fonttools
    python tools/subset_font.py path/to/NotoSansJP.ttf

The source font is the variable Noto Sans JP (wght 100–900) from Google Fonts;
it is instanced at wght=400 first, because its default instance is Thin.
"""

import glob
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "resources" / "fonts" / "NotoSansJP-subset.ttf"
SOURCES = ("*.cpp", "*.h", "qml/*.qml")


def used_characters() -> str:
    chars = set()
    for pattern in SOURCES:
        for path in glob.glob(str(ROOT / pattern)):
            chars.update(Path(path).read_text(encoding="utf-8", errors="ignore"))
    # printable ASCII plus every non-ASCII character appearing in the sources
    keep = {chr(c) for c in range(0x20, 0x7F)} | {c for c in chars if ord(c) > 0x7F}
    keep.discard("﻿")
    return "".join(sorted(keep))


def main(source_font: str) -> int:
    chars = used_characters()
    print(f"{len(chars)} characters kept")

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        charfile = tmp / "chars.txt"
        charfile.write_text(chars, encoding="utf-8")

        static = tmp / "regular.ttf"
        subprocess.run([sys.executable, "-m", "fontTools.varLib.instancer",
                        source_font, "wght=400", "-o", str(static)], check=True)

        subprocess.run([sys.executable, "-m", "fontTools.subset", str(static),
                        f"--text-file={charfile}", f"--output-file={OUT}",
                        "--layout-features=", "--no-hinting",
                        "--name-IDs=0,1,2,3,4,5,6,13,14"], check=True)

    # instancing leaves the variable font's "Thin" name records behind
    from fontTools.ttLib import TTFont
    font = TTFont(OUT)
    for nid, value in ((1, "Noto Sans JP"), (2, "Regular"),
                       (4, "Noto Sans JP Regular"), (6, "NotoSansJP-Regular")):
        font["name"].setName(value, nid, 3, 1, 0x409)
        font["name"].setName(value, nid, 1, 0, 0)
    font.save(OUT)

    print(f"{OUT.relative_to(ROOT).as_posix()}: {OUT.stat().st_size / 1024:.0f} KiB")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
