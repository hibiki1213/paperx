/**
 * wm_strings_ja.h — PaperX 用 日本語 + Apple HIG 風スタイルの WiFiManager 文字列差し替え
 * platformio.ini で  -DWM_STRINGS_FILE=\"wm_strings_ja.h\"  を指定して使う。
 * 元: tzapu/WiFiManager wm_strings_en.h（トークン {v}{t}{c}{i}... は変更不可）
 * フォント(@font-face)は portal_fonts.h を setCustomHeadElement() で注入する。
 */
#ifndef _WM_STRINGS_JA_H_
#define _WM_STRINGS_JA_H_

#include "wm_consts_en.h" // 定数・トークン・ルート定義（必須）

const char WM_LANGUAGE[] PROGMEM = "ja";

const char HTTP_HEAD_START[]       PROGMEM = "<!DOCTYPE html>"
"<html lang='ja'><head>"
"<meta name='format-detection' content='telephone=no'>"
"<meta charset='UTF-8'>"
"<meta  name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
"<title>{v}</title>";

const char HTTP_SCRIPT[]           PROGMEM = "<script>function c(l){"
"document.getElementById('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;"
"p = l.nextElementSibling.classList.contains('l');"
"document.getElementById('p').disabled = !p;"
"if(p)document.getElementById('p').focus();};"
"function f() {var x = document.getElementById('p');x.type==='password'?x.type='text':x.type='password';}"
"</script>";

const char HTTP_HEAD_END[]         PROGMEM = "</head><body class='{c}'><div class='wrap'>";

const char HTTP_ROOT_MAIN[]        PROGMEM = "<h1>{t}</h1><h3>{v}</h3>";

const char * const HTTP_PORTAL_MENU[] PROGMEM = {
"<form action='/wifi'    method='get'><button>Wi-Fiを設定</button></form><br/>\n",          // MENU_WIFI
"<form action='/0wifi'   method='get'><button>Wi-Fiを設定（スキャンなし）</button></form><br/>\n", // MENU_WIFINOSCAN
"<form action='/info'    method='get'><button class='sec'>デバイス情報</button></form><br/>\n",   // MENU_INFO
"<form action='/param'   method='get'><button class='sec'>設定</button></form><br/>\n",          // MENU_PARAM
"<form action='/close'   method='get'><button class='sec'>閉じる</button></form><br/>\n",        // MENU_CLOSE
"<form action='/restart' method='get'><button class='sec'>再起動</button></form><br/>\n",        // MENU_RESTART
"<form action='/exit'    method='get'><button class='sec'>終了</button></form><br/>\n",          // MENU_EXIT
"<form action='/erase'   method='get'><button class='D'>Wi-Fi設定を消去</button></form><br/>\n", // MENU_ERASE
"<form action='/update'  method='get'><button class='sec'>ファーム更新</button></form><br/>\n",  // MENU_UPDATE
"<hr>" // MENU_SEP
};

const char HTTP_PORTAL_OPTIONS[]   PROGMEM = "";
const char HTTP_ITEM_QI[]          PROGMEM = "<div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i} {h}'></div>";
const char HTTP_ITEM_QP[]          PROGMEM = "<div class='q {h}'>{r}%</div>";
const char HTTP_ITEM[]             PROGMEM = "<div><a href='#p' onclick='c(this)' data-ssid='{V}'>{v}</a>{qi}{qp}</div>";

const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'>";
const char HTTP_FORM_WIFI[]        PROGMEM = "<label for='s'>ネットワーク（SSID）</label><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='{v}'><br/><label for='p'>パスワード</label><input id='p' name='p' maxlength='64' type='password' placeholder='{p}'><input type='checkbox' id='showpass' onclick='f()'> <label for='showpass'>パスワードを表示</label><br/>";
const char HTTP_FORM_WIFI_END[]    PROGMEM = "";
const char HTTP_FORM_STATIC_HEAD[] PROGMEM = "<hr><br/>";
const char HTTP_FORM_END[]         PROGMEM = "<br/><br/><button type='submit'>保存</button></form>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "<hr><br/>";
const char HTTP_FORM_PARAM[]       PROGMEM = "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n";

