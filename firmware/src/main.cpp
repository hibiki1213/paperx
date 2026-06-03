// PaperX — 常時ニュース表示 + 毎分の時計更新（部分更新）。
// USB給電モード（常時稼働）前提。電池モード/deep sleep は M6。
//   毎分: 時計だけ partial refresh（高速・低点滅）
//   10分: データ再取得（天気・ニュース）＋full refresh（ゴースト解消）
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <WiFi.h>
#include <WiFiManager.h>   // tzapu/WiFiManager
#include "portal_fonts.h"  // PORTAL_HEAD: 埋め込みフォント(@font-face)
#include <Preferences.h>
#include "weather.h"
#include "news.h"
#include "market.h"          // 為替・日経平均（ページ2）
#include "transit.h"         // 鉄道遅延（ページ2）
#include "ota.h"             // GitHub Releases からの無線ファーム更新
#include "fonts/wx_fonts.h" // pop_temp/pop_clock/pop_mid/lsjp_xb30/lsjp_b24/lsjp_r20/lsjp_r13/lsjp_news
#include "wx_icons.h"        // Lucide天気アイコン(1bit ビットマップ: 120px/44px)
#include <qrcode.h>          // ricmoo/QRCode: Wi-Fi設定AP参加用QR
#include <time.h>
#include <math.h>

// === Seeed XIAO "ePaper Display Board (A)" のピン配（生GPIO） ===
#define EPD_CS   44
#define EPD_DC   10
#define EPD_RST  38
#define EPD_BUSY 4
#define EPD_PWR  43   // TFT_ENABLE: パネル電源(HIGHで給電)
#define BTN_BOOT 0

// 物理ボタン（Seeed EE04 想定: active-low + INPUT_PULLUP）。
// ※実機GPIOは要確認。起動後に各ピンの押下をシリアルに出すので、KEY1/KEY2の実割当を見て調整する。
#define PIN_HOME 2   // KEY1: ホーム（ページ1）へ
#define PIN_NEXT 3   // KEY2: 次のページへ切替
#define PIN_AUX  5   // 予備

GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(
    GxEPD2_750_T7(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

static const char* AP_NAME = "PaperX-Setup";
static const char* TZ_JST  = "JST-9";
static const char* NTP1    = "ntp.nict.jp";
static const char* NTP2    = "pool.ntp.org";

static const int W = 800, Hh = 480;
static const int HD_BOT = 72, MAIN_BOT = 356, DIV_X = 452, MOON_DIV = 648;
// 時計の部分更新ウィンドウ（ヘッダ右側。罫線 y70-72 は含めない）。
static const int CLK_X = 560, CLK_Y = 0, CLK_W = 240, CLK_H = 68;

// データ再取得＋全面更新の間隔
static const unsigned long REFRESH_MS = 10UL * 60UL * 1000UL;  // 10分
// OTA 更新確認の間隔（GitHub API 60req/h 制限に余裕。起動時にも1回確認する）
static const unsigned long OTA_CHECK_MS = 6UL * 60UL * 60UL * 1000UL;  // 6時間

// --- 状態 ---------------------------------------------------------------
Preferences prefs;
static double g_lat = 35.6895, g_lon = 139.6917;
static String g_place = "Tokyo";
static WiFiManagerParameter *p_lat, *p_lon, *p_place;

static Weather g_w;
static News    g_news;
static int     g_lastMin = -1;
static unsigned long g_lastRefreshMs = 0;
static unsigned long g_lastOtaMs = 0;

// ページ2（マーケット & 運行）用のデータと、現在表示中のページ。
static Market  g_mkt;
static Transit g_tr;
enum { PAGE_HOME = 0, PAGE_MARKET, PAGE_STATUS, PAGE_COUNT };
static int g_page = PAGE_HOME;

// ページ3: 特大時計の部分更新ウィンドウ（日付は上、罫線は下なので帯で囲える）。
static const int SCLK_X = 0, SCLK_Y = 132, SCLK_W = 800, SCLK_H = 96;

static void loadLocation() {
  prefs.begin("paperx", true);
  g_lat   = prefs.getDouble("lat", 35.6895);
  g_lon   = prefs.getDouble("lon", 139.6917);
  g_place = prefs.getString("place", "Tokyo");
  prefs.end();
}
static void onSaveParams() {
  prefs.begin("paperx", false);
  if (p_lat   && strlen(p_lat->getValue()))   prefs.putDouble("lat", atof(p_lat->getValue()));
  if (p_lon   && strlen(p_lon->getValue()))   prefs.putDouble("lon", atof(p_lon->getValue()));
  if (p_place && strlen(p_place->getValue())) prefs.putString("place", p_place->getValue());
  prefs.end();
  Serial.println("location params saved to NVS");
}

// === u8g2 テキスト描画ヘルパ（baseline 指定） ===========================
static void uText(const uint8_t* f, int x, int yb, const char* s) {
  u8g2Fonts.setFont(f); u8g2Fonts.setCursor(x, yb); u8g2Fonts.print(s);
}
static int uWidth(const uint8_t* f, const char* s) {
  u8g2Fonts.setFont(f); return u8g2Fonts.getUTF8Width(s);
}
static void uRight(const uint8_t* f, int xr, int yb, const char* s) { uText(f, xr - uWidth(f, s), yb, s); }
static void uCenter(const uint8_t* f, int xc, int yb, const char* s) { uText(f, xc - uWidth(f, s) / 2, yb, s); }

static int utf8Len(const char* p) {
  unsigned char c = (unsigned char)*p;
  if (c < 0x80) return 1; if ((c >> 5) == 0x6) return 2;
  if ((c >> 4) == 0xE) return 3; if ((c >> 3) == 0x1E) return 4; return 1;
}
// UTF-8 を maxw 幅で最大2行に折り返して描画（日本語は任意位置で改行）。
static void drawWrapped(const uint8_t* f, int x, int y0, int maxw, int lh, const char* s) {
  char line[256] = {0}; int ll = 0, lineNo = 0;
  const char* p = s;
  while (*p && lineNo < 2) {
    int n = utf8Len(p);
    char cand[260]; memcpy(cand, line, ll); memcpy(cand + ll, p, n); cand[ll + n] = 0;
    if (uWidth(f, cand) > maxw && ll > 0) {
      uText(f, x, y0 + lineNo * lh, line);
      lineNo++; ll = 0; line[0] = 0;
      if (lineNo >= 2) break;
    }
    memcpy(line + ll, p, n); ll += n; line[ll] = 0; p += n;
  }
  if (ll > 0 && lineNo < 2) uText(f, x, y0 + lineNo * lh, line);
}

// UTF-8 を maxw 幅・最大 maxLines 行で折返し描画。あふれたら最終行末に「…」。描いた行数を返す。
static int drawReasonWrapped(const uint8_t* f, int x, int y0, int maxw, int lh,
                             const char* s, int maxLines) {
  char line[256] = {0}; int ll = 0, lineNo = 0;
  const char* p = s;
  while (*p) {
    int n = utf8Len(p);
    char cand[300]; memcpy(cand, line, ll); memcpy(cand + ll, p, n); cand[ll + n] = 0;
    if (uWidth(f, cand) > maxw && ll > 0) {
      if (lineNo >= maxLines - 1) {                 // 最終行があふれた → 「…」付けて終了
        char e[300]; snprintf(e, sizeof e, "%s…", line);
        while (uWidth(f, e) > maxw && ll > 0) {
          int back = 1; while (back < ll && ((unsigned char)line[ll - back] & 0xC0) == 0x80) back++;
          ll -= back; line[ll] = 0; snprintf(e, sizeof e, "%s…", line);
        }
        uText(f, x, y0 + lineNo * lh, e);
        return lineNo + 1;
      }
      uText(f, x, y0 + lineNo * lh, line);
      lineNo++; ll = 0; line[0] = 0;
    }
    memcpy(line + ll, p, n); ll += n; line[ll] = 0; p += n;
  }
  if (ll > 0) { uText(f, x, y0 + lineNo * lh, line); lineNo++; }
  return lineNo;
}

// === 天気アイコン（Lucide 1bit ビットマップ） =========================
// s>=80 をヒーロー(120px)、それ未満を予報行(44px)として中央寄せで描画。
// drawBitmap はセットされたビットだけ黒で打つので、白背景はそのまま残る。
static void drawWxIcon(WxIcon ic, int cx, int cy, float s) {
  int sz = (s >= 80) ? 120 : 44;
  const uint8_t* bmp = (sz == 120) ? WXI_120[ic] : WXI_44[ic];
  display.drawBitmap(cx - sz / 2, cy - sz / 2, bmp, sz, sz, GxEPD_BLACK);
}

// === 月齢と満ち欠け ====================================================
static double julianDay(int y, int m, int d) {
  if (m <= 2) { y -= 1; m += 12; }
  int a = y / 100, b = 2 - a + a / 4;
  return floor(365.25 * (y + 4716)) + floor(30.6001 * (m + 1)) + d + b - 1524.5;
}
static double moonAge(int y, int m, int d) {
  double age = fmod(julianDay(y, m, d) - 2451550.1, 29.530588853);
  return age < 0 ? age + 29.530588853 : age;
}
static void drawMoon(int cx, int cy, int r, double phase01) {
  // 明るい側を黒で塗る（満月=黒丸 / 新月=輪郭のみ）。北半球基準で満ちは右側。
  double ct = cos(2 * M_PI * phase01);
  for (int dy = -r; dy <= r; dy++) {
    double xw = sqrt((double)r * r - (double)dy * dy);
    int sx, ex;
    if (phase01 < 0.5) { sx = cx + (int)lround(xw * ct); ex = cx + (int)lround(xw); }
    else               { sx = cx - (int)lround(xw);      ex = cx - (int)lround(xw * ct); }
    if (ex >= sx) display.drawFastHLine(sx, cy + dy, ex - sx + 1, GxEPD_BLACK);
  }
  display.drawCircle(cx, cy, r, GxEPD_BLACK);
  display.drawCircle(cx, cy, r - 1, GxEPD_BLACK);
}
// 月齢(0〜29.53日) → 満ち欠けの呼び名。8相に区切る。
static const char* moonPhaseName(double age) {
  if (age < 1.85 || age >= 27.69) return "新月";
  if (age < 5.54)  return "三日月";
  if (age < 9.23)  return "上弦の月";
  if (age < 12.92) return "満ちゆく月";
  if (age < 16.61) return "満月";
  if (age < 20.30) return "欠けゆく月";
  if (age < 23.84) return "下弦の月";
  return "有明月";
}

static void hbar(int y, int t = 3) { display.fillRect(0, y, W, t, GxEPD_BLACK); }
static void vbar(int x, int y, int h, int t = 3) { display.fillRect(x, y, t, h, GxEPD_BLACK); }

static const char* WD[] = {"日", "月", "火", "水", "木", "金", "土"};

// === 各パーツ描画（全面描画から呼ぶ。バッファ前提・refreshは呼び出し側） ===
static void drawHeader(const struct tm& t) {
  char buf[64];
  snprintf(buf, sizeof buf, "%d月%d日", t.tm_mon + 1, t.tm_mday);
  uText(lsjp_xb30, 28, 48, buf);
  int dx = 28 + uWidth(lsjp_xb30, buf) + 12;
  snprintf(buf, sizeof buf, "%s曜日", WD[t.tm_wday]);
  uText(lsjp_r20, dx, 48, buf);
  snprintf(buf, sizeof buf, "%d:%02d", t.tm_hour, t.tm_min);
  uRight(pop_clock, W - 28, 58, buf);
  hbar(HD_BOT - 2);
}

// ヒーロー下の一言。雨の日は傘、降らない日は気温・天候に応じた注意を1行で。
// 上から順に評価し、最初に一致したものを返す（緊急度の高い順）。
static void heroAdvice(const Weather& w, char* buf, size_t n) {
  WxIcon ic = wxIconFor(w.codeNow);
  if (w.rainStartHour >= 0)        { snprintf(buf, n, "%d時から雨　傘を忘れずに", w.rainStartHour); return; }
  if (w.rainingNow)                { snprintf(buf, n, "今は雨　傘を忘れずに");        return; }
  if (ic == WX_THUNDER)            { snprintf(buf, n, "雷雨のおそれ　外出に注意");     return; }
  if (ic == WX_SNOW)               { snprintf(buf, n, "雪　足元に気をつけて");        return; }
  if (w.hiToday >= 35)             { snprintf(buf, n, "猛暑日　熱中症に警戒");        return; }
  if (w.hiToday >= 33)             { snprintf(buf, n, "厳しい暑さ　熱中症に注意");     return; }
  if (w.hiToday >= 30)             { snprintf(buf, n, "真夏日　熱中症に注意");        return; }
  if (w.hiToday >= 27)             { snprintf(buf, n, "暑い　水分をこまめに");        return; }
  if (w.loToday <= 0)              { snprintf(buf, n, "冷え込み厳しい　凍結に注意");   return; }
  if (w.hiToday <= 7)              { snprintf(buf, n, "厳しい寒さ　暖かい服装で");     return; }
  if (w.hiToday <= 12)             { snprintf(buf, n, "肌寒い　上着があると安心");     return; }
  if (w.hiToday - w.loToday >= 13) { snprintf(buf, n, "寒暖差大　体調に注意");        return; }
  if (ic == WX_FOG)                { snprintf(buf, n, "霧　見通しに注意");            return; }
  snprintf(buf, n, "穏やかな天気　お出かけ日和");
}

static void drawHero(const Weather& w) {
  char buf[64];
  if (!w.valid) {
    uText(lsjp_b24, 30, 180, "天気を取得できませんでした");
    return;
  }
  drawWxIcon(wxIconFor(w.codeNow), 92, 150, 120);
  snprintf(buf, sizeof buf, "%ld°", lround(w.tempNow));
  uText(pop_temp, 172, 178, buf);
  uText(lsjp_b24, 174, 214, wxConditionJP(w.codeNow));
  snprintf(buf, sizeof buf, "最高%d°　最低%d°", w.hiToday, w.loToday);
  uText(lsjp_b24, 30, 256, buf);
  for (int k = 0; k < 3; k++) display.drawRect(26 + k, 296 + k, 400 - 2 * k, 50 - 2 * k, GxEPD_BLACK);
  heroAdvice(w, buf, sizeof buf);
  uCenter(lsjp_b24, 226, 330, buf);
}

// フッタ左: 明日以降の5日間を横並び（各列 曜日 / アイコン / 最高°最低°）。
static void drawForecastRow(const Weather& w) {
  char buf[24];
  const int N = 5, colW = MOON_DIV / N;
  for (int i = 0; i < N; i++) {
    const WxDay& d = w.days[i];
    int cx = i * colW + colW / 2;
    if (i > 0) vbar(i * colW, MAIN_BOT + 12, Hh - MAIN_BOT - 24, 1);
    uCenter(lsjp_b24, cx, 388, WD[d.wday]);
    drawWxIcon(wxIconFor(d.code), cx, 426, 44);
    snprintf(buf, sizeof buf, "%d°/%d°", d.hi, d.lo);
    uCenter(lsjp_b24, cx, 472, buf);
  }
}

static void drawNewsSide(const News& nw) {
  const int NX = DIV_X + 24, NW = W - (DIV_X + 24) - 16;
  uText(lsjp_r13, NX, 94, "ニュース");
  display.fillRect(DIV_X + 3, 100, W - DIV_X - 3, 2, GxEPD_BLACK);
  if (!nw.valid || nw.count == 0) {
    uText(lsjp_b24, NX, 150, "ニュースを取得できませんでした");
    return;
  }
  int areaTop = 104, itemH = (MAIN_BOT - areaTop) / 3;
  for (int i = 0; i < nw.count && i < 3; i++) {
    int top = areaTop + i * itemH;
    if (i > 0) display.drawFastHLine(DIV_X + 3, top, W - DIV_X - 3, GxEPD_BLACK);
    char src[24]; snprintf(src, sizeof src, "NHK ・ %s", nw.items[i].hhmm);
    uText(lsjp_r13, NX, top + 20, src);
    drawWrapped(lsjp_news, NX, top + 46, NW, 24, nw.items[i].title.c_str());
  }
}

static void drawFooter(const struct tm& t) {
  drawForecastRow(g_w);
  vbar(MOON_DIV, MAIN_BOT, Hh - MAIN_BOT);
  double age = moonAge(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  int mcx = (MOON_DIV + W) / 2;
  drawMoon(mcx, 398, 30, age / 29.530588853);
  uCenter(lsjp_r20, mcx, 458, moonPhaseName(age));
}

// 全面描画（full refresh）。右カラムは常時ニュース、フッタは5日予報＋月。
static void drawFull(const struct tm& t) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    drawHeader(t);
    drawHero(g_w);
    vbar(DIV_X, HD_BOT, MAIN_BOT - HD_BOT);
    drawNewsSide(g_news);
    hbar(MAIN_BOT);
    drawFooter(t);
  } while (display.nextPage());
}

// === ページ2: マーケット & 運行 ========================================
// 前日比の上げ下げ三角（cx,cy 中心）。up=true で▲、false で▼。
static void drawArrow(int cx, int cy, bool up) {
  if (up) display.fillTriangle(cx, cy - 8, cx - 7, cy + 6, cx + 7, cy + 6, GxEPD_BLACK);
  else    display.fillTriangle(cx, cy + 8, cx - 7, cy - 6, cx + 7, cy - 6, GxEPD_BLACK);
}
// 平常運転のチェックマーク（x,y を小箱の左上とみなす）。太さ2pxで2本引く。
static void drawCheck(int x, int y) {
  for (int o = 0; o < 2; o++) {
    display.drawLine(x,      y + 10 + o, x + 6,  y + 16 + o, GxEPD_BLACK);
    display.drawLine(x + 6,  y + 16 + o, x + 18, y + 2 + o,  GxEPD_BLACK);
  }
}
// long を3桁区切りカンマ付き文字列に整形（"66934" → "66,934"）。
static void fmtComma(long v, char* out, size_t n) {
  char tmp[24];
  snprintf(tmp, sizeof tmp, "%ld", v < 0 ? -v : v);
  int len = (int)strlen(tmp), oi = 0;
  if (v < 0 && oi < (int)n - 1) out[oi++] = '-';
  for (int i = 0; i < len && oi < (int)n - 1; i++) {
    if (i > 0 && (len - i) % 3 == 0 && oi < (int)n - 1) out[oi++] = ',';
    out[oi++] = tmp[i];
  }
  out[oi] = 0;
}

// マーケット & 運行ページの中身（バッファ前提・refreshは呼び出し側）。
static void drawMarketPage(const struct tm& t) {
  char buf[64];
  drawHeader(t);   // メインページと同じヘッダ（日付・曜日・時刻）

  // --- 日経平均（左・縦積み: ラベル→数値→前日比） ---
  uText(lsjp_b24, 24, 104, "日経平均");
  if (g_mkt.nkValid) {
    fmtComma(g_mkt.nikkei, buf, sizeof buf);
    uText(pop_clock, 24, 165, buf);
    if (g_mkt.hasChange) {
      drawArrow(36, 190, g_mkt.nikkeiChg >= 0);
      snprintf(buf, sizeof buf, "%+ld  %+.2f%%", g_mkt.nikkeiChg, g_mkt.nikkeiPct);
      uText(lsjp_b24, 56, 198, buf);
    } else {
      uText(lsjp_r20, 24, 198, "前日比 集計中");
    }
  } else {
    uText(lsjp_r20, 24, 165, "取得できませんでした");
  }

  // --- 為替（右・2行: ラベル左／数値右・ベースライン揃え） ---
  vbar(470, 84, 122);
  uText(lsjp_b24, 500, 130, "USD / JPY");
  uText(lsjp_b24, 500, 192, "EUR / JPY");
  if (g_mkt.fxValid) {
    snprintf(buf, sizeof buf, "%.2f", g_mkt.usdjpy); uRight(pop_mid, W - 24, 130, buf);
    snprintf(buf, sizeof buf, "%.2f", g_mkt.eurjpy); uRight(pop_mid, W - 24, 192, buf);
  } else {
    uRight(lsjp_r20, W - 24, 130, "—");
    uRight(lsjp_r20, W - 24, 192, "—");
  }

  hbar(206);

  // --- 東京 主要路線：遅延を優先順に理由全文つきで（平常リストは出さない）---
  uText(lsjp_xb30, 24, 242, "東京 主要路線");
  if (!g_tr.valid) {
    uText(lsjp_r20, 24, 290, "運行情報を取得できませんでした");
    return;
  }
  if (g_tr.delayedCount == 0) {           // 全線平常のときだけそう表示
    drawCheck(24, 262);
    uText(lsjp_xb30, 58, 292, "全線 平常運転");
    return;
  }

  // 遅延あり: 優先順(京急→横須賀→京浜東北→山手→…)に、入るだけ理由全文を表示。
  const int badgeW = 64, badgeH = 26, rlh = 25, reasonX = 64, bottom = 470;
  int ry = 282, shown = 0;
  for (int p = 0; p < TRANSIT_N; p++) {
    int i = TRANSIT_PRIORITY[p];
    if (!g_tr.delayed[i]) continue;
    if (ry + 28 + rlh > bottom) break;    // 次の路線の名前＋理由1行も入らない
    display.fillRect(24, ry - 21, badgeW, badgeH, GxEPD_BLACK);
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    uCenter(lsjp_r20, 24 + badgeW / 2, ry - 2, "遅延");
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    uText(lsjp_b24, 24 + badgeW + 16, ry, TRANSIT_NAMES[i]);
    ry += 28;
    int maxLines = (bottom - ry) / rlh;
    if (maxLines > 2) maxLines = 2;
    if (maxLines < 1) maxLines = 1;
    int L = 1;
    if (g_tr.reason[i][0])
      L = drawReasonWrapped(lsjp_news, reasonX, ry, W - reasonX - 24, rlh, g_tr.reason[i], maxLines);
    ry += L * rlh + 14;
    shown++;
  }
  if (g_tr.delayedCount > shown) {
    snprintf(buf, sizeof buf, "ほか %d 路線が遅延", g_tr.delayedCount - shown);
    uText(lsjp_r20, 24, ry < 472 ? ry : 472, buf);   // 画面外に出さないようクランプ
  }
}

// マーケット & 運行ページの全面描画。
static void drawMarketFull(const struct tm& t) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    drawMarketPage(t);
  } while (display.nextPage());
}

