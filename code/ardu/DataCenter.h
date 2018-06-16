#ifndef DATACENTER_H
#define DATACENTER_H



class DataCenter {

  public:
  static DataCenter& get(){
      static DataCenter d;
      return d;
  }

  void reset();
  int val(unsigned char no);
  void write(unsigned char no, int val);
  void load();
  void save();

  static const unsigned char RWMAX = 0;
  static const unsigned char LWMAX = 1;
  static const unsigned char MINTURNDIS = 2;
  static const unsigned char TURNDELAY = 3;
  static const unsigned char LOSTDEBUFF = 4;
  static const unsigned char REVADJSPEED = 5;
  static const unsigned char SDISUPBOUND = 6;
  static const unsigned char SDISLOWBOUND = 7;
  static const unsigned char ADISLOWLIM= 8;
  static const unsigned char ADISRANDOM = 9;
  static const unsigned char QDISRANDOM = 10;
  static const unsigned char QDISRANDOMS = 11;
  static const int M =12;
  
  private:
  DataCenter(){}
  

  //if needs new data, alter the upper limit
  int dataArr[M] = {0};

  
};

#endif
