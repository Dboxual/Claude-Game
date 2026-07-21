#!/usr/bin/env python3
"""Assemble frames from --record-dir into an animated WebP test recording.

Usage: python3 tools/make_test_recording.py FRAME_DIR OUTPUT.webp [fps]
Pillow is a test-machine dependency only; it is not linked into the game.
"""
from pathlib import Path
import sys
from PIL import Image


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: make_test_recording.py FRAME_DIR OUTPUT.webp [fps]")
        return 2
    frame_dir = Path(sys.argv[1])
    output = Path(sys.argv[2])
    fps = float(sys.argv[3]) if len(sys.argv) > 3 else 12.0
    paths = sorted(frame_dir.glob("frame_*.png"))
    if not paths:
        print(f"no recording frames in {frame_dir}")
        return 1

    frames = [Image.open(path).convert("RGB") for path in paths]
    output.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        output,
        "WEBP",
        save_all=True,
        append_images=frames[1:],
        duration=max(1, round(1000.0 / fps)),
        loop=0,
        quality=76,
        method=4,
    )
    for frame in frames:
        frame.close()
    print(f"recording: {len(paths)} frames at {fps:g} fps -> {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
