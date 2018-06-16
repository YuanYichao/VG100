#include <EEPROM.h>
#include <stdio.h>
#include "Chassis.h"
#include "DisDetectors.hpp"
#include "Input.h"
#include "OpenMV.h"
#include "Output.h"
#include "Recorder.h"

char buf[256];
const double RANGE = 100;

#define RIGHT 1
#define LEFT 0
#define UNDEFINEDDIR -1
#define AWMAX 255

#define RWMAX 255
#define LWMAX 255
#define MINTURNDIS 700
#define TURNDELAY 360
#define LOSTDEBUFF 100
#define REVADJSPEED 80
#define SDISUPBOUND 1400
#define SDISLOWBOUND 1200
#define ADISLOWLIM 500
#define ADISRANDOM 50
#define QDISRANDOM 50
#define QDISRANDOMS 100 

// f, r1, r2, l1, l2
#define DISNUM 5
DisDetectors<DISNUM> dis;
unsigned char disPins[DISNUM][2] = {
    {24, 25}, {26, 27}, {28, 29}, {30, 31}, {32, 33}};

InfoData info;

unsigned char rWheel = RWMAX, lWheel = LWMAX;

bool unready() { return info.photoDis < 0 || info.turnDis < 0; }
// for the running task

int avgDisRight() { return (dis[1] + dis[2]) / 2; }

int avgDisLeft() { return (dis[3] + dis[4]) / 2; }

void adjustAngle(int dir){
  if(dir == RIGHT){
    Chassis::state().write(-255, 255);
  }else{
    Chassis::state().write(255, -255);
  }
  Chassis::state().move();
}

void fourSensorsStraight() {
  Output::screen().print("FourStraight", 3);
  rWheel = RWMAX;
  lWheel = LWMAX;
  int avgR = avgDisRight();
  int avgL = avgDisLeft();
  if(!withinError(avgR, avgL, QDISRANDOM)){
    if(avgR > avgL){
      rWheel -= (avgR - avgL) - QDISRANDOM;
    }else {
      lWheel -= (avgL - avgR) - QDISRANDOM;
    }
  }
  int dRb = dis[2] + dis[3];
  int dRf = dis[1] + dis[4];
  if(!withinError(dRb, dRf, QDISRANDOMS)){
    if(dRf > dRb){
      rWheel -= (dRf - dRb) - QDISRANDOMS;
    }else {
      lWheel -= (dRb - dRf) - QDISRANDOMS;
    }
  }
  if(dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM){
    adjustAngle(LEFT);
  }
  if(dis[3] < ADISLOWLIM || dis[4] < ADISLOWLIM){
    adjustAngle(RIGHT);
  }
  Chassis::state().write(rWheel, lWheel);
}


void dualSensorsRight(){
  Output::screen().print("DualRStraight", 3);
  lWheel = LWMAX;
  rWheel = RWMAX;
  if(withinError(dis[1], dis[2], 50));
  if (dis[1] > dis[2]) {
    rWheel = RWMAX - (dis[1] - dis[2]);
  } else if (dis[1] < dis[2]) {
    lWheel = LWMAX - (dis[2] - dis[1]);
  }
  if(dis[1] + dis[2] > SDISUPBOUND){
    rWheel -= (dis[1] + dis[2]) - SDISUPBOUND;
  }
  if(dis[1] + dis[2] < SDISLOWBOUND){
    lWheel -= SDISLOWBOUND - (dis[1] + dis[2]);
  }
  if(dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM){
    adjustAngle(LEFT);
  }
  Chassis::state().write(rWheel, lWheel);
}