const char HTTP_SCAN_LINK[]        PROGMEM = "<br/><form action='/wifi?refresh=1' method='POST'><button name='refresh' value='1' class='sec'>再スキャン</button></form>";
const char HTTP_SAVED[]            PROGMEM = "<div class='msg'>認証情報を保存しました。<br/>ネットワークへの接続を試みています。<br/>失敗した場合は、もう一度このAPに接続してやり直してください。</div>";
const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg S'>保存しました<br/></div>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";
const char HTTP_ERASEBTN[]         PROGMEM = "<br/><form action='/erase' method='get'><button class='D'>Wi-Fi設定を消去</button></form>";
const char HTTP_UPDATEBTN[]        PROGMEM = "<br/><form action='/update' method='get'><button class='sec'>ファーム更新</button></form>";
const char HTTP_BACKBTN[]          PROGMEM = "<hr><br/><form action='/' method='get'><button class='sec'>戻る</button></form>";

const char HTTP_STATUS_ON[]        PROGMEM = "<div class='msg S'><strong>接続済み</strong>：{v}<br/><em><small>IP {i}</small></em></div>";
const char HTTP_STATUS_OFF[]       PROGMEM = "<div class='msg {c}'><strong>未接続</strong>：{v}{r}</div>";
const char HTTP_STATUS_OFFPW[]     PROGMEM = "<br/>認証エラー（パスワードを確認してください）";
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "<br/>ネットワークが見つかりません";
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "<br/>接続できませんでした";
const char HTTP_STATUS_NONE[]      PROGMEM = "<div class='msg'>ネットワーク未設定</div>";
const char HTTP_BR[]               PROGMEM = "<br/>";

