#include <stdio.h>
#include "Chassis.h"
#include "DataCenter.h"
#include "DisDetectors.hpp"
#include "Input.h"
#include "OpenMV.h"
#include "Output.h"

char buf[256];
const double RANGE = 100;

#define RIGHT 1
#define LEFT 0
#define UNDEFINEDDIR -1
#define AWMAX 255

// f, r1, r2, l1, l2
#define DISNUM 5
DisDetectors<DISNUM> dis;
unsigned char disPins[DISNUM][2] = {
    {24, 25}, {26, 27}, {28, 29}, {30, 31}, {32, 33}};

unsigned char rWheel = 0, lWheel = 0;

/////////////////////////////////////////////////////////
//-------------- for the running task-----------------//
////////////////////////////////////////////////////////
int avgDisRight() { return (dis[1] + dis[2]) / 2; }

int avgDisLeft() { return (dis[3] + dis[4]) / 2; }

void adjustAngle(int dir) {
  if (dir == RIGHT) {
    Chassis::state().write(-255, 255);
  } else {
    Chassis::state().write(255, -255);
  }
  Chassis::state().move();
}

void fourSensorsStraight() {
  Output::screen().print("FourStraight", 3);
  const int RWMAX = DataCenter::get().val(DataCenter::RWMAX);
  const int LWMAX = DataCenter::get().val(DataCenter::LWMAX);
  const int ADISLOWLIM = DataCenter::get().val(DataCenter::ADISLOWLIM);
  const int QDISRANDOM = DataCenter::get().val(DataCenter::QDISRANDOM);
  const int QDISRANDOMS = DataCenter::get().val(DataCenter::QDISRANDOMS);
  rWheel = RWMAX;
  lWheel = LWMAX;
  int avgR = avgDisRight();
  int avgL = avgDisLeft();
  if (!withinError(avgR, avgL, QDISRANDOM)) {
    if (avgR > avgL) {
      rWheel -= (avgR - avgL) - QDISRANDOM;
    } else {
      lWheel -= (avgL - avgR) - QDISRANDOM;
    }
  }
  int dRb = dis[2] + dis[3];
  int dRf = dis[1] + dis[4];
  if (!withinError(dRb, dRf, QDISRANDOMS)) {
    if (dRf > dRb) {
      rWheel -= (dRf - dRb) - QDISRANDOMS;
    } else {
      lWheel -= (dRb - dRf) - QDISRANDOMS;
    }
  }
  if (dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM) {
    adjustAngle(LEFT);
  }
  if (dis[3] < ADISLOWLIM || dis[4] < ADISLOWLIM) {
    adjustAngle(RIGHT);
  }
  Chassis::state().write(rWheel, lWheel);
}

void dualSensorsRight() {
  Output::screen().print("DualRStraight", 3);
  const int RWMAX = DataCenter::get().val(DataCenter::RWMAX);
  const int LWMAX = DataCenter::get().val(DataCenter::LWMAX);
  const int ADISLOWLIM = DataCenter::get().val(DataCenter::ADISLOWLIM);
  const int SDISUPBOUND = DataCenter::get().val(DataCenter::SDISUPBOUND);
  const int SDISLOWBOUND = DataCenter::get().val(DataCenter::SDISLOWBOUND);
  const int ADISRANDOM = DataCenter::get().val(DataCenter::ADISRANDOM);
  lWheel = LWMAX;
  rWheel = RWMAX;
  if (withinError(dis[1], dis[2], ADISRANDOM))
    ;
  if (dis[1] > dis[2]) {
    rWheel = RWMAX - (dis[1] - dis[2]);
  } else if (dis[1] < dis[2]) {
    lWheel = LWMAX - (dis[2] - dis[1]);
  }
  if (dis[1] + dis[2] > SDISUPBOUND) {
    rWheel -= (dis[1] + dis[2]) - SDISUPBOUND;
  }
  if (dis[1] + dis[2] < SDISLOWBOUND) {
    lWheel -= SDISLOWBOUND - (dis[1] + dis[2]);
  }
  if (dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM) {
    adjustAngle(LEFT);
  }
  Chassis::state().write(rWheel, lWheel);
}

