// transit.h — 鉄道遅延情報（rti-giken 無認証API）。東京の主要路線の遅延フラグ。
#ifndef PAPERX_TRANSIT_H
#define PAPERX_TRANSIT_H
#include <Arduino.h>

#define TRANSIT_N 15   // 固定で表示する主要路線数

struct Transit {
  bool valid = false;
  bool delayed[TRANSIT_N] = {false};
  // 遅延路線の理由。優先上位は Yahoo!路線情報の全文、それ以外/失敗時は nTool の16字。
  char reason[TRANSIT_N][256] = {{0}};
  int  delayedCount = 0;   // 固定路線のうち遅延中の数
  int  otherCount   = 0;   // 固定リスト外で遅延している路線数
};

// 画面の遅延表示の優先順（この順に上から表示。Yahoo全文取得もこの上位から）。
extern const int TRANSIT_PRIORITY[TRANSIT_N];

extern const char* const TRANSIT_NAMES[TRANSIT_N];  // 画面表示用の路線名

bool fetchTransit(Transit& out);

#endif
