#ifndef CHASSIS_H
#define CHASSIS_H

#include <arduino.h>
/*
 *r1 -> 36 r2->37 enr -> 2 l1 -> 38 l2->39 enl -> 3
 */

class Chassis {
  Chassis(int r1, int r2, int enr, int l1, int l2, int enl) :R1(r1), R2(r2),
      ENR(enr), L1(l1), L2(l2), ENL(enl) {
          pinMode(R1, OUTPUT);
          pinMode(R2, OUTPUT);
          pinMode(L1, OUTPUT);
          pinMode(L2, OUTPUT);
          pinMode(ENR, OUTPUT);
          pinMode(ENL, OUTPUT);
}
  const int R1;
  const int R2;
  const int ENR;
  const int L1;
  const int L2;
  const int ENL;
  int rPower;
  int lPower;
  public:
  static Chassis& state(){
      static Chassis c(36, 37,2,38,39,3);
      return c;
  }
  void move();
  void write(int rP, int lP);
};

#endif