const char HTTP_STYLE[]            PROGMEM = "<style>"
":root{--bg:#f2f2f7;--card:#fff;--text:#1c1c1e;--muted:#8e8e93;--accent:#007aff;--danger:#ff3b30;--field:#fff;--border:#d1d1d6;--sep:#e5e5ea}"
"*{box-sizing:border-box}"
"body{margin:0;padding:30px 16px 60px;background:var(--bg);color:var(--text);"
"font-family:'Noto Sans JP',-apple-system,BlinkMacSystemFont,'Hiragino Sans','Hiragino Kaku Gothic ProN',sans-serif;"
"-webkit-font-smoothing:antialiased;text-rendering:optimizeLegibility;text-align:center;line-height:1.5;font-size:17px}"
".wrap{display:inline-block;text-align:left;width:100%;max-width:430px}"
"h1{font-family:'Poppins',-apple-system,sans-serif;font-weight:600;font-size:34px;letter-spacing:-.3px;margin:6px 0 0;padding:0;text-align:center}"
"h3{font-weight:400;font-size:14px;color:var(--muted);margin:4px 0 26px;padding:0;text-align:center;letter-spacing:.2px}"
"br{line-height:0}"
"label{display:block;font-size:13px;color:var(--muted);margin:16px 4px 6px;font-weight:500}"
"input,select{display:block;width:100%;font-size:17px;font-family:inherit;padding:13px 14px;border:1px solid var(--border);border-radius:12px;background:var(--field);color:var(--text);margin:0;-webkit-appearance:none;appearance:none}"
"input:focus,select:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px rgba(0,122,255,.18)}"
"input[type=radio],input[type=checkbox]{display:inline-block;width:auto;margin:0 8px 0 0;transform:scale(1.25);vertical-align:middle}"
"label[for=showpass]{display:inline-block;color:var(--text);font-size:15px;margin:16px 0 0;vertical-align:middle}"
"input[type=file]{padding:10px;border-style:dashed}"
"button,input[type=button],input[type=submit]{display:block;width:100%;cursor:pointer;border:0;border-radius:12px;background:var(--accent);color:#fff;"
"font-family:inherit;font-weight:600;font-size:17px;line-height:1.2;padding:15px 16px;margin:0}"
"button:active{opacity:.55}"
"button.sec{background:var(--card);color:var(--accent);border:1px solid var(--sep)}"
"button.D{background:var(--danger);color:#fff;border:0}"
"form{margin:0 0 10px}"
// SSID リストを iOS のリスト行風に
".wrap>div{position:relative;background:var(--card);border-radius:12px;margin:0 0 8px;min-height:52px}"
".wrap>div>a{display:block;padding:15px 58px 15px 16px;color:var(--text);font-weight:500;font-size:17px;text-decoration:none}"
".wrap>div>a:active{opacity:.55}"
// メッセージカード
".msg{background:var(--card);border-radius:12px;padding:16px 18px;margin:18px 0;border:0;border-left:4px solid var(--muted);font-size:15px;line-height:1.6;color:var(--text)}"
".msg strong{font-weight:600}.msg.P{border-left-color:var(--accent)}.msg.D{border-left-color:var(--danger)}.msg.S{border-left-color:#34c759}"
"hr{border:0;border-top:1px solid var(--sep);margin:22px 0}"
"dl{margin:0}dt{font-weight:600;margin-top:12px}dd{margin:0;padding:0 0 .4em;color:var(--muted);word-break:break-all}"
"td{vertical-align:top}.h{display:none}:disabled{opacity:.5}"
"a{color:var(--accent);text-decoration:none}"
"progress{width:100%}"
// 電波強度アイコン（オリジナルのスプライトを踏襲。右端に縦中央配置）
".q{position:absolute;right:14px;top:50%;transform:translateY(-50%);height:16px;margin:0;padding:0;text-align:right;min-width:38px}"
".q.q-0:after{background-position-x:0}.q.q-1:after{background-position-x:-16px}.q.q-2:after{background-position-x:-32px}.q.q-3:after{background-position-x:-48px}.q.q-4:after{background-position-x:-64px}.q.l:before{background-position-x:-80px;padding-right:5px}.ql .q{left:14px;right:auto}.q:after,.q:before{content:'';width:16px;height:16px;display:inline-block;background-repeat:no-repeat;background-position:16px 0;"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');}"
"@media (-webkit-min-device-pixel-ratio: 2),(min-resolution: 192dpi){.q:before,.q:after {"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALwAAAAgCAMAAACfM+KhAAAALVBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAOrOgAAAADnRSTlMAESIzRGZ3iJmqu8zd7gKjCLQAAACmSURBVHgB7dDBCoMwEEXRmKlVY3L//3NLhyzqIqSUggy8uxnhCR5Mo8xLt+14aZ7wwgsvvPA/ofv9+44334UXXngvb6XsFhO/VoC2RsSv9J7x8BnYLW+AjT56ud/uePMdb7IP8Bsc/e7h8Cfk912ghsNXWPpDC4hvN+D1560A1QPORyh84VKLjjdvfPFm++i9EWq0348XXnjhhT+4dIbCW+WjZim9AKk4UZMnnCEuAAAAAElFTkSuQmCC');"
"background-size: 95px 16px;}}"
// 端末のダークモードに自動追従（HIG）
"@media(prefers-color-scheme:dark){:root{--bg:#000;--card:#1c1c1e;--text:#fff;--muted:#98989f;--field:#1c1c1e;--border:#3a3a3c;--sep:#38383a}"
"body.invert .q[role=img],.q[role=img]{-webkit-filter:invert(1);filter:invert(1)}}"
"</style>";

#ifndef WM_NOHELP
const char HTTP_HELP[]             PROGMEM =
 "<br/><h3>ページ一覧</h3><hr>"
 "<table class='table'>"
 "<thead><tr><th>Page</th><th>Function</th></tr></thead><tbody>"
 "<tr><td><a href='/'>/</a></td><td>メニュー</td></tr>"
 "<tr><td><a href='/wifi'>/wifi</a></td><td>Wi-Fiスキャン結果と設定（/0wifi はスキャンなし）</td></tr>"
 "<tr><td><a href='/wifisave'>/wifisave</a></td><td>Wi-Fi設定を保存</td></tr>"
 "<tr><td><a href='/param'>/param</a></td><td>パラメータ設定</td></tr>"
 "<tr><td><a href='/info'>/info</a></td><td>デバイス情報</td></tr>"
 "<tr><td><a href='/u'>/u</a></td><td>OTA更新</td></tr>"
 "<tr><td><a href='/close'>/close</a></td><td>キャプティブポータルを閉じる（設定は継続）</td></tr>"
 "<tr><td>/exit</td><td>設定ポータルを終了</td></tr>"
 "<tr><td>/restart</td><td>再起動</td></tr>"
 "<tr><td>/erase</td><td>Wi-Fi設定を消去して再起動</td></tr>"
 "</table>"
 "<p/>Github <a href='https://github.com/tzapu/WiFiManager'>https://github.com/tzapu/WiFiManager</a>.";
