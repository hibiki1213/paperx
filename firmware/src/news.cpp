// news.cpp — NHK 主要ニュース RSS から見出し上位3件を取得（HTTPS・APIキー不要）。
#include "news.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ctype.h>

static const char* NHK_URL = "https://www3.nhk.or.jp/rss/news/cat0.xml";

// 最小限の XML エンティティ復号（&amp; &lt; &gt; &quot; &apos; &#NNN; &#xHH;）。
static String xmlDecode(const String& s) {
  String o; o.reserve(s.length());
  int i = 0, n = s.length();
  while (i < n) {
    if (s[i] == '&') {
      int sc = s.indexOf(';', i);
      if (sc > i && sc - i <= 10) {
        String e = s.substring(i + 1, sc);
        if      (e == "amp")  o += '&';
        else if (e == "lt")   o += '<';
        else if (e == "gt")   o += '>';
        else if (e == "quot") o += '"';
        else if (e == "apos") o += '\'';
        else if (e.length() > 1 && e[0] == '#') {
          long cp = (e[1] == 'x' || e[1] == 'X') ? strtol(e.c_str() + 2, nullptr, 16)
                                                  : atol(e.c_str() + 1);
          if (cp < 0x80) o += (char)cp;
          else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); }
          else { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
        } else { o += '&'; o += e; o += ';'; }
        i = sc + 1; continue;
      }
    }
    o += s[i++];
  }
  return o;
}

// <tag>…</tag> の中身を返す（最初の出現）。
static String tagText(const String& src, const char* tag) {
  String open = String("<") + tag + ">";
  String close = String("</") + tag + ">";
  int a = src.indexOf(open); if (a < 0) return "";
  a += open.length();
  int b = src.indexOf(close, a); if (b < 0) return "";
  return src.substring(a, b);
}

// 空白区切りの n 番目トークン（0始まり）。
static String nthToken(const String& s, int idx) {
  int start = 0, n = s.length();
  for (int i = 0; i <= n; i++) {
    if (i == n || s[i] == ' ') {
      if (idx == 0) return s.substring(start, i);
      idx--; start = i + 1;
    }
  }
  return "";
}

bool fetchNews(News& out) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  if (!http.begin(client, NHK_URL)) return false;
  http.addHeader("User-Agent", "Mozilla/5.0 (PaperX)");
  int st = http.GET();
  if (st != 200) { Serial.printf("[news] HTTP %d\n", st); http.end(); return false; }
  String xml = http.getString();
  http.end();

  News tmp;  // 全件パースできてから out へ反映（途中失敗で直近値を壊さない）
  int pos = 0;
  while (tmp.count < 3) {
    int it = xml.indexOf("<item>", pos);            if (it < 0) break;
    int ite = xml.indexOf("</item>", it);           if (ite < 0) break;
    String item = xml.substring(it, ite);
    String title = xmlDecode(tagText(item, "title")); title.trim();
    if (title.length()) {
      NewsItem& ni = tmp.items[tmp.count];
      ni.title = title;
      String pub = tagText(item, "pubDate"); pub.trim();   // "Mon, 01 Jun 2026 16:20:34 +0900"
      String tm = nthToken(pub, 4);                        // "16:20:34"
      if (tm.length() >= 5) { strncpy(ni.hhmm, tm.c_str(), 5); ni.hhmm[5] = 0; }
      tmp.count++;
    }
    pos = ite + 7;
  }
  if (tmp.count == 0) return false;
  tmp.valid = true;
  out = tmp;
  return true;
}