// === ページ3: 状態（日付・時計メイン＋ファーム/Wi-Fi 情報）===============
static void drawBigClock(const struct tm& t) {
  char buf[8]; snprintf(buf, sizeof buf, "%d:%02d", t.tm_hour, t.tm_min);
  uCenter(pop_temp, W / 2, 212, buf);
}

static void drawStatusPage(const struct tm& t) {
  char buf[96];
  // --- 日付（中央上）・時計（特大・中央）---
  snprintf(buf, sizeof buf, "%d年%d月%d日 %s曜日",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, WD[t.tm_wday]);
  uCenter(lsjp_xb30, W / 2, 104, buf);
  drawBigClock(t);
  hbar(252);
  // --- ファームウェア ---
  uText(lsjp_b24, 40, 304, "ファームウェア");
  const char* st = !g_ota.lastCheckOk     ? "状態不明"
                 : g_ota.latest > FW_VERSION ? "アップデートあり"
                                             : "最新です";
  snprintf(buf, sizeof buf, "v%d ・ %s", FW_VERSION, st);
  uRight(lsjp_b24, W - 40, 304, buf);
  if (g_ota.checkedAt > 0) {
    struct tm c; localtime_r(&g_ota.checkedAt, &c);
    snprintf(buf, sizeof buf, "自動更新 有効 ・ 最終確認 %d:%02d", c.tm_hour, c.tm_min);
  } else {
    snprintf(buf, sizeof buf, "自動更新 有効");
  }
  uText(lsjp_r20, 40, 338, buf);
  // --- Wi-Fi ---
  uText(lsjp_b24, 40, 408, "Wi-Fi");
  String ssid = WiFi.SSID();
  uRight(lsjp_b24, W - 40, 408, ssid.length() ? ssid.c_str() : "未接続");
}