#else
const char HTTP_HELP[]             PROGMEM = "";
#endif

const char HTTP_UPDATE[] PROGMEM = "新しいファームウェアをアップロード<br/><form method='POST' action='u' enctype='multipart/form-data' onchange=\"(function(el){document.getElementById('uploadbin').style.display = el.value=='' ? 'none' : 'initial';})(this)\"><input type='file' name='update' accept='.bin,application/octet-stream'><button id='uploadbin' type='submit' class='h D'>更新</button></form><small><a href='http://192.168.4.1/update' target='_blank'>* キャプティブポータル内で動かない場合はブラウザで http://192.168.4.1 を開いてください</a><small>";
const char HTTP_UPDATE_FAIL[] PROGMEM = "<div class='msg D'><strong>更新に失敗しました</strong><Br/>再起動してやり直してください</div>";
const char HTTP_UPDATE_SUCCESS[] PROGMEM = "<div class='msg S'><strong>更新に成功しました。</strong> <br/> 再起動します...</div>";

// 情報ページ（技術項目は英語のまま）
#ifdef ESP32
	const char HTTP_INFO_esphead[]    PROGMEM = "<h3>esp32</h3><hr><dl>";
	const char HTTP_INFO_chiprev[]    PROGMEM = "<dt>Chip rev</dt><dd>{1}</dd>";
  	const char HTTP_INFO_lastreset[]  PROGMEM = "<dt>Last reset reason</dt><dd>CPU0: {1}<br/>CPU1: {2}</dd>";
  	const char HTTP_INFO_aphost[]     PROGMEM = "<dt>Access point hostname</dt><dd>{1}</dd>";
    const char HTTP_INFO_psrsize[]    PROGMEM = "<dt>PSRAM Size</dt><dd>{1} bytes</dd>";
	const char HTTP_INFO_temp[]       PROGMEM = "<dt>Temperature</dt><dd>{1} C&deg; / {2} F&deg;</dd>";
    const char HTTP_INFO_hall[]       PROGMEM = "<dt>Hall</dt><dd>{1}</dd>";
#else
	const char HTTP_INFO_esphead[]    PROGMEM = "<h3>esp8266</h3><hr><dl>";
	const char HTTP_INFO_fchipid[]    PROGMEM = "<dt>Flash chip ID</dt><dd>{1}</dd>";
	const char HTTP_INFO_corever[]    PROGMEM = "<dt>Core version</dt><dd>{1}</dd>";
	const char HTTP_INFO_bootver[]    PROGMEM = "<dt>Boot version</dt><dd>{1}</dd>";
	const char HTTP_INFO_lastreset[]  PROGMEM = "<dt>Last reset reason</dt><dd>{1}</dd>";
	const char HTTP_INFO_flashsize[]  PROGMEM = "<dt>Real flash size</dt><dd>{1} bytes</dd>";
#endif

const char HTTP_INFO_memsmeter[]  PROGMEM = "<br/><progress value='{1}' max='{2}'></progress></dd>";
const char HTTP_INFO_memsketch[]  PROGMEM = "<dt>Memory - Sketch size</dt><dd>Used / Total bytes<br/>{1} / {2}";
const char HTTP_INFO_freeheap[]   PROGMEM = "<dt>Memory - Free heap</dt><dd>{1} bytes available</dd>";
const char HTTP_INFO_wifihead[]   PROGMEM = "<br/><h3>WiFi</h3><hr>";
const char HTTP_INFO_uptime[]     PROGMEM = "<dt>Uptime</dt><dd>{1} mins {2} secs</dd>";
const char HTTP_INFO_chipid[]     PROGMEM = "<dt>Chip ID</dt><dd>{1}</dd>";
const char HTTP_INFO_idesize[]    PROGMEM = "<dt>Flash size</dt><dd>{1} bytes</dd>";
const char HTTP_INFO_sdkver[]     PROGMEM = "<dt>SDK version</dt><dd>{1}</dd>";
const char HTTP_INFO_cpufreq[]    PROGMEM = "<dt>CPU frequency</dt><dd>{1}MHz</dd>";
const char HTTP_INFO_apip[]       PROGMEM = "<dt>Access point IP</dt><dd>{1}</dd>";
const char HTTP_INFO_apmac[]      PROGMEM = "<dt>Access point MAC</dt><dd>{1}</dd>";
const char HTTP_INFO_apssid[]     PROGMEM = "<dt>Access point SSID</dt><dd>{1}</dd>";
const char HTTP_INFO_apbssid[]    PROGMEM = "<dt>BSSID</dt><dd>{1}</dd>";
const char HTTP_INFO_stassid[]    PROGMEM = "<dt>Station SSID</dt><dd>{1}</dd>";
const char HTTP_INFO_staip[]      PROGMEM = "<dt>Station IP</dt><dd>{1}</dd>";
const char HTTP_INFO_stagw[]      PROGMEM = "<dt>Station gateway</dt><dd>{1}</dd>";
const char HTTP_INFO_stasub[]     PROGMEM = "<dt>Station subnet</dt><dd>{1}</dd>";
const char HTTP_INFO_dnss[]       PROGMEM = "<dt>DNS Server</dt><dd>{1}</dd>";
const char HTTP_INFO_host[]       PROGMEM = "<dt>Hostname</dt><dd>{1}</dd>";
const char HTTP_INFO_stamac[]     PROGMEM = "<dt>Station MAC</dt><dd>{1}</dd>";
const char HTTP_INFO_conx[]       PROGMEM = "<dt>Connected</dt><dd>{1}</dd>";
const char HTTP_INFO_autoconx[]   PROGMEM = "<dt>Autoconnect</dt><dd>{1}</dd>";

