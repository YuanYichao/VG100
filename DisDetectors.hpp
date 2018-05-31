#ifndef DISDETECTORS
#define DISDETECTORS

#include <arduino.h>

template <int N>
class DisDetectors {
  unsigned char pinSets[N][2] = {{0}};
  double dis[N] = {0};
  double detectSingle(const unsigned char trig, const unsigned char echo) {
    //给一个高电平脉冲
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    //读取时间并输出距离
    double distance_time = pulseIn(echo, HIGH);
    double distance_length = distance_time / 58;  //微秒转化为cm
    return distance_length;
  }
  bool rd = false;

 public:
  bool ready() { return rd; }
  void attach(unsigned char pins[N][2]) {
    Serial.println("attach");
    for (int i = 0; i < N; i++) {
      Serial.print("a: two pins are ");
      Serial.print(pins[i][0]);
      Serial.println(pins[i][1]);
      pinMode(pins[i][0], OUTPUT);
      pinMode(pins[i][1], INPUT);
      pinSets[i][0] = pins[i][0];
      pinSets[i][1] = pins[i][1];
    }
  }
  void detect() {
    Serial.println("detect");
    for (int i = 0; i < N; i++) {
      Serial.print("d: two pins are ");
      Serial.print(pinSets[i][0]);
      Serial.println(pinSets[i][1]);
      dis[i] = detectSingle(pinSets[i][0], pinSets[i][1]);
    }
    rd = true;
  }
  double get(const unsigned char pinNum) { 
    Serial.println("get");
    return dis[pinNum]; 
  }
};

#endif
