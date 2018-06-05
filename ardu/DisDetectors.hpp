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
  unsigned long lastReadTime = 0;
  const unsigned int interval = 100; //read those sensors every 100ms

  bool timeOut(){
    unsigned long t = millis();
    return t - lastReadTime > interval;
  }  

  void setlastReadTime(const unsigned long t){
    lastReadTime = t;
  }

  void detect() {
    for (int i = 0; i < N; i++) {
      dis[i] = detectSingle(pinSets[i][0], pinSets[i][1]);
    }
    rd = true;
  }

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
  double operator[](const unsigned int i){
    return get(i);
  }

  double get(const unsigned int Num) { 
    if(timeOut() || !ready()){
      detect();
      setlastReadTime(millis());
    }
    return dis[Num]; 
  }
};

#endif
