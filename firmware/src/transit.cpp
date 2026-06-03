// transit.cpp — 関東主要路線の運行情報。
//   ① nTool(ntool.online/data/train_all.json, 無認証)で「どの路線が支障中か」を判定。
//      data["4"]=関東。railName 部分一致で主要15路線を拾い、status で支障判定。
//      理由(info)は16字で切詰め済 → フォールバック用。
//   ② 表示優先の上位 YAHOO_FETCH_MAX 路線だけ Yahoo!路線情報の個別ページから
//      理由「全文」を取得して上書き（og:description メタを strstr で抽出）。
//      取得/解析に失敗した路線は nTool の16字のまま（堅牢化）。
//   ※ いずれも Yahoo 由来の非公式取得。個人利用前提。消えても valid=false 等に縮退するだけ。
#include "transit.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 画面表示名（gen_wx_fonts.py がここを走査して字形を埋め込む）
const char* const TRANSIT_NAMES[TRANSIT_N] = {
  "山手線", "中央線快速", "京浜東北線", "総武線各停", "総武線快速",
  "東海道線", "横須賀線", "埼京線", "常磐線快速", "京葉線",
  "メトロ丸ノ内線", "メトロ東西線", "都営大江戸線", "小田急線",
  "京急線",
};

// nTool の railName に含まれていれば当該路線とみなす部分文字列（UTF-8バイト一致）。
static const char* const MATCH[TRANSIT_N] = {
  "山手線", "中央線(快速)", "京浜東北", "中央総武線", "総武線(快速)",
  "東海道本線", "横須賀線", "埼京", "常磐線(快速)", "京葉線",
  "丸ノ内", "メトロ東西線", "大江戸", "小田急小田原",
  "京急本線",
};

// Yahoo!路線情報の路線コード（/diainfo/<code>/0）。TRANSIT_NAMES と同順。
static const int YAHOO_CODE[TRANSIT_N] = {
  21, 38, 22, 40, 61,
  27, 29, 50, 57, 69,
  133, 135, 131, 109,
  120,
};

// 表示・全文取得の優先順（京急→横須賀→京浜東北→山手→残り）。値は TRANSIT_NAMES の添字。
const int TRANSIT_PRIORITY[TRANSIT_N] = {
  14, 6, 2, 0,
  1, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13,
};

// 全文を取りに行く最大路線数（表示しうる上限。これ以上は16字のまま=「ほかN路線」へ）。
static const int YAHOO_FETCH_MAX = 4;

// Yahoo!路線情報の個別ページから理由全文を取得して out に格納（成功で true）。
// og:description メタは <head> 先頭(~1.7KB)に出るので先頭16KBだけ読めば足りる。
static bool fetchYahooReason(int code, char* out, size_t n) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  http.setUserAgent("Mozilla/5.0 (compatible; PaperX/1.0)");
  char url[64];
  snprintf(url, sizeof url, "https://transit.yahoo.co.jp/diainfo/%d/0", code);
  if (!http.begin(client, url)) return false;
  int status = http.GET();
  if (status != 200) { Serial.printf("[tr] yahoo %d HTTP %d\n", code, status); http.end(); return false; }

  // 先頭だけストリーム読みして og:description を探す（最大16KB）。
  WiFiClient* st = http.getStreamPtr();
  static const char* MARK = "og:description\" content=\"";
  const int MARKLEN = (int)strlen(MARK);   // =25。値はこの直後から始まる
  String buf;
  buf.reserve(17000);
  const size_t LIMIT = 16384;
  unsigned long t0 = millis();
  int hit = -1;
  while (http.connected() && buf.length() < LIMIT && millis() - t0 < 8000) {
    if (st->available()) {
      char c = (char)st->read();
      buf += c;
      if ((int)buf.length() >= MARKLEN) {
        hit = buf.indexOf(MARK);
        // マーカーの先＝値の開始(hit+MARKLEN)以降に終端 " が出たら読了
        if (hit >= 0 && buf.indexOf('"', hit + MARKLEN) >= 0) break;
      }
    } else {
      delay(2);
    }
  }
  http.end();
  if (hit < 0) hit = buf.indexOf(MARK);
  if (hit < 0) return false;
  int s = hit + MARKLEN;
  int e = buf.indexOf('"', s);
  if (e < 0) return false;
  String desc = buf.substring(s, e);
  int paren = desc.indexOf("（");          // 末尾の「（…時点の情報です…）」を落とす
  if (paren > 0) desc = desc.substring(0, paren);
  desc.trim();
  if (desc.length() == 0 || desc.indexOf("平常") >= 0) return false;  // 平常表示はフォールバック
  strlcpy(out, desc.c_str(), n);
  return true;
}

bool fetchTransit(Transit& out) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  if (!http.begin(client, "https://ntool.online/data/train_all.json")) return false;
  int status = http.GET();
  if (status != 200) { Serial.printf("[tr] HTTP %d\n", status); http.end(); return false; }

  // 関東(data["4"])の railName/status/info だけを残すフィルタで省メモリ解析。
  JsonDocument filter;
  filter["data"]["4"][0]["railName"] = true;
  filter["data"]["4"][0]["status"]   = true;
  filter["data"]["4"][0]["info"]     = true;

  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { Serial.printf("[tr] parse err: %s\n", err.c_str()); return false; }

  JsonArray arr = doc["data"]["4"].as<JsonArray>();
  if (arr.isNull()) { Serial.println("[tr] no kanto data"); return false; }

  for (int i = 0; i < TRANSIT_N; i++) { out.delayed[i] = false; out.reason[i][0] = 0; }
  out.delayedCount = 0;
  out.otherCount = 0;

  for (JsonObject o : arr) {
    const char* name = o["railName"] | "";
    const char* stt  = o["status"]   | "";
    const char* info = o["info"]     | "";
    if (!name[0]) continue;
    bool problem = stt[0] && !strstr(stt, "平常") && !strstr(stt, "計画");
    bool matched = false;
    for (int i = 0; i < TRANSIT_N; i++) {
      if (strstr(name, MATCH[i])) {
        matched = true;
        if (problem && !out.delayed[i]) {
          out.delayed[i] = true; out.delayedCount++;
          strlcpy(out.reason[i], info, sizeof out.reason[i]);  // まず nTool の16字（フォールバック）
        }
        break;
      }
    }
    if (problem && !matched) out.otherCount++;
  }

  // 優先上位の遅延路線だけ Yahoo から全文を取得して上書き（失敗時は16字のまま）。
  int fetched = 0;
  for (int p = 0; p < TRANSIT_N && fetched < YAHOO_FETCH_MAX; p++) {
    int i = TRANSIT_PRIORITY[p];
    if (!out.delayed[i]) continue;
    char full[256];
    if (fetchYahooReason(YAHOO_CODE[i], full, sizeof full)) {
      strlcpy(out.reason[i], full, sizeof out.reason[i]);
      Serial.printf("[tr] yahoo %s: %s\n", TRANSIT_NAMES[i], out.reason[i]);
    }
    fetched++;
  }

  out.valid = true;
  return true;
}
