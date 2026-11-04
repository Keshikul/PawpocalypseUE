"""SPIKE S3 (docs/04 §8): compare Unreal captures of the field sprites with the ideal references. Throwaway.

For each image (Saved/Spikes/S3/<variant>_<comp>_<res>.png, plus SourceArt/png/spike_s3/godot_like_1080.png), per sprite rect
of manifest.csv, against ref_<res>.png (each SVG rasterised by resvg directly at its drawn size, over black):
  interior  mean |RGB error| over pixels inside the sprite (the pixel and its 8 neighbours are all sprite in the reference):
            colour/compression error, edges excluded
  all       mean |RGB error| over the whole cell (edges and outlines included)
  sharp     gradient energy of the image / of the reference in the cell: 1.0 = as sharp as ideal, < 1 = blurrier
Reports the mean over sprites and the worst sprite.

  py tools/spikes/spike_s3_compare.py
"""
import csv
import os

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
S3 = os.path.join(ROOT, "SourceArt/png/spike_s3")
CAP = os.path.join(ROOT, "Saved/Spikes/S3")


def load(path):
    return np.asarray(Image.open(path).convert("RGB"), dtype=np.float64)


def grad(a):
    g = a.mean(axis=2)
    return np.abs(np.diff(g, axis=0)).sum() + np.abs(np.diff(g, axis=1)).sum()


def measure(img, ref, rects, k):
    out = []
    for name, (x, y, s) in rects.items():
        x, y, s = x * k, y * k, s * k
        a, r = img[y:y + s, x:x + s], ref[y:y + s, x:x + s]
        sprite = r.max(axis=2) > 8
        inner = sprite.copy()
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                inner &= np.roll(np.roll(sprite, dy, 0), dx, 1)
        err = np.abs(a - r).mean(axis=2)
        out.append((name, err[inner].mean() if inner.any() else 0.0, err.mean(), grad(a) / max(grad(r), 1.0)))
    return out


def main():
    rects = {}
    for row in csv.DictReader(open(os.path.join(S3, "manifest.csv"))):
        rects[row["name"]] = (int(row["screen_x"]), int(row["screen_y"]), int(row["screen_size"]))
    refs = {"1080": load(os.path.join(S3, "ref_1080.png")), "2160": load(os.path.join(S3, "ref_2160.png"))}
    images = [("godot_like_1080", os.path.join(S3, "godot_like_1080.png"), "1080")]
    if os.path.isdir(CAP):
        for f in sorted(os.listdir(CAP)):
            if f.endswith(".png"):
                # <variant>_<comp>_<res>.png (capture) or <variant>_<comp>_<res>_screen.png (back buffer, windowed)
                images.append((f[:-4], os.path.join(CAP, f), "2160" if "_2160" in f else "1080"))
    print("%-18s %22s %22s %22s" % ("image", "interior err (worst)", "all err (worst)", "sharpness (worst)"))
    for label, path, res in images:
        m = measure(load(path), refs[res], rects, 1 if res == "1080" else 2)
        col = lambda i, worst: "%7.2f (%6.2f %-8.8s)" % (np.mean([v[i] for v in m]), worst(v[i] for v in m),
                                                       [v[0] for v in m if v[i] == worst(w[i] for w in m)][0].replace("enemy_", ""))
        print("%-18s %s %s %s" % (label, col(1, max), col(2, max), col(3, min)))


if __name__ == "__main__":
    main()