static void drawStatusFull(const struct tm& t) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    drawStatusPage(t);
  } while (display.nextPage());
}

// 現在のページを全面描画。
static void drawCurrentPage(const struct tm& t) {
  if (g_page == PAGE_MARKET)      drawMarketFull(t);
  else if (g_page == PAGE_STATUS) drawStatusFull(t);
  else                            drawFull(t);
}

// 時計だけ部分更新（毎分・高速）。
static void drawClockPartial(const struct tm& t) {
  char buf[8]; snprintf(buf, sizeof buf, "%d:%02d", t.tm_hour, t.tm_min);
  display.setPartialWindow(CLK_X, CLK_Y, CLK_W, CLK_H);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    uRight(pop_clock, W - 28, 58, buf);
  } while (display.nextPage());
}

// ページ3の特大時計だけ部分更新（毎分）。
static void drawStatusClockPartial(const struct tm& t) {
  display.setPartialWindow(SCLK_X, SCLK_Y, SCLK_W, SCLK_H);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    drawBigClock(t);
  } while (display.nextPage());
}

static void drawMessage(const char* title, const char* l1, const char* l2) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(0, 0, W, Hh, GxEPD_BLACK);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    uCenter(lsjp_xb30, W / 2, 170, title);
    if (l1 && l1[0]) uCenter(lsjp_r20, W / 2, 250, l1);
    if (l2 && l2[0]) uCenter(lsjp_r20, W / 2, 290, l2);
  } while (display.nextPage());
}
// Wi-Fi 参加用QR（WIFI:形式）を (cx,cy) 中心に scale px/セルで描く。白の余白も確保。
static void drawWifiQR(const char* text, int cx, int cy, int scale) {
  QRCode qr;
  uint8_t data[qrcode_getBufferSize(4)];
  qrcode_initText(&qr, data, 4, ECC_MEDIUM, text);   // v4=33セル, 30字程度を余裕で収容
  int dim = qr.size * scale;
  int x0 = cx - dim / 2, y0 = cy - dim / 2;
  int q = scale * 2;  // クワイエットゾーン（白枠）
  display.fillRect(x0 - q, y0 - q, dim + 2 * q, dim + 2 * q, GxEPD_WHITE);
  for (uint8_t y = 0; y < qr.size; y++)
    for (uint8_t x = 0; x < qr.size; x++)
      if (qrcode_getModule(&qr, x, y))
        display.fillRect(x0 + x * scale, y0 + y * scale, scale, scale, GxEPD_BLACK);
}

