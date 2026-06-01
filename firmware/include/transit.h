// transit.h — 鉄道遅延情報（rti-giken 無認証API）。東京の主要路線の遅延フラグ。
#ifndef PAPERX_TRANSIT_H
#define PAPERX_TRANSIT_H
#include <Arduino.h>

#define TRANSIT_N 13   // 固定で表示する主要路線数

struct Transit {
  bool valid = false;
  bool delayed[TRANSIT_N] = {false};
  char reason[TRANSIT_N][64] = {{0}};  // 遅延路線の理由（nTool info, UTF-8・16字で切詰め済）
  int  delayedCount = 0;   // 固定路線のうち遅延中の数
  int  otherCount   = 0;   // 固定リスト外で遅延している路線数
};

extern const char* const TRANSIT_NAMES[TRANSIT_N];  // 画面表示用の路線名

bool fetchTransit(Transit& out);

#endif
