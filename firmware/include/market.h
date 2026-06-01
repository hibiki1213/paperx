// market.h — 為替(frankfurter)と日経平均(stooq)を取得。描画からは独立。
#ifndef PAPERX_MARKET_H
#define PAPERX_MARKET_H
#include <Arduino.h>

struct Market {
  bool  fxValid = false;
  float usdjpy = 0;        // 1ドル=◯円
  float eurjpy = 0;        // 1ユーロ=◯円

  bool  nkValid = false;
  long  nikkei = 0;        // 日経平均 直近終値（四捨五入）
  bool  hasChange = false; // 前日比が確定しているか（stooqがprevを返さない時のみfalse）
  long  nikkeiChg = 0;     // 前日比（円）
  float nikkeiPct = 0;     // 前日比（%）
};

bool fetchFx(Market& out);      // USD/JPY・EUR/JPY
bool fetchNikkei(Market& out);  // 日経平均。前日比は stooq の前日終値(p)から算出

#endif
