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

// f, r1, r2, l1, l2
#define DISNUM 5
DisDetectors<DISNUM> dis;
unsigned char disPins[DISNUM][2] = {
    {24, 25}, {26, 27}, {28, 29}, {30, 31}, {32, 33}};

InfoData info;
const unsigned char RWMAX = 255, LWMAX = 255;
unsigned char rWheel = RWMAX, lWheel = LWMAX;

const long MINTURNDIS = 600;
const int TURNDELAY = 270;
const int AFTDELAY = 1000;

bool unready() { return info.photoDis < 0 || info.turnDis < 0; }
// for the running task

int avgDisRight() { return (dis[1] + dis[2]) / 2; }

int avgDisLeft() { return (dis[3] + dis[4]) / 2; }

int avgSideWidth() { return (avgDisRight() + avgDisLeft()) / 2; }

int turnOuterRadius() { return dis[0] + info.l - avgSideWidth(); }

int turnInnerRadius() { return turnOuterRadius() - info.w; }

// positive to right, neg to left
int unnormalSingle() {
  if (abs((dis[1] - dis[2])) > RANGE)
    return dis[1] - dis[2];
  else
    return 0;
}

int unnormalDouble() {
  if (abs(avgDisLeft() - avgDisRight()) > RANGE)
    return avgDisRight() - avgDisLeft();
  else
    return 0;
}

bool unnormal() { return unnormalSingle() || unnormalDouble(); }

bool hasFixedS() { return abs(dis[1] - dis[2]) < RANGE; }

bool hasFixedD() { return abs(avgDisLeft() - avgDisRight()) < RANGE; }

void adjustAngle(){
  int dir = unnormalSingle();
  int low = -250, high = 250;
  if(dir >0){
    Chassis::state().write(low, high);
  }else{
    Chassis::state().write(high, low);
  }
  Chassis::state().move();
}

void fourSensorsStraight(){
  Output::screen().print("FourStraight",3);
  static const int INTERVAL = 5, STEPVAL = 100;
  int dir = 0, step = 0;
  // start a fixing task
  if(!unnormalSingle() && !unnormalDouble()){
    rWheel = RWMAX;
    lWheel = LWMAX;
    Chassis::state().write(rWheel, lWheel);
  }
  if (unnormalSingle()) {
    dir = unnormalSingle();
    if(abs(dir) > 250) {
      adjustAngle();
      return;
    }
    if (dir > 0) {
      step += STEPVAL; 
    }
    else{
      step -= STEPVAL;
    }
  }
  if (unnormalDouble()) {
    dir = unnormalDouble();

    if (dir > 0) {
      step += STEPVAL;
    } else {
      step -= STEPVAL;
    }
  }
  if(step >0){
    rWheel = RWMAX - step;
  }
  else{
    lWheel = LWMAX - abs(step);
  }
  Chassis::state().write(rWheel, lWheel);
}

void dualSensorsFront(){
  Output::screen().print("DualFStraight",3);
  if(dis[1] > dis[3]){
    rWheel = RWMAX - 80;
  }else if(dis[1] < dis[3]){
    lWheel = LWMAX - 80;
  }
  Chassis::state().write(rWheel, lWheel);
}

void dualSensorsBack(){
  Output::screen().print("DualBFStraight",3);
  if(dis[2] > dis[4]){
    rWheel = RWMAX - 80;
  }else if(dis[2] < dis[4]){
    lWheel = LWMAX - 80;
  }
  Chassis::state().write(rWheel, lWheel);
}


void goStraight() {
   dis.detect();
   if(dis.normal(1) && dis.normal(2) && dis.normal(3) && dis.normal(4)){
    fourSensorsStraight();
   }else if(dis.normal(1) && dis.normal(3)){
    dis.avlb(2);
    dis.avlb(4);
    dualSensorsFront();
    dis.avlb(2);
    dis.avlb(4);
   }else if(dis.normal(2) && dis.normal(4)){
    dis.avlb(1);
    dis.avlb(3);
    dualSensorsBack();
    dis.avlb(1);
    dis.avlb(3);
   }
   Chassis::state().move();
}

