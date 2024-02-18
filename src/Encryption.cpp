#include "Encryption.h"

EncryptionClass::EncryptionClass() {

}

void EncryptionClass::begin() {
  
}

String EncryptionClass::Encrypt(String dataf,String key){
  xxtea.setKey(key);
  String result = xxtea.encrypt(dataf);
  result.toLowerCase();
  return result;
}

String EncryptionClass::Decrypt(String dataf,String key){
  xxtea.setKey(key);
  return xxtea.decrypt(dataf);
}


EncryptionClass Encryption;