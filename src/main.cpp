#include "global.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiAP.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include <xxtea-lib.h>
#include "Encryption.h"
#include "Storage.h"
#include "ServerEsp.h"
#include "Debug.h"
// #include <Callback.h>
WebServer server(WEB_PORT);

bool debug = DEBUG_MODE;
char* ssid = WIFI_SSID;
char* password = WIFI_PASSWORD;
char www_username[100] = WWW_USERNAME;
char www_password[100] = WWW_PASSWORD;
bool statusSleep = true;
int shutdown_time = SHUTDOWN_TIME;

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


void Resatrt(){
  Serial.println("################## RESTART ##################");
  ESP.restart();
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void handleAddWalletGET() {
  if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
  statusSleep = false;
  String config_auth = Storage.readFile(PATH_CONFIG_AUTH);
  String HTML PROGMEM = ServerEsp.PageAddWallet(config_auth);
  statusSleep = true;
  server.send(200, "text/html", HTML);
}

void handleAddWalletPOST() {
    statusSleep = false;
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        Debug.Print("Error parsing JSON ");
        Debug.PrintLine(error.c_str());
        String msg = error.c_str();
        server.send(400, F("text/html"),"Error in parsin json body! <br>" + msg);
    } else {
        String wallet_name = doc["wallet_name"];
        String address = doc["address"];
        String seed = doc["seed"];
        String dataf = "'"+wallet_name+";"+address+";"+seed+"'";
 

        
        String wal = Storage.readFile(PATH_CONFIG_WALLET);
        if(wal == "" || wal == "0"){
          //Encrypt
          String text = dataf;
          String key = www_password;

          int a = text.length();
          float b = 0;
          float c = 0;
          int len = 40;
          b = a % len;
          if(b > 0){
            c = 1;
          }
          int n = (a - b)/len + c;
          
          String en = String(n)+"|";
          for(int i=0;i<n;i++){
            en = en + Encryption.Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          Storage.writeFile(PATH_CONFIG_WALLET, en.c_str());
        }
        else{
          //Decrypt
          String de = "";
          String key = www_password;
          for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
            de = de + Encryption.Decrypt(getValue(wal, '|', i+1),key);
          }

          de = de +","+ dataf;

          //Encrypt
          String text = de;
          int a = text.length();
          float b = 0;
          float c = 0;
          int len = 40;
          b = a % len;
          if(b > 0){
            c = 1;
          }
          int n = (a - b)/len + c;
          String en = String(n)+"|";
          for(int i=0;i<n;i++){
            en = en + Encryption.Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          Storage.writeFile(PATH_CONFIG_WALLET, en.c_str());
        }
    }
    statusSleep = true;
    server.send(200, "text/plain", "ok");
}


void handleDeleteWalletGET() {
  if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
  statusSleep = false;
  //Decrypt
  String wal = Storage.readFile(PATH_CONFIG_WALLET);
  String de = "";
  String key = www_password;
  for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
    de = de + Encryption.Decrypt(getValue(wal, '|', i+1),key);
  }
  

  int le=1;
  for(int i=0;i<de.length();i++){
    if(de.substring(i,i+1)==","){
      le++;
    }
  }

  String myWallet = "";
  for(int i=0;i<le;i++){
    String wal = getValue(de, ',', i);
    String wal_name = getValue(wal, ';', 0);
    myWallet = myWallet +""+wal_name+"',";
  }

  myWallet.remove(myWallet.length()-1, 1);

  if(myWallet == "'"){
    myWallet = "";
  }

  Debug.PrintLine(__FILE__,__LINE__,myWallet);
  String config_auth = Storage.readFile(PATH_CONFIG_AUTH);
  String HTML PROGMEM = ServerEsp.PageDeleteWallet(config_auth, myWallet);
  statusSleep = true;
  server.send(200, "text/html", HTML);
}


