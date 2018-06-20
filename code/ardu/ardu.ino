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

#define RWMAX 100
#define LWMAX 100
#define MINTURNDIS 700
#define TURNDELAY 360
#define LOSTDEBUFF 100
#define REVADJSPEED 80
#define SDISUPBOUND 1500
#define SDISLOWBOUND 1200
#define ADISLOWLIM 500
#define ADISRANDOM 50
#define QDISRANDOM 50
#define QDISRANDOMS 100
#define PHOTODIS 1200
#define LINEARKDIS 5
#define LINEARKANGLE 10
#define TDELAY 0
#define FORTRIGDIS 1200
#define FORTRIGK 2
#define SPINSPEED 20
#define TURNEND 50
#define ALTMAX 75


// f, r1, r2, l1, l2
#define DISNUM 7
DisDetectors<DISNUM> dis;
unsigned char disPins[DISNUM][2] = {
    {24, 25}, {26, 27}, {28, 29}, {30, 31}, {32, 33}, {6, 7}, {4, 5}};

static const char* DMACROTABLE[] = {
    "RWMAX",        "LWMAX",         "MINTURNDIS",   "TURNDELAY",
    "LOSTDEBUFF",   "REVADJSPEED",   "SDISUPBOUND",  "SDISLOWBOUND",
    "ADISLOWLIM",   "ADISRANDOM",    "QDISRANDOM",   "QDISRANDOMS",
    "PHOTODIS",     "LINEARKDIS",    "LINEARKANGLE", "TDELAY",
    "UNNORMALSIDE", "UNNORMALFRONT", "UNNORMALFOR",  "FORTRIGDIS",
    "FORTRIGK",     "SPINSPEED", "TURNEND"};

unsigned char rWheel = 0, lWheel = 0;

/////////////////////////////////////////////////////////
//-------------- for the running task-----------------//
////////////////////////////////////////////////////////

bool withinError(long dis1, long dis2, long Merror) {
  int error = abs(dis1 - dis2);
  return error < Merror;
}

int avgDisRight() { return (dis[1] + dis[2]) / 2; }

int avgDisLeft() { return (dis[3] + dis[4]) / 2; }


void fourSensorsStraight() {
  Output::screen().print("four");
  // get some constant
  //---------------------core part-----------------------------------
  // set those wheels to full speed
  rWheel = RWMAX;
  lWheel = LWMAX;
  // get average distance of rhs and lhs
  int avgR = avgDisRight();
  int avgL = avgDisLeft();
  int t;
  // adjust based on distance
  if (!withinError(avgR, avgL, QDISRANDOM)) {
    if (avgR > avgL) {
      t = min(((avgR - avgL) - QDISRANDOM) / LINEARKDIS, ALTMAX)/2;
      lWheel += t;
      rWheel -= t;
    } else {
      t = min(((avgL - avgR) - QDISRANDOM) / LINEARKDIS, ALTMAX)/2;
      rWheel += t;
      lWheel -= t;
    }
  }
  // adjust based on angle;
  int dRb = dis[2] + dis[3];
  int dRf = dis[1] + dis[4];
  if (!withinError(dRb, dRf, QDISRANDOMS)) {
    if (dRf > dRb) {
      t = min(((dRf - dRb) - QDISRANDOMS) / LINEARKANGLE, ALTMAX) /2;
      rWheel -= t;
      lWheel += t;
    } else {
      t = min(((dRb - dRf) - QDISRANDOMS) / LINEARKANGLE, ALTMAX) /2;
      lWheel -= t;
      rWheel += t;
    }
  }
  //-----------------------------------------------------------------------
  // serious situation, currently ignore it!
  // if (dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM) {
  //   adjustAngle(LEFT);
  // }
  // if (dis[3] < ADISLOWLIM || dis[4] < ADISLOWLIM) {
  //   adjustAngle(RIGHT);
  // }
  Chassis::state().write(rWheel, lWheel);
}

