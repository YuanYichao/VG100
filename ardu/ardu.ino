#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

#include "DisDetectors.hpp"
#include "Input.h"
#include "OpenMV.h"
#include "Output.h"
#include "Recorder.h"

char buf[256];
#define DISNUM 5
DisDetectors<DISNUM> dis;

// for the EEPROM

int state = 0;

// for the DisDetectors
// f, r1, r2, l1, l2
unsigned char disPins[DISNUM][2] = {
    {24, 25}, {26, 27}, {28, 29}, {30, 31}, {32, 33}};

InfoData info;

bool unready() { return info.photoDis < 0 || info.turnDis < 0; }

// for the running task

void goStraight() {}

void doControlTurn(int dir) {}

void doFreeTurn() {}

void doRun() {
  bool sd = false, st = false;
  while (1) {
    if (dis[0] < info.photoDis && dis[0] > info.turnDis && sd == false) {
      OpenMV::startDetect();
      sd = true;
    }
    if (dis[0] < info.turnDis&& st == false) {
      sd = false;
      OpenMV::endDetect();
      st = true;
    }
    if (st) {
      int dir = OpenMV::getDir();
      if (dir == -1) {
        doFreeTurn();
      } else {
        doControlTurn(dir);
      }
      st = false;
    }
    goStraight();
  }
}

void run() {
  Output::screen().clear();
  if (unready()) {
    Output::screen().parse("{distance data is not collected!} d d c");
  } else {
    Output::screen().parse("{distance data is collected} d c");
  }
  Output::screen().parse("{ready to run, press A to continue}");
  while (Input::device().getKey() != 'A') continue;
  doRun();
}

void configureDisTurn() {}

void configureDisPhoto() {}

void tune() {
  info.w = 11.1;
  info.l = 12.1;
  info.sensorDis = 11.1;
  info.photoDis = 29.0;
  info.turnDis = 11.5;
  Recorder::disk().record(info);
//  Output::screen().parse("c b{1. get disPhoto; 2. get disTurn;A to return}");
//  char key;
//  while (1) {
//    key = Input::device().getKey();
//    if (key != NO_KEY) {
//      switch (key) {
//        case '1':
//          configureDisPhoto();
//          break;
//        case '2':
//          configureDisTurn();
//          break;
//        case 'A':
//          Recorder::disk().record(info);
//          return;
//        default:
//          break;
//      }
//    }
//  }
}

void reset() {
  Output::screen().parse("c {reset} d");
  Output::screen().parse(("c p{you are going to reset the EEPROM.} c p {This process is "
                 "not recoverable!} d"));
  char key;
  while((key = Input::device().getKey()) == NO_KEY) continue;
  if(key == 'A') 
  return;
    else
    for (int i = 0; i < EEPROM.length(); i++) {
      if (EEPROM.read(i) != 0) EEPROM.write(i, 0);
    }
  Output::screen().parse("c p {the EEPROM has been reset} d d");

}

void aboutpt(double v, const char* err) {
  if (v < 0) {
    Output::screen().clear();
    Output::screen().print(err);
    delay(1000);
  } else {
    Output::screen().print(v);
    delay(1000);
  }
}

void about() {
  Output::screen().parse(
      "c p {following are the data}  d c{l, w, sd, pd, td} d");
  aboutpt(info.l, "length undefined");
  aboutpt(info.w, "width undefined");
  aboutpt(info.sensorDis, "sensorDis undefined");
  aboutpt(info.photoDis, "photoDis undefined");
  aboutpt(info.turnDis, "turnDis undefined");
}

void ts() {
  int i;
  Output::screen().parse("c {use A to return}");
  while (1) {
    if (Serial.available()) {
      Output::screen().parse("c {use A to return}");
      for (i = 0; Serial.available(); i++) {
        buf[i] = Serial.read();
        delay(20);
      }
      buf[i] = 0;
      Output::screen().print(buf, 1);
    }
    if (Input::device().getKey() == 'A') break;
  }
  Output::screen().parse("c {end}");
}

void tg() {
  bool p22 = false, p23 = false, p22t = false, p23t = false;
  Output::screen().parse("c {pin 22, 23 is used to test}");
  pinMode(22, INPUT);
  pinMode(23, INPUT);
  while (1) {
    p22 = digitalRead(22);
    p23 = digitalRead(23);
    if (p22 != p22t || p23 != p23t) {
      Output::screen().parse("c {pin 22, 23 is used to test. 22, 23}");
      Output::screen().print(p22, 2);
      Output::screen().print(p23, 3);
      p22t = p22;
      p23t = p23;
    }
    if (Input::device().getKey() == 'A') break;
  }
  Output::screen().parse("c {end}");
}

void debug() {
  char key;
  Output::screen().parse("b{1. test Serial;2. test GPIO;3. quit;}");
  while (1) {
    if ((key = Input::device().getKey()) != NO_KEY) {
      switch (key) {
        case '1':
          ts();
          Output::screen().parse("b{1. test Serial;2. test GPIO;3. quit;}");
          break;
        case '2':
          tg();
          Output::screen().parse("b{1. test Serial;2. test GPIO;3. quit;}");
          break;
        case '3':
        default:
          return;
      }
    }
  }
  return;
}

#define MENU "b {1.run&2.tune;3.reset&4.about;5.debug&;}"

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;
  dis.attach(disPins);
  info = Recorder::disk().readRecord();
  Output::screen().parse(MENU);
}

void loop() {
  char key = Input::device().getKey();
  switch (key) {
    case '1':
      run();
      Output::screen().parse("c {quit} d" MENU);
      break;
    case '2':
      tune();
      Output::screen().parse("c {quit} d" MENU);
      break;
    case '3':
      reset();
      Output::screen().parse("c {quit} d" MENU);
      break;
    case '4':
      about();
      Output::screen().parse("c {quit} d" MENU);
      break;
    case '5':
      debug();
      Output::screen().parse("c {quit} d" MENU);
    default:
      break;
  }
}
