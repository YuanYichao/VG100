#ifndef RECORDER
#define RECORDER

struct InfoData {
  double l = -1;
  double w = -1;
  double sensorDis = -1;
  double photoDis = -1;
  double turnDis = -1;
  InfoData(){}
};

class Recorder {
  Recorder(){}
  Recorder(const Recorder &) = delete;
  Recorder &operator=(const Recorder &) = delete;
  static constexpr int SZ = sizeof(InfoData);
  union dataConvert {
    dataConvert(){}
    InfoData v;
    unsigned char b[SZ];
  };
  dataConvert ddata;
  bool rd[5] = {0};

  public:
   static Recorder& disk(){
     static Recorder r;
     return r;
   }
   InfoData readRecord();
   void record(const InfoData& data);
};

#endif
