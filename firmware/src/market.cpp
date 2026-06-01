// market.cpp — 為替と日経平均を無認証APIから取得。
//   為替 : frankfurter（ECB基準・キー不要）  base=USD → JPY/EUR
//   日経 : stooq の軽量CSV（キー不要）。EODのため前日比は前回終値をNVSに保存して算出。
#include "market.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

static bool httpGet(const char* url, String& body) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  if (!http.begin(client, url)) return false;
  int status = http.GET();
  if (status != 200) { Serial.printf("[mkt] HTTP %d  %s\n", status, url); http.end(); return false; }
  body = http.getString();
  http.end();
  return true;
}

bool fetchFx(Market& out) {
  String payload;
  if (!httpGet("https://api.frankfurter.dev/v1/latest?base=USD&symbols=JPY,EUR", payload))
    return false;
  JsonDocument doc;
  if (deserializeJson(doc, payload)) { Serial.println("[mkt] fx parse err"); return false; }
  float jpy = doc["rates"]["JPY"] | 0.0f;
  float eur = doc["rates"]["EUR"] | 0.0f;
  if (jpy <= 0 || eur <= 0) return false;
  out.usdjpy = jpy;
  out.eurjpy = jpy / eur;   // 1EUR = (JPY/USD) / (EUR/USD)
  out.fxValid = true;
  return true;
}

bool fetchNikkei(Market& out) {
  String payload;
  // f=sd2t2ohlcvp → 銘柄,日付,時刻,始値,高値,安値,終値,出来高,前日終値(Prev)
  // stooq が前日終値(p)を返すので、前日比は当日終値との差で即算出（日跨ぎ不要）。
  if (!httpGet("https://stooq.com/q/l/?s=%5Enkx&f=sd2t2ohlcvp&e=csv", payload))
    return false;

  // CSV 1行をカンマ分割（終値=index6, 前日終値=index8）
  String f[9];
  int idx = 0, start = 0;
  for (int i = 0; i <= (int)payload.length() && idx < 9; i++) {
    if (i == (int)payload.length() || payload[i] == ',' || payload[i] == '\n' || payload[i] == '\r') {
      f[idx++] = payload.substring(start, i);
      start = i + 1;
      if (i < (int)payload.length() && (payload[i] == '\n' || payload[i] == '\r')) break;
    }
  }
  if (idx < 7) { Serial.println("[mkt] nikkei csv short"); return false; }
  float close = f[6].toFloat();
  float prev  = (idx >= 9) ? f[8].toFloat() : 0.0f;
  if (close <= 0) { Serial.printf("[mkt] nikkei bad: %s\n", payload.c_str()); return false; }

  out.nikkei  = lroundf(close);
  out.nkValid = true;

  if (prev > 0) {
    out.hasChange = true;
    out.nikkeiChg = lroundf(close - prev);
    out.nikkeiPct = (close - prev) / prev * 100.0f;
  } else {
    out.hasChange = false;          // stooq が前日終値を返さない稀なケース
  }
  return true;
}