bool withinError(long dis1, long dis2, long Merror) {
  int error = abs(dis1 - dis2);
  return error < Merror;
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
//  Output::screen().print("FreeTurn",3);
//  long avgW = 700;
//  while (dis[1] < 2000 && dis[3] < 2000){
//     sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&%d;}",dis[0] , dis[1], dis[2], dis[3], dis[4], -999);
//     Output::screen().parse(buf);
//  }
//  int dir = dis[1] > dis[3] ? dir = 1 : dir = 0;
//  Chassis::state().write(RWMAX, RWMAX);
//  while (dis[0] > MINTURNDIS){
//     sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&%d;}",dis[0] , dis[1], dis[2], dis[3], dis[4], -999);
//     Output::screen().parse(buf);
//  }
//  Output::screen().clear();Output::screen().print("ggggg",3);
//  long df = dis[0], cl = 1500, cw = 2200;
//  long disOut = cl + dis[0] - cw - 2 * avgW;
//  int high = LWMAX - 100;
//  int low = (high) * (double)(disOut + avgW)/(double)(disOut + avgW + cw);
//  if (dir > 0) {
//     Chassis::state().write(low, high);
//     Chassis::state().move();
//     while(!withinError(dis[3], dis[4], 300) || dis[4] > 1000 || dis[3] > 1000 || dis[0] <4000) {
//      sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&%d;}",dis[0] , dis[1], dis[2], dis[3], dis[4], low);
//     Output::screen().parse(buf);
//     }
//  } else {
//    Output::screen().clear();
//     Chassis::state().write(high, low);
//     Chassis::state().move();
//     while(!withinError(dis[1], dis[2], 300) ||  dis[1] > 1000 || dis[2] > 1000 || dis[0] <4000) {
//      sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&%d;}",dis[0] , dis[1], dis[2], dis[3], dis[4], low);
//     Output::screen().parse(buf);
//     }
//  }
//  Chassis::state().write(RWMAX, RWMAX);
     while (dis[0] > MINTURNDIS) continue;
     if(dis[1] > dis[3]){
      Chassis::state().write(-255,255);
     }else{
      Chassis::state().write(255, -255);
     }
     Chassis::state().move();
     delay(TURNDELAY);
     Chassis::state().write(255, 255);
     delay(AFTDELAY);    
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
  Output::screen().print("ready",3);
  while(1){
    key = Input::device().getKey();
    if(key == 'A') return;
    if(key == 'B'){
      int ti = 0;
      while((key = Input::device().getKey()) == NO_KEY); 
      ti += (key - '0') * 100;
      while((key = Input::device().getKey()) == NO_KEY); 
      ti += (key - '0') * 10; 
      Output::screen().clear();
      Output::screen().print(ti);
      Chassis::state().write(-255, 255);
      Chassis::state().move();
      delay(ti);
      Output::screen().print("ready",3);
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
     if(key >= '0' && key <= '4'){
      dis.avlb(key - '0');
      Serial.print("enter");
     }
     sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&;}",dis[0] , dis[1], dis[2], dis[3], dis[4]);
     Output::screen().parse(buf);
  }
}

void singleMove() {
//  Output::screen().parse("c {smove}");
//  long t = millis();
//  while (dis[0] > 2500) {
//    sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&;}",dis[0] , dis[1], dis[2], dis[3], dis[4]);
//    Output::screen().parse(buf);
//    goStraight();
//    Chassis::state().move();
//  }
//  rWheel = RWMAX;
//  lWheel = LWMAX;
//  doFreeTurn();
//  while (dis[0] > 2500) {
//    sprintf(buf, "c b{%ld&%ld;%ld&%ld;%ld&;}",dis[0] , dis[1], dis[2], dis[3], dis[4]);
//     Output::screen().parse(buf);
//    goStraight();
//    Chassis::state().move();
//  }
//  Chassis::state().write(0, 0);
//  Chassis::state().move();
    while(1){
      if(dis[0] < 3000) doFreeTurn();
      goStraight();
    }
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
