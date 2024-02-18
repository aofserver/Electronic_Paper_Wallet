#ifndef __Debug_H__
#define __Debug_H__

#include "global.h"
#include "Arduino.h"
#include <stdio.h>
#include <iostream>

class DebugClass {
    private:


    public:
        DebugClass();

        void begin();
        void Print(String);
        void PrintLine(String);
        void PrintLine(String,int,String);
};

extern DebugClass Debug;

#endif