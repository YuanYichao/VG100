#include"openmv.h"
#include <arduino.h>

void OpenMV::startDetect(){
    Serial.print("start;");
}

int OpenMV::getDir(){
    char dir;
    Serial.print("dir;");
    while(!Serial.available());
    dir = Serial.read();
    switch(dir){
        case 'r':
        return 1;
        case 'l':
        return 0;
        default:
        return -1;
    }
}

void OpenMV::endDetect() { 
    Serial.print("end;");
}

void OpenMV::cdLen(){
    Serial.print("cd;");
}
