// weather.h — Open-Meteo 取得とデータ構造（M3）。描画からは独立。
#ifndef PAPERX_WEATHER_H
#define PAPERX_WEATHER_H
#include <Arduino.h>

enum WxIcon { WX_SUN, WX_PARTLY, WX_CLOUD, WX_FOG, WX_RAIN, WX_SNOW, WX_THUNDER };

struct WxDay {
  int wday = 0;   // 0=日 .. 6=土
  int code = 0;   // WMO weather code
  int hi = 0;
  int lo = 0;
};

struct Weather {
  bool  valid = false;
  float tempNow = 0;     // 現在気温(℃)
  int   codeNow = 0;     // 現在のWMOコード
  int   hiToday = 0;
  int   loToday = 0;
  WxDay days[5];         // 翌日〜5日後
  int   rainStartHour = -1; // 本日これ以降で降水確率が高くなる最初の時刻(なければ-1)
  bool  rainingNow = false;
};

WxIcon      wxIconFor(int code);     // WMOコード → アイコン種別
const char* wxConditionJP(int code); // WMOコード → 日本語の概況

// Open-Meteo から取得して out に格納。失敗時 false（out.valid=false）。
// nowHour/nowWday は傘判定と曜日計算に使う（RTC の現在時刻から渡す）。
bool fetchWeather(double lat, double lon, int nowHour, int nowWday, Weather& out);

#endif
