#ifndef __ServerEsp_H__
#define __ServerEsp_H__
#include <WiFi.h>
#include "Arduino.h"
#include <WebServer.h>
#include "Storage.h"
#include <stdio.h>
#include <iostream>

class ServerEspClass {
    private:


    public:
        ServerEspClass();

        void begin();
        // void SetCors();
        // void Route();
        // void Listening();

        String Redirect();
        String PageHelp();
        String PageWallet(String);
        String PageAddWallet(String);
        String PageDeleteWallet(String, String);
        String PageSetting();
        String PageNotFound();
};

extern ServerEspClass ServerEsp;

#endif