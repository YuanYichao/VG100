#include "Recorder.h"
#include <EEPROM.h>

InfoData Recorder::readRecord() {
  for (int i = 0; i < SZ; i++) {
    ddata.b[i] = EEPROM[i];
  }
  return ddata.v;
}

void Recorder::record(const InfoData& data) {
  ddata.v = data;
  for (int i = 0; i < SZ; i++) {
    EEPROM[i] = ddata.b[i];
  }
}
