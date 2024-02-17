#include "Storage.h"

StorageClass::StorageClass()
{

}

void StorageClass::begin()
{
  // std::cout << "\n... Begin Storage ...";
  if(!SPIFFS.begin(true)){
    std::printf("SPIFFS Mount Failed!\n");
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
  std::printf("Listing directory: %s\r\n", dirname);
  
  File root = SPIFFS.open(dirname);
  if (!root)
  {
    std::printf("\n- failed to open directory");
    return "";
  }
  if (!root.isDirectory())
  {
    std::printf("\n - not a directory");
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
      std::printf("  FILE: ");
      std::printf(fileName.c_str());
      // std::printf("\tSIZE: ");
      // std::printf(file.size());
    }
    file = root.openNextFile();
  }
  return listFile;
}

void StorageClass::writeFile(const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);
    File file = SPIFFS.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

String StorageClass::readFile(const char * path){
    Serial.printf("Reading file: %s\r\n", path);
    File file = SPIFFS.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
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
    Serial.printf("Deleting file: %s\r\n", path);
    if(SPIFFS.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}


StorageClass Storage;