void handleDeleteWalletPOST() {
    statusSleep = false;
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        String msg = error.c_str();
        Debug.PrintLine(__FILE__,__LINE__,"Error parsing JSON :" + msg);
        server.send(400, F("text/html"),"Error in parsin json body! <br>" + msg);
    } else {
        String delete_wallet_index = doc["delete_wallet_index"];
        String delete_wallet_name = doc["delete_wallet_name"];

        String wal = Storage.readFile(PATH_CONFIG_WALLET);
        if(wal == ""){

        }
        else{
          //Decrypt
          String de = "";
          String key = www_password;
          for(int i=0;i<getValue(wal,'|', 0).toInt();i++){            
            de = de + Encryption.Decrypt(getValue(wal, '|', i+1),key);
          }

          int lenn = 1;
          for(int i=0;i<de.length()-1;i++){
            if(de.substring(i, i+1)==","){
              lenn++;
            }
          }
          String de_new = "";
          String w;
          String w_name;
          for(int i=0;i<lenn;i++){
            w = getValue(de,',', i);
            w_name = getValue(w,':', 0);
            if(delete_wallet_index.toInt() != i && w_name != delete_wallet_name){          
              de_new = de_new +","+ getValue(de,',', i);
            }
          }
          
          de = de_new ;
          de.remove(0, 1);

          //Encrypt
          String text = de;
          int a = text.length();
          float b = 0;
          float c = 0;
          int len = 40;
          b = a % len;
          if(b > 0){
            c = 1;
          }
          int n = (a - b)/len + c;
          String en = String(n)+"|";
          for(int i=0;i<n;i++){
            en = en + Encryption.Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          Storage.writeFile(PATH_CONFIG_WALLET, en.c_str());
        }
    }
    statusSleep = true;
    server.send(200, "text/plain", "ok");
}


void handleWallet() {
  if(!server.authenticate(www_username, www_password))
    return server.requestAuthentication();
      
  statusSleep = false;
  //Decrypt
  String wal = Storage.readFile(PATH_CONFIG_WALLET);
  String de = "";
  String key = www_password;
  for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
    de = de + Encryption.Decrypt(getValue(wal, '|', i+1),key);
  }

  String myWallet = "'Select wallet.;Select wallet.;Select wallet.'";
  myWallet += ","+de;
  String HTML PROGMEM = ServerEsp.PageWallet(myWallet);
  statusSleep = true;
  server.send(200, "text/html", HTML);
}


void handleSettingGET() {
  if(Storage.listDir("/", 0) != ""){
    if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
  }
  statusSleep = false;
  String HTML PROGMEM = ServerEsp.PageSetting();
  statusSleep = true;
  server.send(200, "text/html", HTML);
}

void handleSettingPOST() {
    statusSleep = false;
    String postBody = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, postBody);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        String msg = error.c_str();
        Debug.PrintLine(__FILE__,__LINE__,"Error parsing JSON :" + msg);
        server.send(400, F("text/html"),"Error in parsin json body! <br>" + msg);
    } else {
        String wifi_ssid = doc["wifi_ssid"];
        String wifi_pass = doc["wifi_pass"];
        String username_auth = doc["username_auth"];
        String password_auth = doc["password_auth"];
        String shutdown_time = doc["shutdown"];
        String conf_shutdown = "{'shutdown':'"+shutdown_time+"'}";
        String conf_wifi = "{'wifi_ssid':'"+wifi_ssid+"','wifi_pass':'"+wifi_pass+"'}";
        String conf_auth = "{'username_auth':'"+username_auth+"','password_auth':'"+password_auth+"'}";

        String auth = Storage.readFile(PATH_CONFIG_AUTH);
        
        if(auth == ""){
          Storage.writeFile(PATH_CONFIG_WIFI, conf_wifi.c_str());
          Storage.writeFile(PATH_CONFIG_AUTH, conf_auth.c_str());
          Storage.writeFile(PATH_CONFIG_SHUTDOWN, conf_shutdown.c_str());
        }
        else{
          //Decrypt
          String de = "";
          String key = www_password;
          String wal = Storage.readFile(PATH_CONFIG_WALLET);
          for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
            de = de + Encryption.Decrypt(getValue(wal, '|', i+1),key);
          }

          key = password_auth;
          username_auth.toCharArray(www_username, 100);
          password_auth.toCharArray(www_password, 100);
          
          //Encrypt
          String text = de;
          int a = text.length();
          float b = 0;
          float c = 0;
          int len = 40;
          b = a % len;
          if(b > 0){
            c = 1;
          }
          int n = (a - b)/len + c;
          String en = String(n)+"|";
          for(int i=0;i<n;i++){
            en = en + Encryption.Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          Storage.writeFile(PATH_CONFIG_WALLET, en.c_str());
          Storage.writeFile(PATH_CONFIG_AUTH, conf_auth.c_str());
          Storage.writeFile(PATH_CONFIG_WIFI, conf_wifi.c_str());
          Storage.writeFile(PATH_CONFIG_SHUTDOWN, conf_shutdown.c_str());
        }
    }
    statusSleep = true;
    server.send(200, "text/plain", "ok");
    delay(1000);
    Resatrt();
}


void handleHelp(){
  String HTML PROGMEM = ServerEsp.PageHelp();
  server.send(404, "text/html", HTML);
}


