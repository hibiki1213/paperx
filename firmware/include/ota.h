// ota.h — GitHub Releases からの HTTP-pull OTA（無線でファーム更新）。
//   端末を手元から渡した後でも、リリースを公開するだけで自動更新できるようにするための仕組み。
#ifndef PAPERX_OTA_H
#define PAPERX_OTA_H
#include <Arduino.h>
#include <time.h>

// リリースの度に +1 する。GitHub のタグ "v<N>" の N と一致させること。
#define FW_VERSION 1

// 更新中の状態を画面に出すためのコールバック（任意）。描画は main.cpp が担う。
typedef void (*OtaStatusFn)(const char* title, const char* line1, const char* line2);

// 直近の更新確認の結果（ページ3の状態表示用）。otaCheckAndUpdate() が更新する。
struct OtaState {
  long   latest      = -1;     // 最後に確認できた最新リリース番号（-1=未確認/失敗）
  time_t checkedAt   = 0;      // 最終確認の時刻（epoch, 0=未確認）
  bool   lastCheckOk = false;  // 直近の確認が成功したか
};
extern OtaState g_ota;

// 最新リリースを確認し、FW_VERSION より新しければ自己更新→再起動する。
//   戻り値: true  = 更新を試みて画面に描画した（＝失敗。成功時は再起動して戻らない）
//           false = 更新なし／確認失敗（画面は触っていない）
// 成功時は再起動するためこの関数から戻らない。呼び出し側は true のとき再描画すればよい。
bool otaCheckAndUpdate(OtaStatusFn status = nullptr);

#endif