void dualSensorsRight() {
  Output::screen().print("dr");
  // ------------------------core part --------------------------------
  // full speed
  lWheel = LWMAX;
  rWheel = RWMAX;
  int t;
  // adjust based on angle of right
  if (!withinError(dis[1], dis[2], ADISRANDOM)) {
    if (dis[1] > dis[2]) {
      t = min(((dis[1] - dis[2]) - ADISRANDOM) / LINEARKANGLE, ALTMAX)/2;
      rWheel -= t;
      lWheel += t;
    } else if (dis[1] < dis[2]) {
      t = min(((dis[1] - dis[2]) - ADISRANDOM) / LINEARKANGLE, ALTMAX)/2;
      lWheel -= t;
      rWheel += t;
    }
  }
   //adjust based on distantce
  if (dis[1] + dis[2] > SDISUPBOUND) {
    t = min(((dis[1] + dis[2]) - SDISUPBOUND) / LINEARKDIS, ALTMAX)/ 2;
    rWheel -= t;
    lWheel += t;
  }
  if (dis[1] + dis[2] < SDISLOWBOUND) {
    t = min((SDISLOWBOUND - (dis[2] + dis[1])) / LINEARKDIS, ALTMAX) /2;
    lWheel -= t;
    rWheel += t;
  }
  //----------------------------end------------------------------------
  // no sharp turn
  // if (dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM) {
  //   adjustAngle(LEFT);
  // }
  Chassis::state().write(rWheel, lWheel);
}

void dualSensorsLeft() {
  Output::screen().print("dl");
  //--------------------------core part----------------------------
  // full speed
  lWheel = LWMAX;
  rWheel = RWMAX;
  int t;
  // adjust based on angle of left
  if (!withinError(dis[3], dis[4], ADISRANDOM)) {
    if (dis[3] > dis[4]) {
      t = min(((dis[3] - dis[4]) - ADISRANDOM) / LINEARKANGLE, ALTMAX)/ 2;
      lWheel -= t;
      rWheel += t;
    } else if (dis[3] < dis[4]) {
      t = min(((dis[4] - dis[3]) - ADISRANDOM) / LINEARKANGLE, ALTMAX)/2 ;
      rWheel -= t;
      lWheel += t;
    }
  }
  // adjust based on distantce
  if (dis[3] + dis[4] > SDISUPBOUND) {
    lWheel -= min(((dis[3] + dis[4]) - SDISUPBOUND) / LINEARKDIS, 50);
  }
  if (dis[3] + dis[4] < SDISLOWBOUND) {
    rWheel -= min((SDISLOWBOUND - (dis[3] + dis[4])) / LINEARKDIS, 50);
  }
  //-----------------------------------------------------------------
  Chassis::state().write(rWheel, lWheel);
}

void alterSensorsFor() {
  //-----------core--------
  // right close
  if (dis[5] < FORTRIGDIS) {
    Output::screen().print("af");
    lWheel = 0;
    rWheel = 100;
  }
  if (dis[6] < FORTRIGDIS) {
    Output::screen().print("af");
    rWheel  = 0;
    lWheel = 100;
  }
  Chassis::state().write(rWheel, lWheel);
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
    // searchSignal();
    // do nothing now
  }
  alterSensorsFor();
  Chassis::state().move();
}