// 設定ポータル起動時の画面：左に手順、右に PaperX-Setup 参加QR。
static void drawSetupScreen() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(0, 0, W, Hh, GxEPD_BLACK);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    uText(lsjp_xb30, 56, 96,  "Wi-Fi 設定");
    uText(lsjp_r20,  56, 168, "① スマホのカメラで");
    uText(lsjp_r20,  56, 202, "   右のQRを読み取る");
    uText(lsjp_r20,  56, 268, "② PaperX-Setup に接続");
    uText(lsjp_r20,  56, 334, "③ 開いた画面でホーム");
    uText(lsjp_r20,  56, 368, "   Wi-Fi を設定");
    drawWifiQR("WIFI:S:PaperX-Setup;T:nopass;;", 596, 232, 7);
    uCenter(lsjp_r20, 596, 372, "PaperX-Setup");
  } while (display.nextPage());
}

static void onApMode(WiFiManager*) {
  Serial.println("config portal up -> show setup screen");
  drawSetupScreen();
}

// ページ3でKEY1長押し時：その場でWi-Fi設定ポータルを起動（onApMode が QR を描画）。
// ブロッキング。保存 or 180秒タイムアウトで復帰し、現在ページを再描画する。
static void startWifiPortal(const struct tm& t) {
  Serial.println("[wifi] KEY1 long-press -> start config portal");
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setTitle("PaperX");
  wm.setCustomHeadElement(PORTAL_HEAD);
  wm.setAPCallback(onApMode);
  wm.startConfigPortal(AP_NAME);
  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();   // タイムアウト時は元のAPへ復帰を試みる
  Serial.printf("[wifi] portal done, status=%d\n", WiFi.status());
  g_lastMin = -1;          // 時計を確実に再描画
  drawCurrentPage(t);
}

