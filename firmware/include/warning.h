// warning.h — 気象庁 防災情報JSON（無認証）から大田区(1311100)の警報・注意報を取得。
//   https://www.jma.go.jp/bosai/warning/data/warning/130000.json (東京都)
//   2026-05-28 の新運用に対応：未知コード(危険警報L4等)は「気象警報」として警戒扱い。
#ifndef PAPERX_WARNING_H
#define PAPERX_WARNING_H
#include <Arduino.h>

struct WeatherWarning {
  bool valid    = false;   // 取得に成功したか
  bool active   = false;   // 大田区に発表中(発表/継続)の警報・注意報があるか
  int  severity = 0;       // 最大重大度: 2=注意報 3=警報 4=危険警報/未知 5=特別警報
  char headline[96] = {0}; // 表示用: 例「大雨警報 発表中」「大雨特別警報 ほか2件」
};

bool fetchWarning(WeatherWarning& out);  // 大田区の警報・注意報を取得

#endif
