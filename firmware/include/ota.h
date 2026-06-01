// ota.h — GitHub Releases からの HTTP-pull OTA（無線でファーム更新）。
//   端末を手元から渡した後でも、リリースを公開するだけで自動更新できるようにするための仕組み。
#ifndef PAPERX_OTA_H
#define PAPERX_OTA_H
#include <Arduino.h>

// リリースの度に +1 する。GitHub のタグ "v<N>" の N と一致させること。
#define FW_VERSION 1

// 更新中の状態を画面に出すためのコールバック（任意）。描画は main.cpp が担う。
typedef void (*OtaStatusFn)(const char* title, const char* line1, const char* line2);

// 最新リリースを確認し、FW_VERSION より新しければ自己更新→再起動する。
//   戻り値: true  = 更新を試みて画面に描画した（＝失敗。成功時は再起動して戻らない）
//           false = 更新なし／確認失敗（画面は触っていない）
// 成功時は再起動するためこの関数から戻らない。呼び出し側は true のとき再描画すればよい。
bool otaCheckAndUpdate(OtaStatusFn status = nullptr);

#endif