void dualSensorsLeft() {
  Output::screen().print("DualLStraight", 3);
  const int RWMAX = DataCenter::get().val(DataCenter::RWMAX);
  const int LWMAX = DataCenter::get().val(DataCenter::LWMAX);
  const int ADISLOWLIM = DataCenter::get().val(DataCenter::ADISLOWLIM);
  const int SDISUPBOUND = DataCenter::get().val(DataCenter::SDISUPBOUND);
  const int SDISLOWBOUND = DataCenter::get().val(DataCenter::SDISLOWBOUND);
  const int ADISRANDOM = DataCenter::get().val(DataCenter::ADISRANDOM);
  lWheel = LWMAX;
  rWheel = RWMAX;
  if (withinError(dis[3], dis[4], ADISRANDOM))
    ;
  if (dis[4] > dis[3]) {
    rWheel = RWMAX - (dis[4] - dis[3]);
  } else if (dis[4] < dis[3]) {
    lWheel = LWMAX - (dis[3] - dis[4]);
  }
  if (dis[3] + dis[4] > SDISUPBOUND) {
    lWheel -= (dis[3] + dis[4]) - SDISUPBOUND;
  }
  if (dis[3] + dis[4] < SDISLOWBOUND) {
    rWheel -= SDISLOWBOUND - (dis[3] + dis[4]);
  }
  if (dis[3] < ADISLOWLIM || dis[4] < ADISLOWLIM) {
    adjustAngle(RIGHT);
  }
  Chassis::state().write(rWheel, lWheel);
}

void searchSignal() {
  const int RWMAX = DataCenter::get().val(DataCenter::RWMAX);
  const int LWMAX = DataCenter::get().val(DataCenter::LWMAX);
  const int LOSTDEBUFF = DataCenter::get().val(DataCenter::LOSTDEBUFF);
  const int ADISLOWLIM = DataCenter::get().val(DataCenter::ADISLOWLIM);
  Chassis::state().write(RWMAX - LOSTDEBUFF, LWMAX - LOSTDEBUFF);

  if (dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM) adjustAngle(LEFT);
  if (dis[3] < ADISLOWLIM || dis[4] < ADISLOWLIM) adjustAngle(RIGHT);
}

void goStraight() {
  dis.detect();
  if (dis.normal(1) && dis.normal(2) && dis.normal(3) && dis.normal(4)) {
    fourSensorsStraight();
  } else if (dis.normal(1) && dis.normal(2)) {
    dualSensorsRight();
  } else if (dis.normal(3) && dis.normal(4)) {
    dualSensorsLeft();
  } else {
    searchSignal();
  }
  Chassis::state().move();
}

bool withinError(long dis1, long dis2, long Merror) {
  int error = abs(dis1 - dis2);
  return error < Merror;
}

int getDirection() {
  // int d = OpenMV::getDir();
  int d = UNDEFINEDDIR;
  if (d == UNDEFINEDDIR) {
    if (dis[1] > dis[3]) {
      return RIGHT;
    } else if (dis[1] < dis[3]) {
      return LEFT;
    } else {
      return UNDEFINEDDIR;
    }
  } else {
    return d;
  }
}

void aftAdjust(int dir) {
  const int REVADJSPEED = DataCenter::get().val(DataCenter::REVADJSPEED);
  if (dir == RIGHT) {
    if (dis.normal(3) && dis.normal(4)) return;
    Chassis::state().write(REVADJSPEED, -REVADJSPEED);
    Chassis::state().move();
    while (!dis.normal(3) || !dis.normal(4)) continue;
  } else {
    if (dis.normal(1) && dis.normal(2)) return;
    Chassis::state().write(-REVADJSPEED, REVADJSPEED);
    Chassis::state().move();
    while (!dis.normal(1) || !dis.normal(2)) continue;
  }
}

void doTurn() {
  Output::screen().print("doTurn", 3);
  const int RWMAX = DataCenter::get().val(DataCenter::RWMAX);
  const int LWMAX = DataCenter::get().val(DataCenter::LWMAX);
  const int TURNDELAY = DataCenter::get().val(DataCenter::TURNDELAY);
  static const int high = AWMAX, low = -AWMAX;
  int dir = getDirection();
  while (dir == UNDEFINEDDIR) dir = getDirection();
  if (dir == RIGHT) {
    Chassis::state().write(low, high);
  } else {
    Chassis::state().write(high, low);
  }
  Chassis::state().move();
  delay(TURNDELAY);
  aftAdjust(dir);
  Chassis::state().write(RWMAX, LWMAX);
  Chassis::state().move();
}

/////////////////////////////////////////////////////////
//-------------- for the tasks in MENU-----------------//
////////////////////////////////////////////////////////

