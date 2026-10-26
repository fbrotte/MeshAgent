#!/usr/bin/env python3
# Re-embed JavaScript modules from modules/ into microscript/ILibDuktape_Polyfills.c.
#
# MeshAgent stores each embedded module as a zlib-compressed, base64-encoded blob
# inside ILibDuktape_Polyfills.c, e.g.:
#   duk_peval_string_noresult(ctx, "addCompressedModule('NAME', Buffer.from('BASE64', 'base64'), 'TIMESTAMP');");
# The reference flow (modules/code-utils.js shrink) requires a MeshAgent JS runtime.
# This standalone equivalent lets CI (or a Linux box) regenerate the blob with only the
# Python standard library. It reproduces the exact byte layout: source is read as LF,
# converted to CRLF (MeshAgent convention), then zlib-compressed. Timestamps are kept.
#
# Usage: python3 embed-modules.py [module-name ...]   (default: agent-installer)

import base64
import re
import sys
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
POLYFILLS = REPO / "microscript" / "ILibDuktape_Polyfills.c"


def reencode(name: str, src_path: Path, text: str) -> str:
    src = src_path.read_bytes()
    crlf = src.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    blob = base64.b64encode(zlib.compress(crlf, 9)).decode("ascii")
    pattern = re.compile(
        r"(addCompressedModule\('" + re.escape(name) + r"', Buffer\.from\(')[^']*('[^)]*\))"
    )
    new_text, n = pattern.subn(lambda m: m.group(1) + blob + m.group(2), text)
    if n != 1:
        raise SystemExit(f"error: expected 1 embedded copy of '{name}', found {n}")
    return new_text


def main() -> None:
    names = sys.argv[1:] or ["agent-installer"]
    text = POLYFILLS.read_text(encoding="latin-1")
    for name in names:
        src = REPO / "modules" / f"{name}.js"
        if not src.exists():
            raise SystemExit(f"error: {src} not found")
        text = reencode(name, src, text)
        print(f"re-embedded {name}")
    POLYFILLS.write_text(text, encoding="latin-1")


if __name__ == "__main__":
    main()
