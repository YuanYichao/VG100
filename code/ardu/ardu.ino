#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

#include "Chassis.h"
#include "DisDetectors.hpp"
#include "Input.h"
#include "OpenMV.h"
#include "Output.h"
#include "Recorder.h"

char buf[256];
#define DISNUM 5
DisDetectors<DISNUM> dis;

int state = 0;

// for the DisDetectors
// f, r1, r2, l1, l2
unsigned char disPins[DISNUM][2] = {
    {24, 25}, {26, 27}, {28, 29}, {30, 31}, {32, 33}};

InfoData info;

int rWheel = 255, lWheel = 255;

bool unready() { return info.photoDis < 0 || info.turnDis < 0; }
// for the running task

void goStraight() {}

void doControlTurn(int dir, bool test = false) {}

void doFreeTurn() {}

void doRun() {
  bool sd = false, st = false;
  int dir = -1;
  while (1) {
    if (dis[0] < info.photoDis && dis[0] > info.turnDis && sd == false) {
      OpenMV::startDetect();
      sd = true;
    }
    if (dis[0] < info.turnDis && st == false) {
      sd = false;
      dir = OpenMV::getDir();
      OpenMV::endDetect();
      st = true;
    }
    if (st) {
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

void configureDisTurn() {
  Output::screen().parse(
      "c p{now begin to configure the distance where this car would start to "
      "turn.} c");
  double d;
  int dir;
  char key;
  while (1) {
    Output::screen().parse(
        "c p{put the car at a desired distance and press C to "
        "continue}");
    while (Input::device().getKey() != 'C') continue;
    Output::screen().parse("c {start testing}");
    d = dis[0];
    OpenMV::startDetect();
    delay(1000);
    dir = OpenMV::getDir();
    OpenMV::endDetect();
    doControlTurn(dir, true);
    Output::screen().print("is everything okay?", 1);
    Output::screen().print("A: agian. B: save", 3);
    while ((key = Input::device().getKey()) == NO_KEY) continue;
    if (key == 'A') {
      continue;
    } else {
      break;
    }
  }
  info.turnDis = d;
}

void configureDisPhoto() {
  Output::screen().parse(
      "c p{now begin to configure the distance where this car would start to "
      "photo and detect.} d");
  char key;
  int dir;
  double d;
  while (1) {
    Output::screen().parse(
        "c p{put the car at a desired distance and press C to "
        "continue}");
    while (Input::device().getKey() != 'C') continue;
    Output::screen().parse("c {start testing}");
    d = dis[0];
    OpenMV::startDetect();
    delay(1000);
    dir = OpenMV::getDir();
    OpenMV::endDetect();
    Output::screen().parse("c {the Result: dis is }");
    Output::screen().print(d, 1);
    switch (dir) {
      case 0:
        Output::screen().print("left", 2);
        break;
      case 1:
        Output::screen().print("right", 2);
        break;
      case -1:
        Output::screen().print("failed", 2);
      default:
        break;
    }
    Output::screen().print("A: agian. B: save", 3);
    while ((key = Input::device().getKey()) == NO_KEY) continue;
    if (key == 'A') {
      continue;
    } else {
      break;
    }
  }
  info.photoDis = d;
}

void tune() {
  Output::screen().parse("c b{1. get disPhoto; 2. get disTurn;A to return}");
  char key;
  while (1) {
    key = Input::device().getKey();
    if (key != NO_KEY) {
      switch (key) {
        case '1':
          configureDisPhoto();
          break;
        case '2':
          configureDisTurn();
          break;
        case 'A':
          Recorder::disk().record(info);
          return;
        default:
          break;
      }
    }
  }
}

void reset() {
  Output::screen().parse("c {reset} d");
  Output::screen().parse(
      ("c p{you are going to reset the EEPROM.} c p {This process is "
       "not recoverable!} d"));
  char key;
  while ((key = Input::device().getKey()) == NO_KEY) continue;
  if (key == 'A')
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

void tc() {
  char key;
  Output::screen().parse("c {test commands, 1-start, 2-dir, 3-end}");
  while (1) {
    key = Input::device().getKey();
    if (key != NO_KEY) switch (key) {
        case '1':
          OpenMV::startDetect();
          Output::screen().print("start", 3);
          break;
        case '2':
          Output::screen().print(OpenMV::getDir(), 3);
          break;
        case '3':
          OpenMV::endDetect();
          Output::screen().print("end", 3);
          break;
        default:
          Output::screen().parse("c {end}");
          return;
      }
  }
}

void debug() {
  char key;
  Output::screen().parse(
      "b{1. test Serial;2. test GPIO;3. test commands;4. quit;}");
  while (1) {
    if ((key = Input::device().getKey()) != NO_KEY) {
      switch (key) {
        case '1':
          ts();
          Output::screen().parse(
              "b{1. test Serial;2. test GPIO;3. test commands;4. quit;}");
          break;
        case '2':
          tg();
          Output::screen().parse(
              "b{1. test Serial;2. test GPIO;3. test commands;4. quit;}");
          break;
        case '3':
          tc();
          Output::screen().parse(
              "b{1. test Serial;2. test GPIO;3. test commands;4. quit;}");
          break;
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
