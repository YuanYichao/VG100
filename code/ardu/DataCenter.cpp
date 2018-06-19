#include "DataCenter.h"

#include <EEPROM.h>

void DataCenter::reset() {
  dataArr[RWMAX] = 50;
  dataArr[LWMAX] = 50;
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
  dataArr[PHOTODIS] = 1200;
  dataArr[LINEARKDIS] = 10;
  dataArr[LINEARKANGLE] = 10;
  dataArr[TDELAY] = 0;
  dataArr[UNNORMALSIDE] = 2000;
  dataArr[UNNORMALFRONT] = 1500;
  dataArr[UNNORMALFOR] = 4000;  
  dataArr[FORTRIGDIS] = 600;
  dataArr[FORTRIGK] = 5;
  dataArr[SPINSPEED] = 20;
  dataArr[TURNEND] = 50;
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
