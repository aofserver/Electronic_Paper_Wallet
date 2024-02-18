#include "Storage.h"

StorageClass::StorageClass()
{

}

void StorageClass::begin()
{
  Debug.PrintLine(__FILE__,__LINE__,"... Begin Storage ...");
  if(!SPIFFS.begin(true)){
    Debug.PrintLine(__FILE__,__LINE__,"SPIFFS Mount Failed!\n");
    return;
  }
}

int StorageClass::ASCIIHexToInt(char c)
{
  int ret = 0;
  if ((c >= '0') && (c <= '9'))
    ret = (ret << 4) + c - '0';
  else
    ret = (ret << 4) + toupper(c) - 'A' + 10;
  return ret;
}

String StorageClass::HexToString(String input) {
  char temp[2];
  char c;
  String res = "";
  int val;
  for (int i = 0; i < input.length() - 1; i += 2) {
    temp[0] = input[i];
    temp[1] = input[i + 1];
    val = ASCIIHexToInt(temp[0]) * 16;      // First Hex digit
    val += ASCIIHexToInt(temp[1]);          // Second hex digit
    c = toascii(val);
    res = res + c;
  }
  return res;
}

String StorageClass::listDir(const char *dirname, uint8_t levels)
{
  Debug.PrintLine(__FILE__,__LINE__,"Listing directory: " + String(dirname));
  File root = SPIFFS.open(dirname);
  if (!root)
  {
    Debug.PrintLine(__FILE__,__LINE__,"[ERROR] failed to open directory");
    return "";
  }
  if (!root.isDirectory())
  {
    Debug.PrintLine(__FILE__,__LINE__,"[ERROR] not found a directory");
    return "";
  }
  File file = root.openNextFile();
  String listFile = "";
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      std::printf(file.name());
      if (levels)
      {
        listDir(file.name(), levels - 1);
      }
    }
    else
    {
      String fileName = file.name();
      listFile = listFile + fileName + ",";
      Debug.PrintLine(__FILE__,__LINE__,"  FILE: " + String(fileName.c_str()) + "\tSIZE: " + String(int(file.size())));
    }
    file = root.openNextFile();
  }
  return listFile;
}

void StorageClass::writeFile(const char * path, const char * message){
    Debug.PrintLine(__FILE__,__LINE__,"Writing file: " + String(path));
    File file = SPIFFS.open(path, FILE_WRITE);
    if(!file){
        Debug.PrintLine(__FILE__,__LINE__,"[ERROR] failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Debug.PrintLine(__FILE__,__LINE__,"file written");
    } else {
        Debug.PrintLine(__FILE__,__LINE__,"[ERROR] write failed");
    }
    file.close();
}


String StorageClass::readFile(const char * path){
    Debug.PrintLine(__FILE__,__LINE__,"Reading file: " + String(path));
    File file = SPIFFS.open(path);
    if(!file || file.isDirectory()){
        Debug.PrintLine(__FILE__,__LINE__,"[ERROR] failed to open file for reading");
        return "";
    }
    String dataf = "";
    while(file.available()){
        dataf = dataf + String(file.read(), HEX);
    }
    file.close();
    return HexToString(dataf);
}

void StorageClass::deleteFile(const char * path){
    Debug.PrintLine(__FILE__,__LINE__,"Deleting file: " + String(path));
    if(SPIFFS.remove(path)){
        Debug.PrintLine(__FILE__,__LINE__,"file deleted");
    } else {
        Serial.println("- delete failed");
        Debug.PrintLine(__FILE__,__LINE__,"[ERROR] failed to delete file");
    }
}


StorageClass Storage;