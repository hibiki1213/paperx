// ota.cpp — GitHub Releases から最新ファームを取得して自己更新（HTTP-pull OTA）。
//   1) GitHub API の releases/latest で tag_name(="v<N>") を取り、FW_VERSION と比較。
//   2) 新しければ releases/latest/download/firmware.bin を httpUpdate で書込み→再起動。
//   github.com → objects.githubusercontent.com の 302 を辿るため STRICT_FOLLOW_REDIRECTS 必須。
//   ※ 無認証なので GitHub API は 60req/h 制限。起動時＋数時間おきの確認なら十分収まる。
#include "ota.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

static const char* OTA_OWNER = "hibiki1213";
static const char* OTA_REPO  = "paperx";

// tag_name 先頭の 'v' 等を飛ばし、最初の数字連続だけを整数化（整数バージョン運用）。
static long parseTagVersion(const char* tag) {
  if (!tag) return -1;
  const char* p = tag;
  while (*p && (*p < '0' || *p > '9')) p++;
  long n = 0; bool any = false;
  while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); p++; any = true; }
  return any ? n : -1;
}

// 最新リリースのバージョン番号を取得（失敗時 -1）。
static long fetchLatestVersion() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  char url[128];
  snprintf(url, sizeof url, "https://api.github.com/repos/%s/%s/releases/latest", OTA_OWNER, OTA_REPO);
  if (!http.begin(client, url)) return -1;
  http.addHeader("User-Agent", "PaperX-OTA");          // GitHub API は UA 必須（無いと 403）
  http.addHeader("Accept", "application/vnd.github+json");
  int status = http.GET();
  if (status != 200) { Serial.printf("[ota] releases/latest HTTP %d\n", status); http.end(); return -1; }

  // tag_name だけ取れれば十分。フィルタで省メモリ解析。
  JsonDocument filter; filter["tag_name"] = true;
  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { Serial.printf("[ota] json err: %s\n", err.c_str()); return -1; }

  const char* tag = doc["tag_name"] | "";
  long v = parseTagVersion(tag);
  Serial.printf("[ota] latest tag=%s -> v=%ld (current=%d)\n", tag, v, FW_VERSION);
  return v;
}

bool otaCheckAndUpdate(OtaStatusFn status) {
  if (WiFi.status() != WL_CONNECTED) return false;

  long latest = fetchLatestVersion();
  if (latest <= FW_VERSION) return false;   // 最新 or 取得失敗 → 何もしない

  Serial.printf("[ota] updating %d -> %ld\n", FW_VERSION, latest);
  if (status) status("PaperX", "アップデート中…", "電源を切らないでください");

  WiFiClientSecure client;
  client.setInsecure();
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);   // 成功時はここから戻らず再起動する

  char url[160];
  snprintf(url, sizeof url, "https://github.com/%s/%s/releases/latest/download/firmware.bin",
           OTA_OWNER, OTA_REPO);
  t_httpUpdate_return ret = httpUpdate.update(client, url);

  // ここに到達した時点で失敗（成功なら再起動済み）。
  Serial.printf("[ota] FAILED (%d): %s\n",
                httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
  if (status) status("PaperX", "アップデートに失敗", "通常表示に戻ります");
  return true;   // 画面を触ったので呼び出し側に再描画させる
}
