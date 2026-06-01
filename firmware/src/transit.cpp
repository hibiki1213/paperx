// transit.cpp — ntool.online/data/train_all.json（無認証）から関東の運行情報を取得。
//   このAPIは全国の路線を {railName, companyName, status, info} の配列で返す。
//   data["4"] が関東。主要12路線を railName の部分一致で拾い、status を見て遅延判定する。
//   status 例: 平常運転 / 列車遅延 / 運転見合わせ / 運転状況 / 運転計画 / その他。
//     → "平常"系・"運転計画" 以外（遅延/見合わせ/状況/その他）を支障ありとみなす。
//   ※ Yahoo由来の二次配信・個人運営。公式には提供終了告知あり(惰性稼働中)。
//     消えたら valid=false に縮退するだけで端末は壊れない。ODPTのキーが下りたら差し替え。
#include "transit.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 画面表示名（gen_wx_fonts.py がここを走査して字形を埋め込む）
const char* const TRANSIT_NAMES[TRANSIT_N] = {
  "山手線", "中央線快速", "京浜東北線", "総武線各停",
  "東海道線", "埼京線", "常磐線快速", "京葉線",
  "メトロ丸ノ内線", "メトロ東西線", "都営大江戸線", "小田急線",
  "京急線",
};

// nTool の railName に含まれていれば当該路線とみなす部分文字列（UTF-8バイト一致）。
// 実データ(area"4")で各文字列が一意に1路線へ解決することを確認済み。
// "京急本線" は京急逗子/空港/久里浜線(="京急"を含むが"京急本線"は含まない)と衝突しない。
static const char* const MATCH[TRANSIT_N] = {
  "山手線", "中央線(快速)", "京浜東北", "中央総武線",
  "東海道本線", "埼京", "常磐線(快速)", "京葉線",
  "丸ノ内", "メトロ東西線", "大江戸", "小田急小田原",
  "京急本線",
};

bool fetchTransit(Transit& out) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  if (!http.begin(client, "https://ntool.online/data/train_all.json")) return false;
  int status = http.GET();
  if (status != 200) { Serial.printf("[tr] HTTP %d\n", status); http.end(); return false; }

  // 関東(data["4"])の railName/status だけを残すフィルタで、全国84KBを丸読みせず省メモリ解析。
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
    const char* st   = o["status"]   | "";
    const char* info = o["info"]     | "";
    if (!name[0]) continue;
    // "平常運転"(=平常)・"運転計画"(=計画・予定) 以外は支障ありとして扱う。
    bool problem = st[0] && !strstr(st, "平常") && !strstr(st, "計画");
    bool matched = false;
    for (int i = 0; i < TRANSIT_N; i++) {
      if (strstr(name, MATCH[i])) {
        matched = true;
        if (problem && !out.delayed[i]) {
          out.delayed[i] = true; out.delayedCount++;
          strlcpy(out.reason[i], info, sizeof out.reason[i]);  // nTool が16字で切詰済
        }
        break;
      }
    }
    if (problem && !matched) out.otherCount++;  // 固定路線以外で支障中の関東路線数
  }
  out.valid = true;
  return true;
}
