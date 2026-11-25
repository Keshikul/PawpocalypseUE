"""Layered pictures for the store art tools: a stack of named RGBA layers that flattens to the PNG and saves as a
layered Photoshop file (designer, 2026-11-25: "Make psd"; store assets are PNG and Photoshop only).

    from store_psd import Layers, save_psd
    L = Layers(920, 430)
    sky = L.add("sky", picture)          # a whole picture as a layer
    draw_on(L.new("crowd"))              # a blank layer to draw on
    L.flat()                             # the composite, for the PNG
    save_psd(path, L)                    # the PSD: one pixel layer per entry, cropped to what it covers

Needs psd-tools (`py -m pip install psd-tools`), only when a PSD is written.
"""

from pathlib import Path

from PIL import Image


class Layers:
    def __init__(self, w, h):
        self.size = (w, h)
        self.layers = []   # (name, RGBA image of self.size)

    def add(self, name, img):
        img = img.convert("RGBA")
        if img.size != self.size:
            raise ValueError("layer %s is %s, the picture is %s" % (name, img.size, self.size))
        self.layers.append((name, img))
        return img

    def new(self, name):
        return self.add(name, Image.new("RGBA", self.size, (0, 0, 0, 0)))

    def flat(self):
        out = Image.new("RGBA", self.size, (0, 0, 0, 0))
        for _name, img in self.layers:
            out.alpha_composite(img)
        return out

    def resized(self, w, h):
        """Every layer scaled to w x h (each in premultiplied alpha, so edges do not darken)."""
        out = Layers(w, h)
        for name, img in self.layers:
            out.add(name, img.convert("RGBa").resize((w, h), Image.LANCZOS).convert("RGBA"))
        return out

    @staticmethod
    def single(name, img):
        L = Layers(*img.size)
        L.add(name, img)
        return L


def save_psd(path, layers):
    """A layered PSD: each layer a pixel layer named as given, cropped to its pixels, bottom to top."""
    from psd_tools import PSDImage
    from psd_tools.api.layers import PixelLayer
    psd = PSDImage.new(mode="RGB", size=layers.size)
    for name, img in layers.layers:
        box = img.getchannel("A").getbbox()
        if box:
            psd.append(PixelLayer.frompil(img.crop(box), psd, name, box[1], box[0]))
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    psd.save(str(path))


def psd_matches(path, flat, tolerance=3.0):
    """Reads a written PSD back and compares its composite with the PNG's picture: the mean difference per channel."""
    import numpy as np
    from psd_tools import PSDImage
    def on_white(img):
        img = img.convert("RGBA")
        base = Image.new("RGBA", img.size, (255, 255, 255, 255))
        base.alpha_composite(img)
        return base.convert("RGB")
    a = np.asarray(on_white(PSDImage.open(str(path)).composite(force=True)), np.float32)
    b = np.asarray(on_white(flat), np.float32)
    return float(np.abs(a - b).mean()) <= tolerance, float(np.abs(a - b).mean())
