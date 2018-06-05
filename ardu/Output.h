#ifndef OUTPUT_H
#define OUTPUT_H

#include <LiquidCrystal.h>

/*
 * rs pin ->50, en pin -> 52, d4 - d7 -> 53, 51, 49, 47
 * singleton design
 */

class Output {
  Output(int rs, int en, int d4, int d5, int d6, int d7)
      : lcd(rs, en, d4, d5, d6, d7) {
    lcd.begin(COLNUM, ROWNUM);
  }
  Output(const Output &) = delete;
  Output &operator=(const Output &) = delete;

  void clearBuf() { buf[0] = 0; }
  int mstrfind(const char *src, char target);
  bool iswhite(char c) { return c == ' ' || c == '\t' || c == '\n'; }
  int matchblock(const char *src);
  void printPages(const char *src);
  void printBlock(const char *str);

  LiquidCrystal lcd;
  const int COLNUM = 20;
  const int ROWNUM = 4;
  char buf[256] = {0};

 public:
  static Output &screen() {
    static Output ot(50, 52, 53, 51, 49, 47);
    return ot;
  }
  template <typename T>
  void print(T v, const unsigned char row = 0) {
    lcd.clear();
    lcd.setCursor(0, row);
    lcd.print(v);
  }

  template <int N>
  void print(char (&str)[N], const unsigned char row = 0) {
    for (int i = 0; str[i] && i < (COLNUM - row) * ROWNUM; i++) {
      lcd.setCursor(i % COLNUM, i / COLNUM + row);
      lcd.print(str[i]);
    }
  }

  void print(const char *str, const unsigned char row = 0) {
    for (int i = 0; str[i] && i < (COLNUM - row) * ROWNUM; i++) {
      lcd.setCursor(i % COLNUM, i / COLNUM + row);
      lcd.print(str[i]);
    }
  }

  void parse(const char *src);
  void clear() { lcd.clear(); }
};


#endif
