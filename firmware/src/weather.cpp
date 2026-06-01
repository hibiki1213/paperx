// weather.cpp — Open-Meteo（APIキー不要・HTTPS）から天気を取得して Weather に詰める。
#include "weather.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

WxIcon wxIconFor(int c) {
  if (c == 0 || c == 1)                      return WX_SUN;
  if (c == 2)                                return WX_PARTLY;
  if (c == 3)                                return WX_CLOUD;
  if (c == 45 || c == 48)                    return WX_FOG;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) return WX_RAIN;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86)   return WX_SNOW;
  if (c >= 95)                               return WX_THUNDER;
  return WX_CLOUD;
}

const char* wxConditionJP(int c) {
  switch (c) {
    case 0:  return "快晴";
    case 1:  return "晴れ";
    case 2:  return "晴れ時々曇り";
    case 3:  return "曇り";
    case 45: case 48: return "霧";
    case 51: case 53: case 55: return "小雨";
    case 56: case 57: return "みぞれ";
    case 61: return "弱い雨";
    case 63: return "雨";
    case 65: return "強い雨";
    case 66: case 67: return "みぞれ";
    case 71: return "弱い雪";
    case 73: return "雪";
    case 75: return "大雪";
    case 77: return "霧雪";
    case 80: case 81: case 82: return "にわか雨";
    case 85: case 86: return "にわか雪";
    case 95: return "雷雨";
    case 96: case 99: return "雷を伴う雨";
    default: return "—";
  }
}

bool fetchWeather(double lat, double lon, int nowHour, int nowWday, Weather& out) {
  WiFiClientSecure client;
  client.setInsecure();           // 証明書検証なし（公開APIのため十分）
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);

  char url[440];
  snprintf(url, sizeof url,
    "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
    "&current=temperature_2m,weather_code"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min"
    "&hourly=precipitation_probability"
    "&timezone=Asia%%2FTokyo&forecast_days=6",
    lat, lon);

  if (!http.begin(client, url)) return false;
  int status = http.GET();
  if (status != 200) { Serial.printf("[wx] HTTP %d\n", status); http.end(); return false; }

  // HTTPS では getStream() を ArduinoJson に直接渡すと読めないことがあるため、
  // 一旦 String に受けてから解析する（応答は数KBと小さい）。
  String payload = http.getString();
  http.end();

  // 必要なフィールドだけ残すフィルタ（time/units を捨ててメモリ節約）
  JsonDocument filter;
  filter["current"]["temperature_2m"] = true;
  filter["current"]["weather_code"]   = true;
  filter["daily"]["weather_code"]        = true;
  filter["daily"]["temperature_2m_max"]  = true;
  filter["daily"]["temperature_2m_min"]  = true;
  filter["hourly"]["precipitation_probability"] = true;

  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (err) { Serial.printf("[wx] parse: %s\n", err.c_str()); return false; }

  out.tempNow = doc["current"]["temperature_2m"] | 0.0f;
  out.codeNow = doc["current"]["weather_code"]   | 0;

  JsonArray dc   = doc["daily"]["weather_code"];
  JsonArray dmax = doc["daily"]["temperature_2m_max"];
  JsonArray dmin = doc["daily"]["temperature_2m_min"];
  if (dmax.size() >= 1) {
    out.hiToday = lroundf(dmax[0] | 0.0f);
    out.loToday = lroundf(dmin[0] | 0.0f);
  }
  for (int i = 0; i < 5; i++) {
    int idx = i + 1;  // 0=今日, 1..5=翌日以降
    out.days[i].wday = (nowWday + idx) % 7;
    out.days[i].code = dc[idx]   | 0;
    out.days[i].hi   = lroundf(dmax[idx] | 0.0f);
    out.days[i].lo   = lroundf(dmin[idx] | 0.0f);
  }

  // 傘判定: 本日 nowHour 以降で降水確率 >= 50% の最初の時刻
  JsonArray hp = doc["hourly"]["precipitation_probability"];
  out.rainStartHour = -1;
  for (int h = nowHour; h < 24 && h < (int)hp.size(); h++) {
    if ((hp[h] | 0) >= 50) { out.rainStartHour = h; break; }
  }
  int pnow = (nowHour < (int)hp.size()) ? (hp[nowHour] | 0) : 0;
  WxIcon ic = wxIconFor(out.codeNow);
  out.rainingNow = (ic == WX_RAIN || ic == WX_THUNDER || ic == WX_SNOW) || pnow >= 50;

  out.valid = true;
  return true;
}
