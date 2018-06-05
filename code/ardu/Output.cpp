#include "Output.h"
#include <arduino.h>

int Output::mstrfind(const char *src, char target) {
  for (int i = 0; src[i]; i++) {
    if (src[i] == target) return i;
  }
  return -1;
}

int Output::matchblock(const char *src) {
  int j = 0;
  while (src[j] != '}' && src[j]) j++;
  strncpy(buf, src, j);
  buf[j] = 0;
  return j;
}

void Output::printPages(const char *src) {
  lcd.clear();
  if (strlen(src) <= ROWNUM * COLNUM) {
    print(src);
  } else {
    for (int i = 0; i < strlen(src); i += ROWNUM * COLNUM) {
      lcd.clear();
      print(src + i);
      delay(3000);
    }
  }
}

void Output::printBlock(const char *str) {
  int cc, cr, i, j, cw, cup, t;
  for (cc = 1, i = 0; i < strlen(str) && str[i] != ';'; i++) {
    if (str[i] == '&') cc++;
  }
  for (cr = 0, i = 0; str[i]; i++) {
    if (str[i] == ';') cr++;
  }
  cw = COLNUM / cc;
  cup = 0;
  lcd.clear();
  for (i = 0; i < cr; i++) {
    for (j = 0; j < cc; j++) {
      if (j == cc - 1) {
        t = mstrfind(str + cup, ';');
      } else {
        t = mstrfind(str + cup, '&');
      }
      strncpy(buf, str + cup, t);
      buf[t] = 0;
      lcd.setCursor(j * cw, i);
      lcd.print(buf);
      clearBuf();
      cup += t + 1;
    }
  }
}

void Output::parse(const char *src) {
  for (int i = 0; src[i]; i++) {
    if (iswhite(src[i])) continue;
    if (src[i] == 'p') {
      while (src[i] != '{') i++;
      i++;
      i += matchblock(src + i);
      printPages(buf);
      continue;
    }
    if (src[i] == 'b') {
      while (src[i] != '{') i++;
      i++;
      i += matchblock(src + i);
      print(i, 2);
      print(src[i], 3);
      printBlock(buf);
      continue;
    }
    if (src[i] == '{') {
      i++;
      i += matchblock(src + i);
      print(buf);
      continue;
    }
    if (src[i] == 'd') {
      delay(2000);
      continue;
    }
    if (src[i] == 'c') {
      lcd.clear();
      continue;
    }
  }
}
