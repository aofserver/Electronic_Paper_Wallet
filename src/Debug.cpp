#include "Debug.h"

DebugClass::DebugClass() {

}

void DebugClass::begin() {

}

void DebugClass::Print(String data){
  if(DEBUG_MODE){
    Serial.print(data);
  }
}

void DebugClass::PrintLine(String data){
  if(DEBUG_MODE){
    Serial.println(data);
  }
}

void DebugClass::PrintLine(String file, int line, String data){
  if(DEBUG_MODE){
    Serial.println(file + "\t" + line + "\t" + data);
  }
}

DebugClass Debug;