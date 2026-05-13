#!/usr/bin/env python3
"""Rebuild merged enemy atlases from strip PNGs (requires Pillow). Run from repo root."""
from pathlib import Path

try:
    from PIL import Image
except ImportError as e:
    raise SystemExit("Install Pillow: pip install Pillow") from e

root = Path(__file__).resolve().parents[1] / "assets" / "sprites" / "enemy"


def save_atlas(subdir: str, rows_imgs: list, out_name: str) -> None:
    w = max(im.width for im in rows_imgs)
    h = sum(im.height for im in rows_imgs)
    canvas = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    y = 0
    for im in rows_imgs:
        canvas.paste(im, (0, y))
        y += im.height
    out = root / subdir / out_name
    out.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(out)
    print("saved", out, canvas.size)


def main() -> None:
    a = Image.open(root / "01.Slime/IdleSlime.png")
    b = Image.open(root / "01.Slime/WalkSlime.png")
    save_atlas("01.Slime", [a, b], "slime_atlas.png")

    idle = Image.open(root / "02.BugBit/IdleBug.png")
    walk = Image.open(root / "02.BugBit/WalkBug.png")
    w, h = idle.width, idle.height + walk.height
    c = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    c.paste(idle, (0, 0))
    c.paste(walk, (0, idle.height))
    out = root / "02.BugBit/bugbit_atlas.png"
    c.save(out)
    print("saved", out, c.size)

    f = Image.open(root / "03.Spookmoth/FlySpookmoth.png")
    save_atlas("03.Spookmoth", [f], "spookmoth_atlas.png")

    idle = Image.open(root / "04.Pebblin/IdlePebblin.png")
    atk = Image.open(root / "04.Pebblin/AttackPebblin.png")
    w = max(idle.width, atk.width)
    h = idle.height + atk.height
    c = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    c.paste(idle, (0, 0))
    c.paste(atk, (0, idle.height))
    out = root / "04.Pebblin/pebblin_atlas.png"
    c.save(out)
    print("saved", out, c.size)


if __name__ == "__main__":
    main()
