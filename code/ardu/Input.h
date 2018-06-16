#ifndef INPUT_H
#define INPUT_H
#include <Keypad.h>
#include "Output.h"

static const byte ROWS = 4;
static const byte COLS = 4;
static char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};
static byte rowPins[ROWS] = {38, 40, 42, 44};

static byte colPins[COLS] = {39, 41, 43, 45};

class Input{
    Input(char** keys, byte* rowPins, byte* colPins, const byte ROWS, const byte COLS)
    : keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS){
    }
    Keypad keypad;

    Input(const Input&) = delete;
    Input &operator=(const Input&) = delete;

    public:
    static Input& device(){
        static Input i((char **)keys,(byte *) rowPins, (byte *)colPins, ROWS, COLS);
        return i;
    }
    char getKey(){return keypad.getKey();}
    int getInt();
};

#endif
