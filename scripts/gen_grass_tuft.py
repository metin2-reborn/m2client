#!/usr/bin/env python3
"""Draws the grass tuft texture the terrain grass uses (d:/ymir work/special/grass_tuft.tga).

The tuft is a fan of tapered, slightly curved blades. It is near-white with a darker root: the grass
vertices are tinted with the colour of the terrain tile they grow on, so the texture only carries the
shape and the light falloff. The seed is fixed, so the output is the same on every run.

    python3 scripts/gen_grass_tuft.py [output.tga]
"""

import math
import random
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

SIZE = 256
SUPERSAMPLE = 4
SEED = 20261024

DEFAULT_OUTPUT = Path(__file__).resolve().parents[2] / "m2game/data/system/ymir work/special/grass_tuft.tga"


def blade(draw, rng, big):
    """One blade from the bottom edge upwards, as a filled polygon with a brightness gradient."""
    base_x = big * (0.5 + rng.gauss(0.0, 0.12))
    height = big * rng.uniform(0.55, 0.97)
    width = big * rng.uniform(0.03, 0.05)  # at least 5 px at the final size, so the mips keep it
    lean = rng.uniform(-0.28, 0.28) * height
    bend = rng.uniform(0.6, 1.6)
    shade = rng.uniform(0.85, 1.0)
    steps = 24

    left, right = [], []
    for i in range(steps + 1):
        t = i / steps
        y = big - t * height
        x = base_x + lean * (t ** bend)
        half = width * 0.5 * (1.0 - t) ** 0.8
        left.append((x - half, y))
        right.append((x + half, y))

    # Root dark, tip bright: drawn as stacked segments of the polygon.
    for i in range(steps):
        t = (i + 0.5) / steps
        level = int(255 * shade * (0.7 + 0.3 * t ** 0.7))
        quad = [left[i], right[i], right[i + 1], left[i + 1]]
        draw.polygon(quad, fill=(level, level, level, 255))


def main():
    output = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUTPUT
    rng = random.Random(SEED)
    big = SIZE * SUPERSAMPLE

    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    # Back blades first (shorter, darker), front ones last.
    blades = sorted((rng.random() for _ in range(rng.randint(14, 18))))
    for depth in blades:
        blade(draw, rng, big)

    small = image.resize((SIZE, SIZE), Image.LANCZOS)

    # Spread the blade colour into the transparent texels so filtering and mips do not pull in black.
    r, g, b, a = small.split()
    rgb = Image.merge("RGB", (r, g, b))
    mask = a.point(lambda v: 255 if v > 0 else 0)
    filled = rgb.copy()
    for _ in range(8):
        grown = filled.filter(ImageFilter.MaxFilter(3))
        filled = Image.composite(filled, grown, mask)
        mask = mask.filter(ImageFilter.MaxFilter(3))
    # Binary alpha: the grass is alpha tested.
    alpha = a.point(lambda v: 255 if v >= 96 else 0)
    result = Image.merge("RGBA", (*filled.split(), alpha))

    output.parent.mkdir(parents=True, exist_ok=True)
    result.save(output)
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
