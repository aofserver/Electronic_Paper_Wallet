#ifndef __Encryption_H__
#define __Encryption_H__

#include "Arduino.h"
#include <xxtea-lib.h>
#include <stdio.h>
#include <iostream>

class EncryptionClass {
    private:


    public:
        EncryptionClass();

        void begin();
        String Encrypt(String, String);
        String Decrypt(String, String);

};

extern EncryptionClass Encryption;

#endif