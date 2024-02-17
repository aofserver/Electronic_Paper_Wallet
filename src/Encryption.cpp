#include "Encryption.h"

EncryptionClass::EncryptionClass() {

}

void EncryptionClass::begin() {
    // std::cout << "\n... Begin Encryption ...";
}

String EncryptionClass::Encrypt(String dataf,String key){
  xxtea.setKey(key);
  String result = xxtea.encrypt(dataf);
  result.toLowerCase(); // (Optional)
  return result;
}

String EncryptionClass::Decrypt(String dataf,String key){
  xxtea.setKey(key);
  return xxtea.decrypt(dataf);
}


EncryptionClass Encryption;