void handleNotFound(){
  String HTML PROGMEM = ServerEsp.PageNotFound();
  server.send(404, "text/html", HTML);
}

void ReadConfigWifi(){
  String dataconfig_wifi = Storage.readFile(PATH_CONFIG_WIFI);
  Debug.PrintLine(__FILE__,__LINE__,"config : " + dataconfig_wifi);
  if(dataconfig_wifi != ""){
    DynamicJsonDocument doc_wifi(1024);
    deserializeJson(doc_wifi, dataconfig_wifi);
    DeserializationError error_wifi = deserializeJson(doc_wifi, dataconfig_wifi);
    if (error_wifi) {
        String msg = error_wifi.c_str();
        Debug.PrintLine(__FILE__,__LINE__,"Error parsing JSON :" + msg);
    } else {
        String wifi_ssid = doc_wifi["wifi_ssid"];
        String wifi_pass = doc_wifi["wifi_pass"];
          
        WiFi.softAP(wifi_ssid.c_str(), wifi_pass.c_str());
        WiFi.softAPConfig(local_ip, gateway, subnet);
        IPAddress myIP = WiFi.softAPIP();
        Debug.PrintLine(__FILE__,__LINE__,"AP "+wifi_ssid+" IP address: " + myIP.toString());

        String dataconfig_auth = Storage.readFile(PATH_CONFIG_AUTH);
        Debug.PrintLine(__FILE__,__LINE__,"config: " + dataconfig_auth);

        if(dataconfig_auth != ""){
          DynamicJsonDocument doc_auth(1024);
          deserializeJson(doc_auth, dataconfig_auth);
          DeserializationError error_auth = deserializeJson(doc_auth, dataconfig_auth);
          if (error_auth) {
            String msg = error_auth.c_str();
            Debug.PrintLine(__FILE__,__LINE__,"Error parsing JSON :" + msg);
          } else {
            String username_auth = doc_auth["username_auth"];
            String password_auth = doc_auth["password_auth"];
            username_auth.toCharArray(www_username, 100);
            password_auth.toCharArray(www_password, 100);
          }
        }
     }
  }
  else{
        //AP mode
        WiFi.softAP(ssid, password);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        IPAddress myIP = WiFi.softAPIP();
        Debug.PrintLine(__FILE__,__LINE__,"AP IP address: " + myIP.toString());
  }
}


void ReadConfigShutdown(void){
  String dataconfig_shutdown = Storage.readFile(PATH_CONFIG_SHUTDOWN);
  Debug.PrintLine(__FILE__,__LINE__,"config : " + dataconfig_shutdown);
  if(dataconfig_shutdown != ""){
    DynamicJsonDocument doc_shutdown(1024);
    deserializeJson(doc_shutdown, dataconfig_shutdown);
    DeserializationError error_wifi = deserializeJson(doc_shutdown, dataconfig_shutdown);
    if (error_wifi) {
      String msg = error_wifi.c_str();
      Debug.PrintLine(__FILE__,__LINE__,"Error parsing JSON :" + msg);
    } else {
      String buff = doc_shutdown["shutdown"];
      shutdown_time = buff.toInt();
    }
  }
  else{
    shutdown_time = 999999999;
  }
}

void SetCors(){
  server.enableCORS();
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
}

void SetRoute(){
  server.on("/", [](){
    if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
    String HTML = ServerEsp.Redirect();
    server.send(200, "text/html", HTML);
  });
  server.on("/help", HTTP_GET, handleHelp);
  server.on("/wallet", HTTP_GET, handleWallet);
  server.on("/addwallet", HTTP_GET, handleAddWalletGET);
  server.on("/addwallet", HTTP_POST, handleAddWalletPOST);
  server.on("/deletewallet", HTTP_GET, handleDeleteWalletGET);
  server.on("/deletewallet", HTTP_POST, handleDeleteWalletPOST);
  server.on("/setting", HTTP_GET, handleSettingGET);
  server.on("/setting", HTTP_POST, handleSettingPOST);

  server.onNotFound(handleNotFound);
  server.begin();
}


void setup(void){
  Serial.begin(115200);
  Debug.PrintLine(__FILE__,__LINE__,"################### START ###################");
  Storage.begin();
  ReadConfigWifi();
  ReadConfigShutdown();

  if (MDNS.begin("esp32")) {
    Debug.PrintLine(__FILE__,__LINE__,"mDNS responder started");
  }

  SetCors();
  SetRoute();
}

void loop(void){
  //deep sleep  
  if ((millis() > shutdown_time * 60 * 1000) && statusSleep) { 
    esp_deep_sleep_start(); 
  }  

  server.handleClient();
}