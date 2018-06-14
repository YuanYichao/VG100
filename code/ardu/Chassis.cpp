#include "Chassis.h"
#include <arduino.h>

void Chassis::move(){
    if(rPower > 0){
        digitalWrite(R1, HIGH);
        digitalWrite(R2, LOW);
    }else if(rPower < 0){
        digitalWrite(R1, LOW);
        digitalWrite(R2, HIGH);
    }else{
        digitalWrite(R1, LOW);
        digitalWrite(R2, LOW);
    }
    if (lPower > 0) {
      digitalWrite(L1, HIGH);
      digitalWrite(L2, LOW);
    } else if (lPower < 0) {
      digitalWrite(L1, LOW);
      digitalWrite(L2, HIGH);
    } else {
      digitalWrite(L1, LOW);
      digitalWrite(L2, LOW);
    }
    int rp = abs(rPower);
    int lp = abs(lPower);
    analogWrite(ENR, rp);
    analogWrite(ENL, lp);
}

void Chassis::write(int rP, int lP){
    rPower = rP;
    lPower = lP;
}
