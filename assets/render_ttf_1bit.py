#!/usr/bin/env python3
# Poppins (英数) + LINE Seed JP (和文) を、実機の e-ink と同じ 1bit(アンチエイリアスなし)で描画。
# 比較用に「アンチエイリアスあり(=PCやスマホでの見え方)」も並べて、実機との差を可視化する。
from PIL import Image, ImageDraw, ImageFont

POPPINS_BOLD = "Poppins-Bold.ttf"
POPPINS_SEMI = "Poppins-SemiBold.ttf"
POPPINS_MED  = "Poppins-Medium.ttf"
POPPINS_REG  = "Poppins-Regular.ttf"
SEED_BOLD    = "LINESeedJP-Bold.ttf"
SEED_XBOLD   = "LINESeedJP-ExtraBold.ttf"
SEED_REG     = "LINESeedJP-Regular.ttf"

W = 900
canvas = Image.new("L", (W, 1700), 255)
cd = ImageDraw.Draw(canvas)
y = 20
PAD = 24

def label(text, size=15):
    """グレーの注釈ラベル（プレビュー用。実機には出ない）"""
    global y
    f = ImageFont.truetype(POPPINS_MED, size)
    cd.text((PAD, y), text, font=f, fill=120)
    y += size + 8

def rule():
    global y
    for x in range(PAD, W-PAD):
        canvas.putpixel((x, y), 180)
    y += 14

def to_1bit(img):
    # mode "1" 変換 = しきい値128で白黒2値化（アンチエイリアスを潰す）= 実機相当
    return img.convert("1").convert("L")

def render_text(text, font_path, px, aa=True):
    """1行を専用キャンバスに描いて bbox でトリミングして返す"""
    f = ImageFont.truetype(font_path, px)
    tmp = Image.new("L", (W, px*2), 255)
    d = ImageDraw.Draw(tmp)
    d.text((0, 0), text, font=f, fill=0)
    if not aa:
        tmp = to_1bit(tmp)
    bbox = Image.new("L",(W,px*2),255)  # placeholder
    # crop to content height
    return tmp

def put_pair(text, font_path, px, gap=10):
    """左=実機(1bit) / 右=参考(AAあり) を同じ行に並べる"""
    global y
    f = ImageFont.truetype(font_path, px)
    bbox = f.getbbox(text)
    tw = bbox[2]-bbox[0]
    th = bbox[3]-bbox[1]
    cellh = px + 8

    # 1bit 版（実機）
    a = Image.new("L", (max(1,tw+4), cellh), 255)
    da = ImageDraw.Draw(a)
    da.text((-bbox[0], -bbox[1]), text, font=f, fill=0)
    a = to_1bit(a)

    # AAあり版（参考）
    b = Image.new("L", (max(1,tw+4), cellh), 255)
    db = ImageDraw.Draw(b)
    db.text((-bbox[0], -bbox[1]), text, font=f, fill=0)

    canvas.paste(a, (PAD, y))
    canvas.paste(b, (PAD + a.width + 60, y))
    y += cellh + 10

# === タイトル ===
ttl = ImageFont.truetype(POPPINS_BOLD, 26)
cd.text((PAD, y), "Poppins + LINE Seed JP  /  left = 1bit e-ink (real)   right = anti-aliased (screen)", font=ttl, fill=0)
y += 40
rule()

# === 特大: 時計・気温（Poppins） ===
label("Clock / Temp  -  Poppins, very large (96-120px) : 1bit clean here")
put_pair("7:15", POPPINS_BOLD, 120)
put_pair("22°  26°/18°", POPPINS_SEMI, 64)
rule()

# === 中: 数値+ラテン（Poppins） ===
label("Latin/number body  -  Poppins (20-32px)")
put_pair("ABCabc 0123456789  22C 26/18", POPPINS_MED, 32)
put_pair("ABCabc 0123456789  NHK 6:40", POPPINS_REG, 20)
rule()

# === 大: 見出し和文（LINE Seed JP） ===
label("JP headline  -  LINE Seed JP Bold (large 40-64px)")
put_pair("傘いる", SEED_XBOLD, 64)   # 傘いる
put_pair("梅雨入り　傘いる", SEED_BOLD, 40)  # 梅雨入り 傘いる
rule()

# === 中: 和文ラベル（LINE Seed JP） ===
label("JP labels  -  LINE Seed JP (24-30px)")
put_pair("月曜日　曇りのち雨", SEED_BOLD, 30)  # 月曜日 曇りのち雨
put_pair("最高２６°　最低１８°", SEED_REG, 26)  # 最高26 最低18
rule()

# === 小: 本文・ニュース（LINE Seed JP） ===  ※ここが一番厳しい
label("JP body / news  -  LINE Seed JP (20-22px) : smallest, hardest for 1bit")
put_pair("西日本と東海が梅雨入り　平年より数日早く", SEED_REG, 22)  # 西日本と東海が梅雨入り 平年より数日早く
put_pair("今日は何の日 ― 気象記念日", SEED_REG, 20)  # 今日は何の日 ― 気象記念日
put_pair("１４時から雨　傘いる", SEED_BOLD, 22)  # 14時から雨 傘いる
rule()

canvas = canvas.crop((0, 0, W, min(y+10, canvas.height)))
canvas.save("ttf_1bit_preview.png")
print("saved ttf_1bit_preview.png", canvas.size)