int getDirection() {
  int d = OpenMV::getDir();
  // int d = UNDEFINEDDIR;
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

void doTurn() {
  Output::screen().print("doTurn", 3);
  // const int TURNDELAY = DataCenter::get().val(DataCenter::TURNDELAY);
  const int high = SPINSPEED, low = -high;
  int dir = getDirection();
  while (dir == UNDEFINEDDIR) dir = getDirection();
  if (dir == RIGHT) {
    Chassis::state().write(low, high);
  } else {
    Chassis::state().write(high, low);
  }
  Chassis::state().move();
  //go to a close place first
  delay(TURNDELAY);
  //find the end condition
  if (dir == RIGHT) {
    dis.avlb(1);
    dis.avlb(2);
    dis.avlb(0);
    dis.avlb(5);
    dis.avlb(6);
    while(!withinError(dis[3], dis[4], TURNEND)) continue;
    dis.avlb(1);
    dis.avlb(2);
    dis.avlb(0);
    dis.avlb(5);
    dis.avlb(6);
  } else {
    dis.avlb(3);
    dis.avlb(4);
    dis.avlb(0);
    dis.avlb(5);
    dis.avlb(6);
    while (!withinError(dis[1], dis[2],TURNEND)) continue;
    dis.avlb(3);
    dis.avlb(4);
    dis.avlb(0);
    dis.avlb(5);
    dis.avlb(6);
  }
  Chassis::state().write(RWMAX, LWMAX);
  Chassis::state().move();
}

// void aftAdjust(int dir) {
//   const int REVADJSPEED = DataCenter::get().val(DataCenter::REVADJSPEED);
//   if (dir == RIGHT) {
//     if (dis.normal(3) && dis.normal(4)) return;
//     Chassis::state().write(REVADJSPEED, -REVADJSPEED);
//     Chassis::state().move();
//     while (!dis.normal(3) || !dis.normal(4)) continue;
//   } else {
//     if (dis.normal(1) && dis.normal(2)) return;
//     Chassis::state().write(-REVADJSPEED, REVADJSPEED);
//     Chassis::state().move();
//     while (!dis.normal(1) || !dis.normal(2)) continue;
//   }
// }

/////////////////////////////////////////////////////////
//-------------- for the tasks in tune-----------------//
////////////////////////////////////////////////////////

void changeMacro() {
  int num = 0;
  while (1) {
    Output::screen().clear();
    Output::screen().print("enter a number to select the MACRO");
    num = Input::device().getInt();
    if (num >= DataCenter::M) {
      Output::screen().parse("{Out of range!} d c");
      continue;
    }
    Output::screen().clear();
    Output::screen().print("you select: ");
    Output::screen().print(DMACROTABLE[num], 2);
    Output::screen().print("A to continue", 3);
    while (Input::device().getKey() != 'A')
      ;
    break;
  }
  Output::screen().clear();
  Output::screen().print("enter the value of the MACRO");
  int v = Input::device().getInt();
  DataCenter::get().write(num, v);
  DataCenter::get().save();
}

void getPhotodis() {
  Output::screen().clear();
  Output::screen().print("A to return, B to start a new detect.");
  char key;
  int dir = UNDEFINEDDIR;
  while (1) {
    key = Input::device().getKey();
    if (key == 'A') return;
    if (key == 'B') {
      OpenMV::endDetect();
      OpenMV::startDetect();
    }
    if (millis() % 50 > 40) {
      Output::screen().print(dis[0], 2);
      dir = OpenMV::getDir();
      if (dir == 1) {
        Output::screen().print("right", 3);
      } else if (dir == 0) {
        Output::screen().print("left", 3);
      } else {
        Output::screen().print("undefined", 3);
      }
    }
  }
}

/////////////////////////////////////////////////////////
//-------------- for the tasks in MENU-----------------//
////////////////////////////////////////////////////////

void run() {
  Output::screen().clear();
  while (1) {
    if (TDELAY) {
      Chassis::state().write(0, 0);
      Chassis::state().move();
      delay(TDELAY);
    }
    if (dis[0] < PHOTODIS && dis.normal(0)) {
      OpenMV::startDetect();
    }
//    if (dis[0] < MINTURNDIS && dis.normal(0)) {
//      doTurn();
//      OpenMV::endDetect();
//    }
    goStraight();
    // log
    sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&%ld;%ld&;}", dis[0], dis[1], dis[2],
            dis[3], dis[4], dis[5], dis[6]);
    Output::screen().parse(buf);
  }
}

#define TUNEMENU "c b{1.change MACRO;2.get PHOTODIS;A to return}"

void tune() {
  Output::screen().parse(TUNEMENU);
  char key;
  while (1) {
    key = Input::device().getKey();
    if (key != NO_KEY) {
      switch (key) {
        case '1':
          changeMacro();
          Output::screen().parse(TUNEMENU);
          break;
        case '2':
          Output::screen().parse(TUNEMENU);
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
  Output::screen().parse("c p {following are the data}d");
  for (int i = 0; i < DataCenter::M; i++) {
    Output::screen().clear();
    Output::screen().print(DMACROTABLE[i], 1);
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
    sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&%ld;%ld&;}", dis[0], dis[1], dis[2], dis[3],
            dis[4], dis[5], dis[6]);
    Output::screen().parse(buf);
  }
}

void singleMove() { run(); }
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
