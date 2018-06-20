#ifndef DISDETECTORS
#define DISDETECTORS

#include <arduino.h>
#include "DataCenter.h"

#define UNNORMALSIDE 2000
#define UNNORMALFRONT 1500
#define UNNORMALFOR 4000


template <int N>
class DisDetectors {
  unsigned char pinSets[N][2] = {{0}};
  long dis[N] = {0};
  long detectSingle(const unsigned char trig, const unsigned char echo) {
    //给一个高电平脉冲
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    //读取时间并输出距离
    double distance_time = pulseIn(echo, HIGH, 50000);
    long distance_length = distance_time / 58 * 100;
    if (distance_length ==0) return -1;
    return distance_length;
  }
  bool rd = false;
  unsigned long lastReadTime = 0;
  const unsigned int interval = 20;
  unsigned char avalBits = 0xff;

  bool timeOut() {
    unsigned long t = millis();
    return t - lastReadTime > interval;
  }

  void setlastReadTime(const unsigned long t) { lastReadTime = t; }

 public:
  bool ready() { return rd; }
  void attach(unsigned char (&pins)[N][2]) {
    for (int i = 0; i < N; i++) {
      pinMode(pins[i][0], OUTPUT);
      pinMode(pins[i][1], INPUT);
      pinSets[i][0] = pins[i][0];
      pinSets[i][1] = pins[i][1];
    }
  }

  void detect() {
    for (int i = 0; i < N; i++) {
      if (avalBits & (1 << i)) {
        dis[i] = detectSingle(pinSets[i][0], pinSets[i][1]);
      }
    }
    rd = true;
  }

  long operator[](const unsigned int i) { return get(i); }

  long get(const unsigned int Num) {
    if (!(avalBits & (1 << Num))) return -1;
    if (timeOut() || !ready()) {
      detect();
      setlastReadTime(millis());
    }
    return dis[Num];
  }

  void state(unsigned char bits) { avalBits = bits; }

  void allOn() { avalBits = 0xff; }

  unsigned char curState() { return avalBits; }

  void avlb(int num) { avalBits ^= (1 << num); }

  bool normal(int num) {
    long dis = get(num);
    //------
    if (num == 0) return dis > 0 && dis <= UNNORMALFRONT;
    if (num == 5 || num == 6) return dis > 0 && dis <= UNNORMALFOR;
    return dis > 0 && dis <= UNNORMALSIDE;
  }
};

#endif