const char HTTP_INFO_aboutver[]     PROGMEM = "<dt>WiFiManager</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutarduino[] PROGMEM = "<dt>Arduino</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutsdk[]     PROGMEM = "<dt>ESP-SDK/IDF</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutdate[]    PROGMEM = "<dt>Build date</dt><dd>{1}</dd>";

const char S_brand[]              PROGMEM = "PaperX";
const char S_debugPrefix[]        PROGMEM = "*wm:";
const char S_y[]                  PROGMEM = "はい";
const char S_n[]                  PROGMEM = "いいえ";
const char S_enable[]             PROGMEM = "有効";
const char S_disable[]            PROGMEM = "無効";
const char S_GET[]                PROGMEM = "GET";
const char S_POST[]               PROGMEM = "POST";
const char S_NA[]                 PROGMEM = "不明";
const char S_passph[]             PROGMEM = "********";
const char S_titlewifisaved[]     PROGMEM = "保存しました";
const char S_titlewifisettings[]  PROGMEM = "設定を保存しました";
const char S_titlewifi[]          PROGMEM = "Wi-Fi設定";
const char S_titleinfo[]          PROGMEM = "デバイス情報";
const char S_titleparam[]         PROGMEM = "設定";
const char S_titleparamsaved[]    PROGMEM = "保存しました";
const char S_titleexit[]          PROGMEM = "終了";
const char S_titlereset[]         PROGMEM = "リセット";
const char S_titleerase[]         PROGMEM = "消去";
const char S_titleclose[]         PROGMEM = "閉じる";
const char S_options[]            PROGMEM = "options";
const char S_nonetworks[]         PROGMEM = "ネットワークが見つかりません。再スキャンしてください。";
const char S_staticip[]           PROGMEM = "固定IP";
const char S_staticgw[]           PROGMEM = "ゲートウェイ";
const char S_staticdns[]          PROGMEM = "DNS";
const char S_subnet[]             PROGMEM = "サブネット";
const char S_exiting[]            PROGMEM = "終了します";
const char S_resetting[]          PROGMEM = "数秒後に再起動します。";
const char S_closing[]            PROGMEM = "このページは閉じて構いません。設定ポータルは引き続き動作します。";
const char S_error[]              PROGMEM = "エラーが発生しました";
const char S_notfound[]           PROGMEM = "File not found\n\n";
const char S_uri[]                PROGMEM = "URI: ";
const char S_method[]             PROGMEM = "\nMethod: ";
const char S_args[]               PROGMEM = "\nArguments: ";
const char S_parampre[]           PROGMEM = "param_";

const char D_HR[]                 PROGMEM = "--------------------";

#ifdef ESP8266
    const char S_ssidpre[]        PROGMEM = "ESP";
#elif defined(ESP32)
    const char S_ssidpre[]        PROGMEM = "ESP32";
#else
    const char S_ssidpre[]        PROGMEM = "WM";
#endif

#endif
