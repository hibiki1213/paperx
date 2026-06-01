#!/usr/bin/env python3
# unifont_jp (= U8g2 の日本語ドットフォント相当) の実データを描画してプレビューPNGを作る
from PIL import Image

HEX = "unifont_jp-16.0.04.hex"
OUT = "unifont_preview.png"

def load(path):
    d = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or ":" not in line:
                continue
            cp, data = line.split(":", 1)
            try:
                d[int(cp, 16)] = data
            except ValueError:
                pass
    return d

FONT = load(HEX)

def glyph_bits(ch):
    data = FONT.get(ord(ch))
    if data is None:
        return 8, [[0]*8 for _ in range(16)]
    per = len(data) // 16          # hex chars per row (2 -> 8px, 4 -> 16px)
    w = per * 4
    rows = []
    for r in range(16):
        v = int(data[r*per:(r+1)*per], 16)
        rows.append([(v >> (w-1-c)) & 1 for c in range(w)])
    return w, rows

def render_line(text, scale=1, tracking=0):
    glyphs = [glyph_bits(c) for c in text]
    total_w = sum(w for w, _ in glyphs) + tracking * max(0, len(glyphs)-1)
    img = Image.new("L", (max(1, total_w), 16), 255)
    px = img.load()
    x = 0
    for w, rows in glyphs:
        for r in range(16):
            for c in range(w):
                if rows[r][c]:
                    px[x+c, r] = 0
        x += w + tracking
    if scale != 1:
        img = img.resize((img.width*scale, img.height*scale), Image.NEAREST)
    return img

# --- 組版 ---
W = 860
PAD = 18
canvas = Image.new("L", (W, 1400), 255)
y = PAD

def put(img, x=PAD):
    global y
    canvas.paste(img, (x, y))
    y += img.height

def gap(n):
    global y
    y += n

def rule():
    global y
    for x in range(PAD, W-PAD):
        canvas.putpixel((x, y), 0)
    y += 1

put(render_line("U8g2 Japanese font preview  /  unifont_jp 16px", 1)); gap(6)
rule(); gap(12)

put(render_line("16px tou (toubai) -- news/honbun no jissun", 1)); gap(2)
put(render_line("16px 等倍 ― ニュース・本文の実寸", 1)); gap(10)
put(render_line("西日本と東海が梅雨入り　平年より数日早く", 1)); gap(8)
put(render_line("今日は何の日 ― 気象記念日", 1)); gap(8)
put(render_line("曇りのち雨　14時から雨　傘いる", 1)); gap(18)
rule(); gap(12)

put(render_line("32px (2bai kakudai)", 1)); gap(2)
put(render_line("32px ＝ 2倍拡大", 1)); gap(8)
put(render_line("梅雨入り　傘いる", 2)); gap(18)
rule(); gap(12)

put(render_line("48px (3bai / midashi-you, block-ppoku naru)", 1)); gap(2)
put(render_line("48px ＝ 3倍拡大（見出し用）", 1)); gap(8)
put(render_line("傘いる", 3)); gap(18)
rule(); gap(12)

put(render_line("number & latin (16px / 32px)", 1)); gap(6)
put(render_line("7:15   22C  26C/18C   ABCabc 0123456789", 1)); gap(8)
put(render_line("7:15   22C", 2)); gap(10)

canvas = canvas.crop((0, 0, W, min(y+PAD, canvas.height)))
canvas.save(OUT)
print("saved", OUT, canvas.size)
