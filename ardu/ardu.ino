#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define ROWNUM 4
#define COLNUM 20

using byte = unsigned char;

union doubleData {
  double v;
  unsigned char b[4];
};

doubleData ddata;

double info[5] = {0};
bool rd[5] = {0};
char buf[256] = {0};

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};

byte rowPins[ROWS] = {38, 40, 42, 44};

byte colPins[COLS] = {39, 41, 43, 45};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal lcd(50, 52, 53, 51, 49, 47);


int mmin(int a, int b) { return a < b ? a : b; }

bool iswhite(char c){
    return c == ' ' || c == '\t' || c == '\n';
}

int mstrfind(const char *src, char target) {
  for (int i = 0; src[i] ; i++) {
    if (src[i] == target) return i;
  }
  return -1;
}

void clearBuf(){
  buf[0] = 0;
}


void clearLine(const unsigned char line) {
  for (int i = 0; i < COLNUM; i++) {
    lcd.setCursor(i, line);
    lcd.print(" ");
  }
}

template <typename T>
void print(T v, const unsigned char row = 0, bool strict = false) {
    clearLine(row);
    lcd.setCursor(0, row);
    lcd.print(v);
}

void print(const char* str, const unsigned char row = 0) {
  for(int i = 0; str[i] && i < COLNUM * ROWNUM; i++){
    lcd.setCursor(i % COLNUM, i / COLNUM);
    lcd.print(str[i]);
  }
}

void printerr(const int code, const char* str) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR: CODE ");
  lcd.setCursor(12, 0);
  lcd.print(code);
  lcd.setCursor(0, 1);
  print(str, 1);
  while (1) continue;
}

int matchblock(const char* src) {
  int j = 0;
  while (src[j] != '}' && src[j]) j++;
  strncpy(buf, src, j);
  buf[j] = 0;
  return j;
}

void parse(const char* src) {
  for (int i = 0; src[i]; i++) {
    if (iswhite(src[i])) continue;
    if (src[i] == 'p') {
      while (src[i] != '{') i++;
      i++;
      i += matchblock(src + i);
      printPages(buf);
      continue;
    }
    if (src[i] == 'b') {
      while (src[i] != '{') i++;
      i++;
      i += matchblock(src + i);
     print(i, 2);
     print(src[i], 3);
      printBlock(buf);
      continue;
    }
    if (src[i] == '{') {
      i++;
      i += matchblock(src + i);
      print(buf);
      continue;
    }
    if (src[i] == 'd') {
      delay(2000);
      continue;
    }
    if (src[i] == 'c') {
      lcd.clear();
      continue;
    }

  }
}

void printPages(const char* src) {
  lcd.clear();
  if (strlen(src) <= ROWNUM * COLNUM) {
    print(src);
  } else {
    for (int i = 0; i < strlen(src); i += ROWNUM * COLNUM) {
      lcd.clear();
      print(src + i);
      delay(3000);
    }
  }
}

void printBlock(const char* str) {
  int cc, cr , i ,j, cw , cup, t;
  for (cc = 1, i = 0; i < strlen(str) && str[i] != ';'; i++) {
    if (str[i] == '&') cc++;
  }
  for(cr = 0, i = 0; str[i] ; i++){
    if (str[i] == ';') cr++;
  }
  cw = COLNUM / cc;
  cup = 0;
  lcd.clear();
  for (i = 0; i < cr; i++) {
    for (j = 0; j < cc; j++) {
      if (j == cc - 1) {
        t = mstrfind(str + cup, ';');
      }else{
        t = mstrfind(str + cup, '&');
      }
      strncpy(buf, str + cup, t);
      buf[t] = 0;
      lcd.setCursor(j * cw, i);
      lcd.print(buf);
      clearBuf();
      cup += t + 1;
    }
  }
}

bool comfirmDouble(double d){
  parse(" c {You entered:} d c");
  print(d);
  parse(" {is that right? (1/0)}");
  char key;
  while(1){
    key = keypad.getKey();
    if(key != NO_KEY) break;
  }
  return key == '1';
}

bool confirmStr(const char * str){
  parse(str);
  parse("c {Sure? (1/0)}");
  char key;
  while(1){
    key = keypad.getKey();
    if(key != NO_KEY) break;
  }
  return key == '1';
}

void readRecord(){
  for(int i = 0; i < 5; i++){
    rd[i] = EEPROM.read(i* 5);
    for(int j = 0; j < 4; j++){
      ddata.b[j] = EEPROM.read(i* 5 + 1 +j);
    }
    info[i] = ddata.v;
  }
}

void record(){
  for(int i = 0; i < 5; i++){
    EEPROM.write(i* 5, rd[i]);
    ddata.v = info[i];
    for(int j = 0; j < 4; j++){
      EEPROM.write(i* 5 + 1 +j, ddata.b[j]);
    }
  }
}

void run() {
  parse("p {hello world, this is a long part of words, use p to display it in seveal pages, just like this, now the display is bigger...} d dddd");
}

void tune() {
  for(int i = 0; i< 5; i++){
    rd[i] = true;
  }
  info[0] = 11.5;
  info[1] = 11.1;
  info[2] = 12.1;
  info[3] = 13.4;
  info[4] = 8.9;
  record();
}

void reset() {
  parse("c {reset} d");
  if(confirmStr("c p{you are going to reset the EEPROM.} c p {This process is not recoverable!} d"))
  for (int i = 0; i < EEPROM.length(); i++) {
    if(EEPROM.read(i)!=0) EEPROM.write(i, 0);
  }
  else return;
  parse("c p {the EEPROM has been reset} d d");
}

void about() {
  parse("c p {following are the data}  d c p{dp, dt, dw, w, l} d c");
  for(int i = 0; i < 5; i++){
    if(rd[i]){
    print(info[i]);
    delay(1000);
    lcd.clear();
    }
    else
    parse("c {no such info} d");
  }
}

void setup() { 
  Serial.begin(9600);
    lcd.begin(COLNUM, ROWNUM);
    readRecord();
    while(!Serial);
    parse("b {1.run&2.tune;3.reset&4.about;}");
}

void loop() {
  char key = keypad.getKey();
  switch (key) {
    case '1':
      run();
      parse("c {quit} d b {1.run&2.tune;3.reset&4.about;}");
      break;
    case '2':
      tune();
      parse("c {quit} d b {1.run&2.tune;3.reset&4.about;}");
      break;
    case '3':
      reset();
      parse("c {quit} d b {1.run&2.tune;3.reset&4.about;}");
    case '4':
      about();
      parse("c {quit} d b {1.run&2.tune;3.reset&4.about;}");
    default:
      break;
  }
  
}