void run() {
  Output::screen().clear();
  const int MINTURNDIS = DataCenter::get().val(DataCenter::MINTURNDIS);  
  while (1) {
    sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&;}", dis[0], dis[1], dis[2], dis[3],
            dis[4]);
    Output::screen().parse(buf);
    if (dis[0] < MINTURNDIS) doTurn();
    goStraight();
  }
}

void tune() {
  Output::screen().parse("c b{1. get disPhoto; 2. get disTurn;A to return}");
  char key;
  while (1) {
    key = Input::device().getKey();
    if (key != NO_KEY) {
      switch (key) {
        case '1':
          break;
        case '2':
          break;
        case 'A':
          return;
        default:
          break;
      }
    }
  }
}

void reset() {
  Output::screen().parse(
      ("c p {This process is "
       "not recoverable!} d c {A to return} d"));
  char key;
  while ((key = Input::device().getKey()) == NO_KEY) continue;
  if (key == 'A')
    return;
  else {
    DataCenter::get().reset();
    DataCenter::get().save();
  }
  Output::screen().parse("c p {the EEPROM has been reset} d d");
}

void about() {
  static const char* str[] = {"RWMAX",       "LWMAX",        "MINTURNDIS",
                             "TURNDELAY",   "LOSTDEBUFF",   "REVADJSPEED",
                             "SDISUPBOUND", "SDISLOWBOUND", "ADISLOWLIM",
                             "ADISRANDOM",  "QDISRANDOM",   "QDISRANDOMS"};
  Output::screen().parse("c p {following are the data}d");
  for (int i = 0; i < DataCenter::M; i++) {
    Output::screen().clear();
    Output::screen().print(str[i], 1);
    Output::screen().print(DataCenter::get().val(i));
    delay(1000);
  }
}

/////////////////////////////////////////////////////////////
//-------------- for the tasks in sub MENU-----------------//
/////////////////////////////////////////////////////////////

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
  Output::screen().print("ready", 3);
  while (1) {
    key = Input::device().getKey();
    if (key == 'A') return;
    if (key == 'B') {
      int ti = 0;
      while ((key = Input::device().getKey()) == NO_KEY)
        ;
      ti += (key - '0') * 100;
      while ((key = Input::device().getKey()) == NO_KEY)
        ;
      ti += (key - '0') * 10;
      Output::screen().clear();
      Output::screen().print(ti);
      Chassis::state().write(-255, 255);
      Chassis::state().move();
      delay(ti);
      Output::screen().print("ready", 3);
      Chassis::state().write(0, 0);
      Chassis::state().move();
    }
  }
}

void tss() {
  char key = 0;
  unsigned char st;
  Output::screen().parse("c {A to break}");
  while (1) {
    key = Input::device().getKey();
    st = dis.curState();
    if (key == 'A') break;
    if (key >= '0' && key <= '4') {
      dis.avlb(key - '0');
      Serial.print("enter");
    }
    sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&;}", dis[0], dis[1], dis[2], dis[3],
            dis[4]);
    Output::screen().parse(buf);
  }
}

void singleMove() {
  run();
}
#define DEBUGMENU "c b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}"

void debug() {
  char key;
  Output::screen().parse(DEBUGMENU);
  while (1) {
    if ((key = Input::device().getKey()) != NO_KEY) {
      switch (key) {
        case '1':
          ts();
          Output::screen().parse(DEBUGMENU);
          break;
        case '2':
          tg();
          Output::screen().parse(DEBUGMENU);
          break;
        case '3':
          tc();
          Output::screen().parse(DEBUGMENU);
          break;
        case '4':
          tm();
          Output::screen().parse(DEBUGMENU);
          break;
        case '5':
          tss();
          Output::screen().parse(DEBUGMENU);
          break;
        case '6':
          singleMove();
          Output::screen().parse(DEBUGMENU);
          break;
        default:
          return;
      }
    }
  }
  return;
}

#define MENU "c b {1.run&2.tune;3.reset&4.about;5.debug&;}"

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;
  dis.attach(disPins);
  DataCenter::get().load();
  Output::screen().parse(MENU);
}

void loop() {
  char key = Input::device().getKey();
  switch (key) {
    case '1':
      run();
      Output::screen().parse(MENU);
      break;
    case '2':
      tune();
      Output::screen().parse(MENU);
      break;
    case '3':
      reset();
      Output::screen().parse(MENU);
      break;
    case '4':
      about();
      Output::screen().parse(MENU);
      break;
    case '5':
      debug();
      Output::screen().parse(MENU);
    default:
      break;
  }
}