// KEY3: 保存済みWi-Fiを削除して再起動。再起動後は autoConnect が設定画面(QR)を出す。
static void eraseWifiAndRestart() {
  Serial.println("[wifi] KEY3 -> erase credentials & restart");
  drawMessage("PaperX", "Wi-Fi情報を削除しました", "再起動して設定画面を表示します");
  delay(1500);
  WiFiManager wm;
  wm.resetSettings();
  delay(300);
  ESP.restart();
}

// === 物理ボタン（debounce付き 立ち下がり検出） =========================
// KEY1=ページ送り / KEY2=Wi-Fi設定（QR）/ KEY3=Wi-Fi削除。
static const int BTN_PINS[3] = {PIN_HOME, PIN_NEXT, PIN_AUX};
static int g_btnLast[3] = {HIGH, HIGH, HIGH};
static unsigned long g_btnT[3] = {0, 0, 0};
// KEY3(Wi-Fi削除)は確認制：1回目で「初期化しますか？」、2回目の押下で実行。
static const unsigned long RESET_CONFIRM_MS = 10000;  // 確認の有効時間（無操作で取消）
static bool g_wifiResetPending = false;
static unsigned long g_wifiResetAt = 0;
// 押された(立ち下がり)ボタンの index(0=KEY1 / 1=KEY2 / 2=KEY3) を返す。なければ -1。
static int pollButtons() {
  int hit = -1;
  unsigned long now = millis();
  for (int i = 0; i < 3; i++) {
    int s = digitalRead(BTN_PINS[i]);
    if (s != g_btnLast[i] && now - g_btnT[i] > 30) {  // 30ms debounce
      g_btnT[i] = now;
      g_btnLast[i] = s;
      if (s == LOW) {  // active-low: LOW=押下
        Serial.printf("[btn] KEY%d pressed\n", i + 1);
        hit = i;
      }
    }
  }
  return hit;
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("\n=== PaperX: news + 5-day forecast ===");

  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR, HIGH);
  delay(100);
  pinMode(BTN_BOOT, INPUT_PULLUP);
  for (int i = 0; i < 3; i++) pinMode(BTN_PINS[i], INPUT_PULLUP);

  display.init(115200, true, 50, false);
  display.setRotation(0);
  u8g2Fonts.begin(display);

  loadLocation();
  Serial.printf("location: %s (%.4f, %.4f)\n", g_place.c_str(), g_lat, g_lon);

  const bool resetCfg = (digitalRead(BTN_BOOT) == LOW);
  char latbuf[24], lonbuf[24];
  snprintf(latbuf, sizeof latbuf, "%.4f", g_lat);
  snprintf(lonbuf, sizeof lonbuf, "%.4f", g_lon);
  p_lat   = new WiFiManagerParameter("lat",   "緯度（例: 35.6895）",  latbuf, 16);
  p_lon   = new WiFiManagerParameter("lon",   "経度（例: 139.6917）", lonbuf, 16);
  p_place = new WiFiManagerParameter("place", "地点名（任意）",        g_place.c_str(), 31);

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setTitle("PaperX");
  wm.setCustomHeadElement(PORTAL_HEAD);
  wm.setAPCallback(onApMode);
  wm.setSaveParamsCallback(onSaveParams);
  wm.addParameter(p_lat); wm.addParameter(p_lon); wm.addParameter(p_place);
  if (resetCfg) { Serial.println("BOOT held -> erase saved WiFi"); wm.resetSettings(); }

  drawMessage("PaperX", "Wi-Fiに接続中…", "");
  if (!wm.autoConnect(AP_NAME)) {
    Serial.println("WiFi connect failed -> offline");
    drawMessage("PaperX", "オフライン", "電源を入れ直すと設定画面が出ます");
    display.hibernate();
    return;
  }
  Serial.printf("WiFi connected: IP=%s RSSI=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  loadLocation();

  configTzTime(TZ_JST, NTP1, NTP2);
  struct tm t;
  bool synced = false;
  for (int i = 0; i < 20; i++) if (getLocalTime(&t, 500)) { synced = true; break; }
  if (!synced) {
    Serial.println("NTP sync failed");
    drawMessage("PaperX", "時刻同期に失敗", "(Wi-Fiは接続済み)");
    display.hibernate();
    return;
  }
  Serial.printf("NTP: %04d-%02d-%02d %02d:%02d wday=%d\n",
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_wday);

  // 起動時に最新ファームを確認（あれば更新→再起動。失敗時は drawFull が画面を戻す）
  otaCheckAndUpdate(drawMessage);
  g_lastOtaMs = millis();

  bool gw = fetchWeather(g_lat, g_lon, t.tm_hour, t.tm_wday, g_w);
  bool gn = fetchNews(g_news);
  Serial.printf("weather=%s temp=%.1f code=%d | news=%s count=%d\n",
                gw ? "OK" : "FAIL", g_w.tempNow, g_w.codeNow, gn ? "OK" : "FAIL", g_news.count);

  bool gfx = fetchFx(g_mkt);
  bool gnk = fetchNikkei(g_mkt);
  bool gtr = fetchTransit(g_tr);
  Serial.printf("fx=%s usdjpy=%.2f eurjpy=%.2f | nikkei=%s %ld chg=%s%+ld(%+.2f%%) | transit=%s delayed=%d other=%d\n",
                gfx ? "OK" : "FAIL", g_mkt.usdjpy, g_mkt.eurjpy,
                gnk ? "OK" : "FAIL", g_mkt.nikkei,
                g_mkt.hasChange ? "" : "集計中", g_mkt.nikkeiChg, g_mkt.nikkeiPct,
                gtr ? "OK" : "FAIL", g_tr.delayedCount, g_tr.otherCount);

  drawFull(t);
  g_lastMin = t.tm_min;
  g_lastRefreshMs = millis();
  Serial.println("setup done -> entering loop (powered mode)");
}

