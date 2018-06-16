#include "Input.h"

int Input::getInt(){
    Output::screen().parse("c {input Int, use # to end:}");
    char key;
    int val = 0;
    while(1){
        key = getKey();
        if(key == '#') return val;
        if(key >= '0' && key <= '9'){
            val *= 10;
            val += key - '0';
            Output::screen().print(val, 2);
        }
    }
}
