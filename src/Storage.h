#ifndef __Storage_H__
#define __Storage_H__

#include "Arduino.h"
#include <stdio.h>
#include <iostream>
#include "FS.h"
#include "SPIFFS.h"

class StorageClass {
    private:


    public:
        StorageClass();
        void begin();
        int ASCIIHexToInt(char);
        String HexToString(String);
        String listDir(const char *dirname, uint8_t levels);
        void writeFile(const char * path, const char * message);
        String readFile(const char * path);
        void deleteFile(const char * path);
};

extern StorageClass Storage;

#endif