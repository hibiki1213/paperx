// warning.cpp — 気象庁 防災情報JSON から大田区(1311100)の警報・注意報を取得。
//   areaTypes[].areas[].code が "1311100" の warnings[]（{code,status}）を読む。
//   status が「解除」以外で code を持つものを発表中とみなす（「発表警報・注意報はなし」は code 無し）。
//   ※コード→名称表は確認済み。2026-05-28 新設の危険警報(L4)等の新コードは未知のため
//     「気象警報」(severity=4) のフォールバックで安全に表示する（実データ確認後に拡充）。
#include "warning.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char* AREA_OTA = "1311100";  // 大田区（東京都 130000 配下）

// severity: 5=特別警報 / 3=警報 / 2=注意報（危険警報L4は新コード=未知側で扱う）
struct WCode { const char* code; const char* name; uint8_t sev; };
static const WCode WCODES[] = {
  {"33","大雨特別警報",5},{"32","暴風雪特別警報",5},{"35","暴風特別警報",5},
  {"36","大雪特別警報",5},{"37","波浪特別警報",5},{"38","高潮特別警報",5},
  {"03","大雨警報",3},{"04","洪水警報",3},{"05","暴風警報",3},{"02","暴風雪警報",3},
  {"06","大雪警報",3},{"07","波浪警報",3},{"08","高潮警報",3},
  {"10","大雨注意報",2},{"18","洪水注意報",2},{"19","高潮注意報",2},{"12","大雪注意報",2},
  {"13","風雪注意報",2},{"14","雷注意報",2},{"15","強風注意報",2},{"16","波浪注意報",2},
  {"17","融雪注意報",2},{"20","濃霧注意報",2},{"21","乾燥注意報",2},{"22","なだれ注意報",2},
  {"23","低温注意報",2},{"24","霜注意報",2},{"25","着氷注意報",2},{"26","着雪注意報",2},
  {"27","その他の注意報",2},
};
static const WCode* lookup(const char* code) {
  for (const WCode& w : WCODES) if (!strcmp(w.code, code)) return &w;
  return nullptr;
}

bool fetchWarning(WeatherWarning& out) {
  out = WeatherWarning();
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; http.setConnectTimeout(8000); http.setTimeout(8000);
  if (!http.begin(client, "https://www.jma.go.jp/bosai/warning/data/warning/130000.json")) return false;
  int st = http.GET();
  if (st != 200) { Serial.printf("[warn] HTTP %d\n", st); http.end(); return false; }

  // 大田区の warnings[] だけ残すフィルタ（全エリア舐めず省メモリ）。
  JsonDocument filter;
  filter["areaTypes"][0]["areas"][0]["code"] = true;
  filter["areaTypes"][0]["areas"][0]["warnings"][0]["code"]   = true;
  filter["areaTypes"][0]["areas"][0]["warnings"][0]["status"] = true;

  JsonDocument doc;
  // timeSeries が深さ14あり、フィルタ除外しても走査で既定の深さ上限(10)に当たるため引き上げる。
  DeserializationError err =
      deserializeJson(doc, http.getStream(),
                      DeserializationOption::Filter(filter),
                      DeserializationOption::NestingLimit(16));
  http.end();
  if (err) { Serial.printf("[warn] parse err %s\n", err.c_str()); return false; }
  out.valid = true;

  // 1311100 の warnings を探す
  JsonArray warns;
  for (JsonObject at : doc["areaTypes"].as<JsonArray>()) {
    for (JsonObject ar : at["areas"].as<JsonArray>()) {
      if (!strcmp(ar["code"] | "", AREA_OTA)) { warns = ar["warnings"].as<JsonArray>(); break; }
    }
    if (!warns.isNull()) break;
  }
  if (warns.isNull()) return true;  // 見つからない=警報なし扱い

  char names[8][28]; int sevs[8]; int nn = 0;
  for (JsonObject w : warns) {
    const char* code   = w["code"]   | "";
    const char* status = w["status"] | "";
    if (!code[0]) continue;                 // 「発表警報・注意報はなし」エントリ
    if (strstr(status, "解除")) continue;   // 解除は無視（発表/継続のみ）
    const WCode* wc = lookup(code);
    const char* nm = wc ? wc->name : "気象警報";
    int sev = wc ? wc->sev : 4;             // 未知コード(危険警報等)は警戒扱い
    if (nn < 8) { strlcpy(names[nn], nm, sizeof names[nn]); sevs[nn] = sev; nn++; }
    if (sev > out.severity) out.severity = sev;
  }
  if (nn == 0) return true;
  out.active = true;
  int top = 0;
  for (int i = 1; i < nn; i++) if (sevs[i] > sevs[top]) top = i;
  if (nn == 1) snprintf(out.headline, sizeof out.headline, "%s 発表中", names[top]);
  else         snprintf(out.headline, sizeof out.headline, "%s ほか%d件", names[top], nn - 1);
  Serial.printf("[warn] active=%d sev=%d %s\n", out.active, out.severity, out.headline);
  return true;
}