void loop() {
  struct tm t;
  if (!getLocalTime(&t, 50)) { delay(500); return; }

  // ボタン: KEY1=ページ送り（1→2→3→1）/ KEY2=Wi-Fi設定（QR）/ KEY3=Wi-Fi削除（確認制）。
  int btn = pollButtons();

  // KEY3 の確認待ち中：もう一度KEY3で実行、ほかのボタン or タイムアウトで取消。
  if (g_wifiResetPending) {
    if (btn == 2) {
      eraseWifiAndRestart();                        // 削除→再起動（戻らない）
    } else if (btn >= 0 || millis() - g_wifiResetAt >= RESET_CONFIRM_MS) {
      g_wifiResetPending = false;
      Serial.println("[wifi] reset canceled");
      drawCurrentPage(t);
      g_lastMin = t.tm_min;
    }
    delay(40);
    return;
  }

  if (btn == 0) {                                   // KEY1 → 次のページへ
    g_page = (g_page + 1) % PAGE_COUNT;
    drawCurrentPage(t);
    Serial.printf("[page] -> %d\n", g_page);
  } else if (btn == 1) {                            // KEY2 → Wi-Fi設定（QR画面）
    startWifiPortal(t);
  } else if (btn == 2) {                            // KEY3 → まず確認を表示（1回目）
    g_wifiResetPending = true;
    g_wifiResetAt = millis();
    drawMessage("初期化しますか？", "もう一度同じボタンを押すと削除", "ほかのボタンで取消");
    Serial.println("[wifi] reset pending (press same button again)");
  }

  // 毎分: 時計だけ部分更新（ホーム表示中のみ。時計はホームにしか無い）
  if (t.tm_min != g_lastMin) {
    if (g_page == PAGE_HOME) {
      drawClockPartial(t);
      Serial.printf("[clock] %02d:%02d (partial)\n", t.tm_hour, t.tm_min);
    } else if (g_page == PAGE_STATUS) {
      drawStatusClockPartial(t);
      Serial.printf("[clock] %02d:%02d (status partial)\n", t.tm_hour, t.tm_min);
    }
    g_lastMin = t.tm_min;
  }

  // 10分: データ再取得（天気・ニュース・マーケット・運行）＋現在ページを全面更新
  if (millis() - g_lastRefreshMs >= REFRESH_MS) {
    if (WiFi.status() == WL_CONNECTED) {
      fetchWeather(g_lat, g_lon, t.tm_hour, t.tm_wday, g_w);
      fetchNews(g_news);
      fetchFx(g_mkt);
      fetchNikkei(g_mkt);
      fetchTransit(g_tr);
    }
    getLocalTime(&t, 50);
    drawCurrentPage(t);
    g_lastRefreshMs = millis();
    g_lastMin = t.tm_min;
    Serial.printf("[refresh] %02d:%02d (full, page=%d)\n", t.tm_hour, t.tm_min, g_page);
  }

  // 6時間ごと: OTA 更新確認（更新あれば再起動。失敗時のみ true→現在ページを描き直す）
  if (millis() - g_lastOtaMs >= OTA_CHECK_MS) {
    if (WiFi.status() == WL_CONNECTED && otaCheckAndUpdate(drawMessage)) {
      getLocalTime(&t, 50);
      drawCurrentPage(t);
      g_lastMin = t.tm_min;
    }
    g_lastOtaMs = millis();
  }

  delay(40);
}
