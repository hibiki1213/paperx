// news.h — RSS（NHK 主要ニュース）取得とデータ構造（M5）。描画からは独立。
#ifndef PAPERX_NEWS_H
#define PAPERX_NEWS_H
#include <Arduino.h>

struct NewsItem {
  String title;
  char   hhmm[6] = "--:--";  // pubDate の HH:MM（JST）
};

struct News {
  bool valid = false;
  int  count = 0;
  NewsItem items[3];         // 見出し上位3件
};

// RSS を取得して out に格納。失敗時 false（out は変更しない＝直近値を保持）。
bool fetchNews(News& out);

#endif
