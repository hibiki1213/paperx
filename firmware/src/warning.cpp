// warning.cpp — 気象庁 防災情報JSON（2026-05-28 新運用 / r8）から大田区の警報・注意報を取得。
//   新エンドポイント: https://www.jma.go.jp/bosai/warning/data/r8/130000.json（東京都）
//     旧 /bosai/warning/data/warning/130000.json は刷新時(2026-05-28)に凍結されたため使わない。
//   構造: トップは「製品(大雨/土砂災害/暴風/波浪/雷…)」ごとの配列。各製品の
//     warning.class20Items[] が市区町村単位（areaCode "1311100"=大田区）で、
//     kinds[] が {code, status}。status が「解除」以外を発表中とみなす。
//   コードは旧来(02〜38)を維持しつつ、2026新設の危険警報(L4)コード(43大雨/49土砂災害等)が追加。
//     未知コード(高潮/氾濫の危険警報など未観測分)は「気象警報」(severity=4)でフォールバック。
#include "warning.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char* AREA_OTA = "1311100";  // 大田区（東京都 130000 配下）

// severity: 5=特別警報 / 4=危険警報(2026新) / 3=警報 / 2=注意報
struct WCode { const char* code; const char* name; uint8_t sev; };
static const WCode WCODES[] = {
  // 特別警報（レベル5相当）
  {"33","大雨特別警報",5},{"32","暴風雪特別警報",5},{"35","暴風特別警報",5},
  {"36","大雪特別警報",5},{"37","波浪特別警報",5},{"38","高潮特別警報",5},
  // 危険警報（レベル4相当・2026新設）
  {"43","大雨危険警報",4},{"49","土砂災害危険警報",4},
  // 警報（レベル3相当）
  {"03","大雨警報",3},{"29","土砂災害警報",3},{"05","暴風警報",3},{"02","暴風雪警報",3},
  {"06","大雪警報",3},{"07","波浪警報",3},{"08","高潮警報",3},{"04","洪水警報",3},
  // 注意報（レベル2相当 / 警戒レベル対象外）
  {"10","大雨注意報",2},{"19","高潮注意報",2},{"12","大雪注意報",2},{"13","風雪注意報",2},
  {"14","雷注意報",2},{"15","強風注意報",2},{"16","波浪注意報",2},{"17","融雪注意報",2},
  {"18","洪水注意報",2},{"20","濃霧注意報",2},{"21","乾燥注意報",2},{"22","なだれ注意報",2},
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
  if (!http.begin(client, "https://www.jma.go.jp/bosai/warning/data/r8/130000.json")) return false;
  int st = http.GET();
  if (st != 200) { Serial.printf("[warn] HTTP %d\n", st); http.end(); return false; }

  // 各製品の class20Items の areaCode と kinds(code,status) だけ残す（class10等は捨てる）。
  JsonDocument filter;
  filter[0]["warning"]["class20Items"][0]["areaCode"]           = true;
  filter[0]["warning"]["class20Items"][0]["kinds"][0]["code"]   = true;
  filter[0]["warning"]["class20Items"][0]["kinds"][0]["status"] = true;

  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, http.getStream(),
                      DeserializationOption::Filter(filter),
                      DeserializationOption::NestingLimit(20));
  http.end();
  if (err) { Serial.printf("[warn] parse err %s\n", err.c_str()); return false; }
  out.valid = true;

  char names[10][28]; int sevs[10]; int nn = 0;
  for (JsonObject item : doc.as<JsonArray>()) {                          // 製品ごと
    for (JsonObject ar : item["warning"]["class20Items"].as<JsonArray>()) {
      if (strcmp(ar["areaCode"] | "", AREA_OTA)) continue;
      for (JsonObject k : ar["kinds"].as<JsonArray>()) {
        const char* code   = k["code"]   | "";
        const char* status = k["status"] | "";
        if (!code[0]) continue;
        if (strstr(status, "解除")) continue;
        const WCode* wc = lookup(code);
        const char* nm = wc ? wc->name : "気象警報";
        int sev = wc ? wc->sev : 4;                 // 未知コード(危険警報等)は警戒扱い
        bool dup = false;
        for (int j = 0; j < nn; j++) if (!strcmp(names[j], nm)) { dup = true; break; }
        if (!dup && nn < 10) { strlcpy(names[nn], nm, sizeof names[nn]); sevs[nn] = sev; nn++; }
        if (sev > out.severity) out.severity = sev;
      }
    }
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
