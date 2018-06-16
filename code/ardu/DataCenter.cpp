#include "DataCenter.h"

#include <EEPROM.h>

void DataCenter::reset() {
  dataArr[RWMAX] = 255;
  dataArr[LWMAX] = 255;
  dataArr[MINTURNDIS] = 700;
  dataArr[TURNDELAY] = 360;
  dataArr[LOSTDEBUFF] = 100;
  dataArr[REVADJSPEED] = 80;
  dataArr[SDISUPBOUND] = 1400;
  dataArr[SDISLOWBOUND] = 1200;
  dataArr[ADISLOWLIM] = 500;
  dataArr[ADISRANDOM] = 50;
  dataArr[QDISRANDOM] = 50;
  dataArr[QDISRANDOMS] = 100;
}

int DataCenter::val(unsigned char no) { return dataArr[no]; }

void DataCenter::write(unsigned char no, int val) { dataArr[no] = val; }

void DataCenter::load() {
  char *p = (char *)dataArr;
  int l = sizeof(int) * M;
  for (int i = 0; i < l; i++) {
    p[i] = EEPROM[i];
  }
}

void DataCenter::save() {
  char *p = (char *)dataArr;
  int l = sizeof(int) * M;
  for (int i = 0; i < l; i++) {
    EEPROM[i] = p[i];
  }
}