void dualSensorsLeft(){
  Output::screen().print("DualLStraight", 3);
  lWheel = LWMAX;
  rWheel = RWMAX;
  if(withinError(dis[3], dis[4], ADISRANDOM));
  if (dis[4] > dis[3]) {
    rWheel = RWMAX - (dis[4] - dis[3]);
  } else if (dis[4] < dis[3]) {
    lWheel = LWMAX - (dis[3] - dis[4]);
  }
  if(dis[3] + dis[4] > SDISUPBOUND){
    lWheel -= (dis[3] + dis[4]) - SDISUPBOUND;
  }
  if(dis[3] + dis[4] < SDISLOWBOUND){
    rWheel -= SDISLOWBOUND - (dis[3] + dis[4]);
  }
  if(dis[3] < ADISLOWLIM || dis[4] < ADISLOWLIM){
    adjustAngle(RIGHT);
  }
  Chassis::state().write(rWheel, lWheel);
}

void searchSignal(){
  Chassis::state().write(RWMAX - LOSTDEBUFF,LWMAX - LOSTDEBUFF);
  if(dis[1] < ADISLOWLIM || dis[2] < ADISLOWLIM) adjustAngle(LEFT);
  if(dis[3] < ADISLOWLIM || dis[4] < ADISLOWLIM) adjustAngle(RIGHT);
} 

void goStraight() {
  dis.detect();
  if (dis.normal(1) && dis.normal(2) && dis.normal(3) && dis.normal(4)) {
    fourSensorsStraight();  
  }else if(dis.normal(1) && dis.normal(2)){
    dualSensorsRight();
  } else if (dis.normal(3) && dis.normal(4)) {
    dualSensorsLeft();
  } else{
    searchSignal();
  }
  Chassis::state().move();
}

bool withinError(long dis1, long dis2, long Merror) {
  int error = abs(dis1 - dis2);
  return error < Merror;
}

int getDirection() {
  //int d = OpenMV::getDir();
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

void aftAdjust(int dir){
  if(dir == RIGHT){
    if(dis.normal(3) && dis.normal(4)) return;
    Chassis::state().write(REVADJSPEED, -REVADJSPEED);
    Chassis::state().move();
    while(!dis.normal(3) || !dis.normal(4)) continue;
  }else{
    if(dis.normal(1) && dis.normal(2)) return;
    Chassis::state().write(-REVADJSPEED, REVADJSPEED);
    Chassis::state().move();
    while(!dis.normal(1) || !dis.normal(2)) continue;
  }
}

void doTurn() {
  Output::screen().print("doTurn",3);
  static const int high = AWMAX, low = -AWMAX;
  int dir = getDirection();
  while(dir == UNDEFINEDDIR) dir = getDirection();
  if(dir == RIGHT){
    Chassis::state().write(low, high);
  }else{
    Chassis::state().write(high, low);
  }
  Chassis::state().move();
  delay(TURNDELAY);
  aftAdjust(dir);
  Chassis::state().write(RWMAX, LWMAX);
  Chassis::state().move();
}



void doRun() {
  bool sd = false, st = false;
  int dir = UNDEFINEDDIR;
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
      if (dir == UNDEFINEDDIR) {
        doTurn();
      } else {
        doTurn();
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
  while (1) {
    sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&;}", dis[0], dis[1], dis[2], dis[3],
            dis[4]);
    Output::screen().parse(buf);
    if (dis[0] < MINTURNDIS) doTurn();
    goStraight();
  }
}
#define DEBUGMENU "c b{1.serial&2.GPIO;3.command&4.moter;5.sensor&6.smove;}"

void debug() {
  char key;
  Output::screen().parse(
      DEBUGMENU);
  while (1) {
    if ((key = Input::device().getKey()) != NO_KEY) {
      switch (key) {
        case '1':
          ts();
          Output::screen().parse(DEBUGMENU
              );
          break;
        case '2':
          tg();
          Output::screen().parse(
              DEBUGMENU);
          break;
        case '3':
          tc();
          Output::screen().parse(
              DEBUGMENU);
          break;
        case '4':
          tm();
          Output::screen().parse(
              DEBUGMENU);
          break;
        case '5':
          tss();
          Output::screen().parse(
              DEBUGMENU);
          break;
        case '6':
          singleMove();
          Output::screen().parse(
              DEBUGMENU);
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
  info = Recorder::disk().readRecord();
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
      Output::screen().parse( MENU);
    default:
      break;
  }
}
