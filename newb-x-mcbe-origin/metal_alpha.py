#!/usr/bin/env python3
# Writes the Origin material flags into the alpha channel of a block texture.
# The colours (RGB) are not changed.
#
# usage: python metal_alpha.py in.png out.png gold|silver|copper|iron|gloss [rough]
#
# alpha = code / 255, a plain block has code 255 (no flags)
#   bit 0x80 cleared: metal
#   bit 0x40 cleared: smooth, the block mirrors the sky and clouds
#   bit 0x20 cleared: fluorescence, the block ignores lighting
#   bits 0x0C: metal type, 00 gold, 01 silver, 10 copper, 11 iron
# gold = 51, silver = 55, copper = 59, iron = 63 (add 64 for a rough metal)
# gloss is a smooth block that is not a metal (glass like, 191): ice, obsidian, gems
import sys
from PIL import Image

TYPES = {'gold': 0, 'silver': 1, 'copper': 2, 'iron': 3}
NAMES = list(TYPES) + ['gloss']

def code(metal, smooth=True):
    if metal == 'gloss':
        return 0xFF & ~0x40
    c = 0xFF & ~0x80
    if smooth:
        c &= ~0x40
    return (c & ~0x0C) | (TYPES[metal] << 2)

def main():
    a = sys.argv[1:]
    if len(a) < 3 or a[2] not in NAMES:
        print(__doc__)
        return
    im = Image.open(a[0]).convert('RGBA')
    alpha = code(a[2], not (len(a) > 3 and a[3] == 'rough'))
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, _ = px[x, y]
            px[x, y] = (r, g, b, alpha)
    im.save(a[1])
    print(a[1], a[2], 'alpha code', alpha)

main()
