#include <EEPROM.h>
#include <stdio.h>
#include "Chassis.h"
#include "DisDetectors.hpp"
#include "Input.h"
#include "OpenMV.h"
#include "Output.h"
#include "Recorder.h"

char buf[256];
const double RANGE = 0.5;

// f, r1, r2, l1, l2
#define DISNUM 5
DisDetectors<DISNUM> dis;
unsigned char disPins[DISNUM][2] = {
    {24, 25}, {26, 27}, {28, 29}, {30, 31}, {32, 33}};

InfoData info;
const unsigned char RWMAX = 255, LWMAX = 255;
unsigned char rWheel = RWMAX, lWheel = LWMAX;

const double MINTURNDIS = 20.00;

bool unready() { return info.photoDis < 0 || info.turnDis < 0; }
// for the running task

double avgDisRight() { return (dis[1] + dis[2]) / 2; }

double avgDisLeft() { return (dis[3] + dis[4]) / 2; }

double avgSideWidth() { return (avgDisRight() + avgDisLeft()) / 2; }

double turnOuterRadius() { return dis[0] + info.l - avgSideWidth(); }

double turnInnerRadius() { return turnOuterRadius() - info.w; }

// positive to right, neg to left
int unnormalSingle() {
  if (abs((dis[1] - dis[2])) > RANGE) return dis[1] - dis[2];
  else return 0;
}

int unnormalDouble() {
  if (abs(avgDisLeft() - avgDisRight()) > RANGE)
    return avgDisRight() - avgDisLeft();
  else return 0;
}

bool unnormal() { return unnormalSingle() || unnormalDouble(); }

bool hasFixedS() { return abs(dis[1] - dis[2]) < RANGE; }

bool hasFixedD() { return abs(avgDisLeft() - avgDisRight()) < RANGE; }

void goStraight() {
  static const int INTERVAL = 5, STEPVAL = 10;
  long lastTimeS = 0, lastTimeD = 0;
  bool fixingS = false, fixingD = false;
  int dir = 0, step = STEPVAL;
  // start a fixing task
  if (unnormalSingle() && !fixingS && !fixingD) {
    Output::screen().parse("{enter FS}");
    fixingS = true;
    lastTimeS = millis();
  }
  if (unnormalDouble() && !fixingS && !fixingD) {
    Output::screen().parse("{enter FD}");
    fixingD = true;
    lastTimeD = millis();
  }
  // alter step
  if (fixingS) {
    dir = unnormalSingle();
    if (lastTimeS - millis() > INTERVAL) {
      step += STEPVAL;
      lastTimeS = millis();
    }
  }
  if (fixingD) {
    dir = unnormalDouble();
    if (lastTimeD - millis() > INTERVAL) {
      step += STEPVAL;
      lastTimeD = millis();
    }
  }
  if (dir > 0 && (fixingD || fixingS)) {
    rWheel -= step;
  } else if(dir <0) {
    lWheel -= step;
  }
  Output::screen().print(dir,1);
  Output::screen().print(rWheel,2);
  Output::screen().print(lWheel,3);
  // end task
  if (fixingS && hasFixedS()) {
    Output::screen().parse("{quit FS}");
    fixingS = false;
    rWheel = RWMAX;
    lWheel = LWMAX;
    dir = 0;
    step = STEPVAL;
  }
  if (fixingD && hasFixedD()) {
    Output::screen().parse("{quit FD}");
    fixingD = false;
    rWheel = RWMAX;
    lWheel = LWMAX;
    dir =0;
    step = STEPVAL;
  }
  Chassis::state().write(rWheel, lWheel);
}

bool withinError(double dis1, double dis2, double Merror) {
  double error = dis1 - dis2;
  double abserror = error > 0 ? error : -error;
  return abserror < Merror;
}

bool isTurningEnd(int dir, bool test = false, long stTime = 0) {
  bool isEnd = false;
  if (withinError(dis[1 + dir * 2], dis[2 + dir * 2], RANGE)) isEnd = true;
  if (test) {
    if (millis() - stTime > 4000) isEnd = true;
  }
  return isEnd;
}

void doControlTurn(int dir, bool test = false) {
  long stTime = millis();
  double ratio = turnOuterRadius() / turnInnerRadius();
  if (dir > 0) {
    rWheel *= ratio;
  } else if (dir < 0) {
    lWheel *= ratio;
  }
  Chassis::state().write(rWheel, lWheel);
  while (1) {
    if (isTurningEnd(dir, test, stTime)) return;
    Chassis::state().move();
  }
}

void doFreeTurn() {
  while (dis[0] < MINTURNDIS) Chassis::state().write(RWMAX, LWMAX);
  int dir = dis[1] > dis[3] ? dir = 1 : dir = 0;
  if (dir > 0) {
    Chassis::state().write(0, LWMAX);
  } else if(dir < 0) {
    Chassis::state().write(RWMAX, 0);
  }
  while (!withinError(dis[1 + dir * 2], dis[2 + dir * 2], 1))
    Chassis::state().move();
}

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
    Chassis::state().move();
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

void tm() {
  char key;
  Output::screen().parse("c {test motors, A to break} d d");
  while (1) {
    key = Input::device().getKey();
    if (key == 'A') break;
    Output::screen().parse("c {right motor moves forward}");
    Chassis::state().write(200, 0);
    Chassis::state().move();
    delay(5000);
    Output::screen().parse("c {left motor moves forward}");
    Chassis::state().write(0, 200);
    Chassis::state().move();
    delay(5000);
    Output::screen().parse("c {right motor moves backward}");
    Chassis::state().write(-200, 0);
    Chassis::state().move();
    delay(5000);
    Output::screen().parse("c {left motor moves backward}");
    Chassis::state().write(0, -200);
    Chassis::state().move();
    delay(5000);
  }
  Output::screen().parse("c {end}");
}

void tss() {
  char key;
  int cur = 0;
  long lastT = millis();
  Output::screen().parse("c {test sensors, numbers to switch, A to break} d d");
  while (1) {
    Serial.print("loop at ");
    Serial.println(millis());
    key = Input::device().getKey();
    if (key != NO_KEY) switch (key) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':     
          cur = key - '0';
          break;
        default:
          Output::screen().parse("c {end}");
          return;
      }
      Serial.println(millis());
     Output::screen().print(dis[cur]);
     Serial.print("next at ");
     Serial.println(millis());
  }
}

void singleMove(){
  Output::screen().parse("c {smove}");
  long t= millis();
  while(dis[0] > 20){
    if(millis() - t % 50 > 40){
//      Output::screen().print(dis[1],0);
//       Output::screen().print(dis[2],1);
//       Output::screen().print(dis[3],2);
//       Output::screen().print(dis[4],3);
    }
    goStraight();
    Chassis::state().move();
  }
  rWheel= RWMAX;
  lWheel = LWMAX;
  Chassis::state().write(0,0);
  Chassis::state().move();
}

void debug() {
  char key;
  Output::screen().parse(
      "b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}");
  while (1) {
    if ((key = Input::device().getKey()) != NO_KEY) {
      switch (key) {
        case '1':
          ts();
          Output::screen().parse(
              "b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}");
          break;
        case '2':
          tg();
          Output::screen().parse(
              "b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}");
          break;
        case '3':
          tc();
          Output::screen().parse(
              "b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}");
          break;
        case '4':
          tm();
          Output::screen().parse(
              "b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}");
          break;
         case '5':
         tss();
          Output::screen().parse(
              "b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}");
          break;
         case '6':
         singleMove();
         Output::screen().parse(
              "b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}");
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
