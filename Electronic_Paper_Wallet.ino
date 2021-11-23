#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiAP.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include <xxtea-iot-crypt.h>
ESP8266WebServer server(80);
#else
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiAP.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include <xxtea-iot-crypt.h>
WebServer server(80);
#endif

char* ssid = "Wallet";
char* password = "";

char www_username[100] = "admin";
char www_password[100] = "password";

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


bool statusSleep = true;
int shutdown_time = 999999999;



int ASCIIHexToInt(char c)
{
  int ret = 0;
  if ((c >= '0') && (c <= '9'))
    ret = (ret << 4) + c - '0';
  else
    ret = (ret << 4) + toupper(c) - 'A' + 10;
  return ret;
}

String HexToString(String input){
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

String Encrypt(String dataf,String key){
  xxtea.setKey(key);
  String result = xxtea.encrypt(dataf);
  result.toLowerCase(); // (Optional)
  return result;
}

String Decrypt(String dataf,String key){
  xxtea.setKey(key);
  return xxtea.decrypt(dataf);
}

String listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);
    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return "";
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return "";
    }
    File file = root.openNextFile();
    String listFile = "";
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            String fileName = file.name();
            listFile = listFile + fileName + ",";
            Serial.print("  FILE: ");
            Serial.print(fileName);
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    return listFile;
}

void Resatrt(){
  Serial.println("################## RESTART ##################");
  ESP.restart();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
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

String readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return "";
    }
    Serial.println("- read from file:");
    String dataf = "";
    while(file.available()){
        dataf = dataf + String(file.read(), HEX);
    }
    file.close();
    return HexToString(dataf);
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
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


void handleRoot() {
  #ifdef ESP8266
    server.send(200, "text/plain", "hello from esp8266!");
  #else
    String HTML = "<!DOCTYPE HTML><html lang='en-US'><head><meta charset='UTF-8'><script type='text/javascript'>window.location.href = 'http://'+window.location.hostname+'/address';</script></head<body></body></html>";
    server.send(200, "text/html", HTML);
  #endif
}



void handleAddWalletGET() {
  if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
  statusSleep = false;
  String HTML PROGMEM = R"=====(
                                <!DOCTYPE HTML>
                                <html lang='en-US'>
                                <meta http-equiv='Content-Type' content='text/html; charset=UTF-8' />
                                <meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no' />
                                )=====";
  if(readFile(SPIFFS, "/config_auth.txt") == ""){
      HTML = HTML + "<script type='text/javascript'>window.location.href = 'http://'+window.location.hostname+'/setting';</script>";
  }
                         
      HTML = HTML +     R"=====(
                                <body style="background-color: #515A5A;">
                                <h2 style='text-align: center; margin: 50px 0 0 0; padding: 0; color: #FFF  ;'>Add Wallet</h2>
                                <p style='text-align: center; margin: 0 0 20px 0; padding: 0; color: #F4D03F;'>(อย่าเชื่อมต่ออินเทอร์เน็ตและควรใช้งานเมื่ออยู่คนเดียวตามลำพัง!)</p>
                                
                                <div style="background-color: #273746; border-radius: 30px;">
                                  <h3 style='text-align: center; color: #FFF; margin:0; padding-top: 20px;'>Wallet</h3>
                                  <div style="width: 90%; margin: 0 auto 0 auto;">
                                    <p style='text-align: center; margin: 0 0 0 0; padding: 0; color: #F4D03F;'>(Seed จะถูกเข้ารหัสไว้ด้วย Password ของอุปกรณ์ควรจำ Password ให้ขึ้นใจ)</p>
                                  </div>
                                  <div style="display: flex; margin-top:20px; width: 100%;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="wallet_name" placeholder='Wallet Name' onkeyup="BlockSpecialCharacter(event)" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                
                                  <div style="display: flex; width: 100%; padding:20px 0 0 0;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="address" placeholder='Address' onkeyup="BlockSpecialCharacter(event)" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                  <div style="display: flex; width: 100%; padding:20px 0 0 0;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="seed" placeholder='Seed' onkeyup="BlockSpecialCharacter(event)" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                  <div style="display: flex; width: 100%; padding:20px 0 20px 0;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="seed2" placeholder='Confirm seed' onkeyup="BlockSpecialCharacter(event)" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                </div>
                                
                                <div style=' display: flex; padding:20px 0 20px 0;'>
                                  <div style="margin: 0 auto 0 auto;">
                                    <button style='width:150px; height:35px; background-color:#1E8449; color: #FFF; border-radius: 10px;' onclick='SAVE()'>SAVE</button>
                                  </div>
                                </div>
                                
                                <div style="padding-bottom: 80px;"></div>
                                
                                <script type='text/javascript'>
                                  function SAVE(){
                                    if(document.getElementById("wallet_name").value != "" && document.getElementById("address").value != "" && document.getElementById("seed").value != "" && document.getElementById("seed2").value != "" && document.getElementById("seed").value == document.getElementById("seed2").value){
                                      var myHeaders = new Headers();
                                      myHeaders.append("Content-Type", "application/json");
                                      myHeaders.append("Access-Control-Allow-Headers", "*");
                                      var raw = JSON.stringify({"wallet_name": document.getElementById("wallet_name").value,"address": document.getElementById("address").value,"seed": document.getElementById("seed").value});
                                      var requestOptions = {method: 'POST',headers: myHeaders,body: raw,redirect: 'follow'};
                                      alert("กำลังเพิ่ม wallet รอสักครู่...");
                                      fetch('http://'+window.location.hostname+'/addwallet', requestOptions).then(response => response.text()).then((result) => {console.log(result);if(result == 'ok'){
                                        alert("เพิ่ม wallet สำเร็จ.")
                                        document.getElementById("wallet_name").value = "";
                                        document.getElementById("address").value = "";
                                        document.getElementById("seed").value = "";
                                        document.getElementById("seed2").value = "";
                                      }}).catch((error) => {
                                        console.log('error', error)
                                        alert("Wallet ไม่ถูกบันทึก!\nตรวจสอบการเชื่อมต่อ Wifi ของอุปกรณ์")
                                      });
                                    }
                                    else{
                                      alert("กรอกข้อมูลให้ครบถ้วน ตรวจสอบ seed ให้แน่ใจว่าถูกต้อง!")
                                    }
                                  }
                                
                                  function BlockSpecialCharacter(e){
                                    var text = e.target.value;
                                    var text_new = text;
                                    var iChars = "\\\',|\";";
                                    for (var i=0; i < text.length; i++) {
                                      if(iChars.includes(text[i])){
                                        console.log(text[i]);
                                        text_new = text_new.replace(text[i], "");
                                        alert("ห้ามใช้อักษร | ; , \' \" `")
                                      }
                                    }
                                    e.target.value = text_new;
                                  }
                                
                                </script>
                                  <div class="footer" style="display: flex; justify-content: space-evenly; background-color: #000; position: fixed; left: 0; bottom: 0; width: 100%; border-radius: 20px 20px 0 0;">
                                    <div>
                                      <p id="wallet_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirectt("wallet")'>Wallet</p>
                                    </div>
                                    <div>
                                      <p id="setting_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirectt("setting")'>Setting</p>
                                    </div>
                                  </div>
                                
                                <script type='text/javascript'>
                                  function Redirectt(v){
                                    window.location.href='http://'+window.location.hostname+'/'+v;
                                  }
                                
                                  var listt = window.location.href.split("/");
                                  for(var i=0;i<listt.length;i++){
                                      if(listt[i]==="wallet"){
                                        const element1 = document.getElementById("wallet_footer");
                                        const element2 = document.getElementById("setting_footer");
                                        element1.style = "cursor: pointer; font-weight: bold; color: white;"
                                        element2.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                      }
                                      else if(listt[i]==="setting"){
                                      const element1 = document.getElementById("wallet_footer");
                                        const element2 = document.getElementById("setting_footer");
                                        element1.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                        element2.style = "cursor: pointer; font-weight: bold; color: white;"
                                      }
                                  }
                                    
                                </script>
                                </body>
                                </html>
                                )=====";
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
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());
        String msg = error.c_str();
        server.send(400, F("text/html"),"Error in parsin json body! <br>" + msg);
    } else {
        String wallet_name = doc["wallet_name"];
        String address = doc["address"];
        String seed = doc["seed"];
        String dataf = "'"+wallet_name+";"+address+";"+seed+"'";
 

        
        String wal = readFile(SPIFFS, "/config_wallet.txt");
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
            en = en + Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          writeFile(SPIFFS, "/config_wallet.txt", en.c_str());
        }
        else{
          //Decrypt
          String de = "";
          String key = www_password;
          for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
            de = de + Decrypt(getValue(wal, '|', i+1),key);
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
            en = en + Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          writeFile(SPIFFS, "/config_wallet.txt", en.c_str());
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
  String wal = readFile(SPIFFS, "/config_wallet.txt");
  String de = "";
  String key = www_password;
  for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
    de = de + Decrypt(getValue(wal, '|', i+1),key);
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

  Serial.println(myWallet);
   
  String HTML PROGMEM = R"=====(
                                <!DOCTYPE HTML>
                                <html lang='en-US'>
                                <meta http-equiv='Content-Type' content='text/html; charset=UTF-8' />
                                <meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no' />
                                <head>)=====";
  if(readFile(SPIFFS, "/config_auth.txt") == ""){
      HTML = HTML + "<script type='text/javascript'>window.location.href = 'http://'+window.location.hostname+'/setting';</script>";
  }
                         
      HTML = HTML +     R"=====(                                  
                                <script type='text/javascript'>
                                  alert("การลบ wallet ควรทำด้วยความระมัดระวัง!");
                                  var listwallet = [)=====";
  if(myWallet != "''"){
    HTML = HTML + myWallet;
  }
  HTML = HTML +        R"=====(];
                              </script>
                              <style>
                              body {font-family: Arial, Helvetica, sans-serif;}
                              
                              /* The Modal (background) */
                              .modal {
                                display: none; /* Hidden by default */
                                position: fixed; /* Stay in place */
                                z-index: 1; /* Sit on top */
                                padding-top: 100px; /* Location of the box */
                                left: 0;
                                top: 0;
                                width: 100%; /* Full width */
                                height: 100%; /* Full height */
                                overflow: auto; /* Enable scroll if needed */
                                background-color: rgb(0,0,0); /* Fallback color */
                                background-color: rgba(0,0,0,0.4); /* Black w/ opacity */
                              }
                              
                              /* Modal Content */
                              .modal-content {
                                position: relative;
                                background-color: #fefefe;
                                margin: auto;
                                padding: 0;
                                border: 1px solid #888;
                                width: 80%;
                                box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2),0 6px 20px 0 rgba(0,0,0,0.19);
                                -webkit-animation-name: animatetop;
                                -webkit-animation-duration: 0.4s;
                                animation-name: animatetop;
                                animation-duration: 0.4s;
                                border-radius: 20px 20px 20px 20px;
                              }
                              
                              /* Add Animation */
                              @-webkit-keyframes animatetop {
                                from {top:-300px; opacity:0} 
                                to {top:0; opacity:1}
                              }
                              
                              @keyframes animatetop {
                                from {top:-300px; opacity:0}
                                to {top:0; opacity:1}
                              }
                              
                              /* The Close Button */
                              .close {
                                color: white;
                                float: right;
                                font-size: 28px;
                                font-weight: bold;
                              }
                              
                              .close:hover,
                              .close:focus {
                                color: #000;
                                text-decoration: none;
                                cursor: pointer;
                              }
                              
                              .modal-header {
                                padding: 2px 16px;
                                background-color: #FF0000;
                                color: white;
                                border-radius: 20px 20px 0px 0px;
                              }
                              
                              .modal-body {padding: 2px 16px; border-radius: 0px 0px 20px 20px;}
                              
                              </style>
                              </head>
                              <body style="background-color: #515A5A;">
                                <h2 style='text-align: center; margin: 50px 0 0 0; padding: 0; color: #FFF  ;'>Delete Wallet</h2>
                                <div style="width: 90%; margin: 0 auto 0 auto;">
                                  <p style='text-align: center; margin: 0 0 20px 0; padding: 0; color: #F4D03F;'>(อย่าเชื่อมต่ออินเทอร์เน็ตและควรใช้งานเมื่ออยู่คนเดียวตามลำพัง!)</p>
                                </div>
                                <div style="background-color: #273746; border-radius: 30px;">
                                  <h3 style='text-align: center; color: #FFF; margin:0; padding-top: 20px;'>Wallet</h3>
                                  <div style="width: 90%; margin: auto;">
                                    <p style='text-align: center; margin: 0 0 0 0; padding: 0; color: #F4D03F;'>(Seed จะถูกเข้ารหัสไว้ด้วย Password ของอุปกรณ์ควรจำ Password ให้ขึ้นใจ)</p>
                                  </div>
                                  <div style="padding-bottom: 20px;"></div>
                                  <div id="btndelete"></div>
                                  <div style="padding-bottom: 20px;"></div>
                                </div>
                                <div style="padding-bottom: 80px;"></div>
                              
                                <div id="myModal" class="modal">
                                  <div class="modal-content">
                                    <div class="modal-header">
                                      <span class="close">&times;</span>
                                      <h2 id="headerdeletewallet">Delete Wallet</h2>
                                    </div>
                                    <div class="modal-body">
                                      <p id="deletewallettext1" style='text-align: center;'>Wallet จะถูกลบโดยการเขียนทับไฟล์เดิมและไม่สามารถกู้คืนได้</p>
                                      <p id="deletewallettext2" style='text-align: center;'>ถ้าหากต้องการลบพิมพ์ "Wallet"</p>
                                      <center><input id="walletdeleted" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 10px auto; border-radius: 10px; background-color: #FFC900; color: #000;' onkeyup="DeleteWallet(event)"></input></center>
                                    </div>
                                  </div>
                                </div>
                              
                              <script type='text/javascript'>
                                for(var i=0;i<listwallet.length;i++){
                                    const divwalletdel = document.createElement("div");
                                    divwalletdel.style = "display: block; width: 100%; margin: 30px 0 10px 0;";
                                    const divtext = document.createElement("div");
                                    divtext.style = "display: block; width: 90%; margin: 40px auto 0 auto;";
                                    const walletdel = document.createElement("p");
                                    const wal_t = document.createTextNode(listwallet[i]);
                                    walletdel.style = " text-align: center; color: #FFF; font-size: 12px;";
                                    walletdel.appendChild(wal_t);
                                    divtext.appendChild(walletdel);
                                    const btndel = document.createElement("button");
                                    const t = document.createTextNode("Delete");
                                    const center = document.createElement("center");
                                    btndel.style = "width:150px; height:35px; background-color:#FF0000; color: #FFF; border-radius: 10px; cursor: pointer;";
                                    btndel.value = i;
                                    btndel.classList.add("deletewallet");
                                    btndel.appendChild(t);
                                    divwalletdel.appendChild(divtext);
                                    center.appendChild(btndel);
                                    divwalletdel.appendChild(center);
                                    const element = document.getElementById("btndelete");
                                    element.appendChild(divwalletdel);
                                  }
                              
                              
                                var namewallet = "";
                                var indexwallet = -1;
                                var modal = document.getElementById("myModal");
                                var btn = document.querySelectorAll(".deletewallet");
                                btn.forEach((element,index) => {
                                  element.addEventListener('click',()=>{
                                    console.log(index,element.value)
                                    document.getElementById("headerdeletewallet").textContent = "Delete " + listwallet[index];
                                    document.getElementById("headerdeletewallet").style= "text-align: center; color: #FFF; font-size: 20px; font-weight: bold;";
                                    document.getElementById("deletewallettext1").textContent = listwallet[index] + " จะถูกลบโดยการเขียนทับไฟล์เดิมและไม่สามารถกู้คืนได้";
                                    document.getElementById("deletewallettext2").textContent = 'ถ้าหากต้องการลบพิมพ์ "' + listwallet[index] + '"  เพื่อยืนยัน';
                                    indexwallet = index;
                                    namewallet = listwallet[index];
                                    modal.style.display = "block";
                                  })
                                });
                              
                              
                                var span = document.getElementsByClassName("close")[0];
                                btn.onclick = function() {
                                  modal.style.display = "block";
                                }
                                span.onclick = function() {
                                  modal.style.display = "none";
                                }
                                window.onclick = function(event) {
                                  if (event.target == modal) {
                                    modal.style.display = "none";
                                  }
                                }
                              
                                function DeleteWallet(e){
                                    if(e.target.value === namewallet){
                                      document.getElementById("walletdeleted").value = ""
                              
                                      modal.style.display = "none";
                                      
                                      var myHeaders = new Headers();
                                      myHeaders.append("Content-Type", "application/json");
                                      myHeaders.append("Access-Control-Allow-Headers", "*");
                                      var raw = JSON.stringify({"delete_wallet_index": indexwallet,"delete_wallet_name": namewallet});
                                      var requestOptions = {method: 'POST',headers: myHeaders,body: raw,redirect: 'follow'};
                                      alert("ลบ "+ namewallet + " สำเร็จ.\nโปรดรอ website update สักครู่...");
                                      fetch('http://'+window.location.hostname+'/deletewallet', requestOptions).then(
                                        response => response.text()).then((result) => {
                                          if(result == 'ok'){
                                            namewallet = "";
                                            indexwallet = -1;
                                            window.location.href = 'http://'+window.location.hostname+'/deletewallet';
                                          }
                                          else{
                                            namewallet = "";
                                          }
                                        }).catch((error) => {console.log('error', error); alert("Wallet ไม่ถูกลบ!\nตรวจสอบการเชื่อมต่อ Wifi ของอุปกรณ์")});
                                    }
                                }
                              </script>
                              
                              <div class="footer" style="display: flex; justify-content: space-evenly; background-color: #000; position: fixed; left: 0; bottom: 0; width: 100%; border-radius: 20px 20px 0 0;">
                                <div>
                                  <p id="wallet_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("wallet")'>Wallet</p>
                                </div>
                                <div>
                                  <p id="setting_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("setting")'>Setting</p>
                                </div>
                              </div>
                              
                              <script type='text/javascript'>
                              var listt = window.location.href.split("/");
                                for(var i=0;i<listt.length;i++){
                                    if(listt[i]==="wallet"){
                                        const element1 = document.getElementById("wallet_footer");
                                        const element2 = document.getElementById("setting_footer");
                                        element1.style = "cursor: pointer; font-weight: bold; color: white;"
                                        element2.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                    }
                                    else if(listt[i]==="setting"){
                                    const element1 = document.getElementById("wallet_footer");
                                        const element2 = document.getElementById("setting_footer");
                                        element1.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                        element2.style = "cursor: pointer; font-weight: bold; color: white;"
                                    }
                                }
                              
                              function Redirect(v){
                                window.location.href='http://'+window.location.hostname+'/'+v;
                              }
                              </script>
                              
                              </body>
                              </html>
                                )=====";
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
        // if the file didn't open, print an error:
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());
        String msg = error.c_str();
        server.send(400, F("text/html"),"Error in parsin json body! <br>" + msg);
    } else {
        String delete_wallet_index = doc["delete_wallet_index"];
        String delete_wallet_name = doc["delete_wallet_name"];

        String wal = readFile(SPIFFS, "/config_wallet.txt");
        if(wal == ""){

        }
        else{
          //Decrypt
          String de = "";
          String key = www_password;
          for(int i=0;i<getValue(wal,'|', 0).toInt();i++){            
            de = de + Decrypt(getValue(wal, '|', i+1),key);
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
            en = en + Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          writeFile(SPIFFS, "/config_wallet.txt", en.c_str());
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
  String wal = readFile(SPIFFS, "/config_wallet.txt");
  String de = "";
  String key = www_password;
  for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
    de = de + Decrypt(getValue(wal, '|', i+1),key);
  }

  String myWallet = "'Select wallet.;Select wallet.;Select wallet.'";
  myWallet += ","+de;
  String HTML PROGMEM = R"=====(
                                <!DOCTYPE HTML><html lang='en-US'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8' /><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no' /></head>
                                <body style="background-color: #515A5A;">
                                <script type='text/javascript'>
                                  var listwalet = [)=====";

  HTML += myWallet;
  HTML +=               R"=====(]
                                </script>
                                
                                <h2 style='text-align: center; margin: 50px 0 0 0; padding: 0; color: #FFF  ;'>Welcome to</h2>
                                <h2 style='text-align: center; margin: 0 0 20px 0; padding: 0; color: #FFF;'>Electronic Paper Wallet</h2>
                                <p style='text-align: center; margin: 0 0 20px 0; padding: 0; color: #F4D03F;'>(อย่าเชื่อมต่ออินเทอร์เน็ตและควรใช้งานเมื่ออยู่คนเดียวตามลำพัง!)</p>
                                
                                <center>
                                  <select id="mywallet" onchange="GetValueDropDown()" name="wallet" id="wallet" style='text-align: center; width: 50%; height: 35px; border-radius: 10px; background-color: #D35400; color: #FFF;'>
                                  </select>
                                  <button id="showseed_button" style='height:35px; margin: 0 auto 0 0; background-color:#B03A2E; color: #FFF; border-radius: 10px; display: inline; cursor: pointer;' onclick='ShowSeed()'>Show Seed</button>
                                  <button id="hideseed_button" style='height:35px; margin: 0 auto 0 0; background-color:#B03A2E; color: #FFF; border-radius: 10px; display: none; cursor: pointer;' onclick='HideSeed()'>Hide Seed</button>
                                </center>
                                
                                <div style="padding-top: 30px;">⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀  </div>
                                
                                <div style="background-color: #273746; border-radius: 30px;">
                                  <h3 style='text-align: center; color: #FFF; margin:0; padding-top: 20px;'>Address</h3>
                                  <div style="background-color: #FFF; height: 270px; width: 270px; margin: auto; border-radius: 10px;">
                                    <center><div id='qrcode_address' style='width:100px; height:100px; padding: 8px 5px 0 0; margin:15px 0px 15px -150px;'></div></center>
                                  </div>
                                  <div style="display: flex; margin-top:20px; width: 100%; padding-bottom: 20px;">
                                    <input id="address" style='text-align: center; width: 50%;  height: 30px; margin: 0 0 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    <button style='height:35px; margin: 0 auto 0 5px; background-color:#1E8449; color: #FFF; border-radius: 10px; cursor: pointer;' onclick='CopyAddress()'>Copy</button>
                                  </div>
                                </div>
                                
                                <div style="padding-top: 30px;">⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀  </div>
                                
                                <div id="seed_div" style="background-color: #273746; border-radius: 30px; display: none;">
                                  <h3 style='text-align: center; color: #FFF; margin:0; padding-top: 20px;'>Seed</h3>
                                  <div style="background-color: #FFF; height: 270px; width: 270px; margin: auto; border-radius: 10px;">
                                    <center><div id='qrcode_seed' style='width:100px; height:100px; padding: 8px 5px 0 0; margin:15px 0px 15px -150px;'></div></center>
                                  </div>
                                  <div style="display: flex; margin-top:20px; width: 100%; padding-bottom: 20px;">
                                    <input id="seed" style='text-align: center; width: 50%;  height: 30px; margin: 0 0 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    <button style='height:35px; margin: 0 auto 0 5px; background-color:#1E8449; color: #FFF; border-radius: 10px; cursor: pointer;' onclick='CopySeed()'>Copy</button>
                                  </div>
                                </div>
                                
                                
                                <div style="display: flex; padding-top: 20px; ">
                                  <button style='width: 30%; height:45px; background-color:#FF0000; color: #FFF; border-radius: 10px; margin: 0 20px 0 auto; cursor: pointer;' onclick='Redirect("deletewallet")'>Delete Wallet</button>
                                  <button style='width: 30%; height:45px; background-color:#1D26E8; color: #FFF; border-radius: 10px; margin: 0 auto 0 20px; cursor: pointer;' onclick='Redirect("addwallet")'>Add Wallet</button>
                                </div>
                                <div style="padding-bottom: 80px;"></div>
                                
                                
                                <script type='text/javascript'>
                                  for(var i=0;i<listwalet.length;i++){
                                    const opt = document.createElement("option");
                                    var list = listwalet[i].split(";")
                                    const text = document.createTextNode(list[0]);
                                    opt.appendChild(text);
                                    opt.value = i;
                                    const mywal = document.getElementById("mywallet");
                                    mywal.appendChild(opt);
                                  }
                                
                                
                                  var address = 'Select wallet.';
                                  var seed = 'Select wallet.';
                                  document.getElementById('address').value = address;
                                  document.getElementById('seed').value = seed;
                                  
                                  //library gen qr code ref:https://github.com/davidshimjs/qrcodejs
                                  var QRCode;!function(){function a(a){this.mode=c.MODE_8BIT_BYTE,this.data=a,this.parsedData=[];for(var b=[],d=0,e=this.data.length;e>d;d++){var f=this.data.charCodeAt(d);f>65536?(b[0]=240|(1835008&f)>>>18,b[1]=128|(258048&f)>>>12,b[2]=128|(4032&f)>>>6,b[3]=128|63&f):f>2048?(b[0]=224|(61440&f)>>>12,b[1]=128|(4032&f)>>>6,b[2]=128|63&f):f>128?(b[0]=192|(1984&f)>>>6,b[1]=128|63&f):b[0]=f,this.parsedData=this.parsedData.concat(b)}this.parsedData.length!=this.data.length&&(this.parsedData.unshift(191),this.parsedData.unshift(187),this.parsedData.unshift(239))}function b(a,b){this.typeNumber=a,this.errorCorrectLevel=b,this.modules=null,this.moduleCount=0,this.dataCache=null,this.dataList=[]}function i(a,b){if(void 0==a.length)throw new Error(a.length+"/"+b);for(var c=0;c<a.length&&0==a[c];)c++;this.num=new Array(a.length-c+b);for(var d=0;d<a.length-c;d++)this.num[d]=a[d+c]}function j(a,b){this.totalCount=a,this.dataCount=b}function k(){this.buffer=[],this.length=0}function m(){return"undefined"!=typeof CanvasRenderingContext2D}function n(){var a=!1,b=navigator.userAgent;return/android/i.test(b)&&(a=!0,aMat=b.toString().match(/android ([0-9]\.[0-9])/i),aMat&&aMat[1]&&(a=parseFloat(aMat[1]))),a}function r(a,b){for(var c=1,e=s(a),f=0,g=l.length;g>=f;f++){var h=0;switch(b){case d.L:h=l[f][0];break;case d.M:h=l[f][1];break;case d.Q:h=l[f][2];break;case d.H:h=l[f][3]}if(h>=e)break;c++}if(c>l.length)throw new Error("Too long data");return c}function s(a){var b=encodeURI(a).toString().replace(/\%[0-9a-fA-F]{2}/g,"a");return b.length+(b.length!=a?3:0)}a.prototype={getLength:function(){return this.parsedData.length},write:function(a){for(var b=0,c=this.parsedData.length;c>b;b++)a.put(this.parsedData[b],8)}},b.prototype={addData:function(b){var c=new a(b);this.dataList.push(c),this.dataCache=null},isDark:function(a,b){if(0>a||this.moduleCount<=a||0>b||this.moduleCount<=b)throw new Error(a+","+b);return this.modules[a][b]},getModuleCount:function(){return this.moduleCount},make:function(){this.makeImpl(!1,this.getBestMaskPattern())},makeImpl:function(a,c){this.moduleCount=4*this.typeNumber+17,this.modules=new Array(this.moduleCount);for(var d=0;d<this.moduleCount;d++){this.modules[d]=new Array(this.moduleCount);for(var e=0;e<this.moduleCount;e++)this.modules[d][e]=null}this.setupPositionProbePattern(0,0),this.setupPositionProbePattern(this.moduleCount-7,0),this.setupPositionProbePattern(0,this.moduleCount-7),this.setupPositionAdjustPattern(),this.setupTimingPattern(),this.setupTypeInfo(a,c),this.typeNumber>=7&&this.setupTypeNumber(a),null==this.dataCache&&(this.dataCache=b.createData(this.typeNumber,this.errorCorrectLevel,this.dataList)),this.mapData(this.dataCache,c)},setupPositionProbePattern:function(a,b){for(var c=-1;7>=c;c++)if(!(-1>=a+c||this.moduleCount<=a+c))for(var d=-1;7>=d;d++)-1>=b+d||this.moduleCount<=b+d||(this.modules[a+c][b+d]=c>=0&&6>=c&&(0==d||6==d)||d>=0&&6>=d&&(0==c||6==c)||c>=2&&4>=c&&d>=2&&4>=d?!0:!1)},getBestMaskPattern:function(){for(var a=0,b=0,c=0;8>c;c++){this.makeImpl(!0,c);var d=f.getLostPoint(this);(0==c||a>d)&&(a=d,b=c)}return b},createMovieClip:function(a,b,c){var d=a.createEmptyMovieClip(b,c),e=1;this.make();for(var f=0;f<this.modules.length;f++)for(var g=f*e,h=0;h<this.modules[f].length;h++){var i=h*e,j=this.modules[f][h];j&&(d.beginFill(0,100),d.moveTo(i,g),d.lineTo(i+e,g),d.lineTo(i+e,g+e),d.lineTo(i,g+e),d.endFill())}return d},setupTimingPattern:function(){for(var a=8;a<this.moduleCount-8;a++)null==this.modules[a][6]&&(this.modules[a][6]=0==a%2);for(var b=8;b<this.moduleCount-8;b++)null==this.modules[6][b]&&(this.modules[6][b]=0==b%2)},setupPositionAdjustPattern:function(){for(var a=f.getPatternPosition(this.typeNumber),b=0;b<a.length;b++)for(var c=0;c<a.length;c++){var d=a[b],e=a[c];if(null==this.modules[d][e])for(var g=-2;2>=g;g++)for(var h=-2;2>=h;h++)this.modules[d+g][e+h]=-2==g||2==g||-2==h||2==h||0==g&&0==h?!0:!1}},setupTypeNumber:function(a){for(var b=f.getBCHTypeNumber(this.typeNumber),c=0;18>c;c++){var d=!a&&1==(1&b>>c);this.modules[Math.floor(c/3)][c%3+this.moduleCount-8-3]=d}for(var c=0;18>c;c++){var d=!a&&1==(1&b>>c);this.modules[c%3+this.moduleCount-8-3][Math.floor(c/3)]=d}},setupTypeInfo:function(a,b){for(var c=this.errorCorrectLevel<<3|b,d=f.getBCHTypeInfo(c),e=0;15>e;e++){var g=!a&&1==(1&d>>e);6>e?this.modules[e][8]=g:8>e?this.modules[e+1][8]=g:this.modules[this.moduleCount-15+e][8]=g}for(var e=0;15>e;e++){var g=!a&&1==(1&d>>e);8>e?this.modules[8][this.moduleCount-e-1]=g:9>e?this.modules[8][15-e-1+1]=g:this.modules[8][15-e-1]=g}this.modules[this.moduleCount-8][8]=!a},mapData:function(a,b){for(var c=-1,d=this.moduleCount-1,e=7,g=0,h=this.moduleCount-1;h>0;h-=2)for(6==h&&h--;;){for(var i=0;2>i;i++)if(null==this.modules[d][h-i]){var j=!1;g<a.length&&(j=1==(1&a[g]>>>e));var k=f.getMask(b,d,h-i);k&&(j=!j),this.modules[d][h-i]=j,e--,-1==e&&(g++,e=7)}if(d+=c,0>d||this.moduleCount<=d){d-=c,c=-c;break}}}},b.PAD0=236,b.PAD1=17,b.createData=function(a,c,d){for(var e=j.getRSBlocks(a,c),g=new k,h=0;h<d.length;h++){var i=d[h];g.put(i.mode,4),g.put(i.getLength(),f.getLengthInBits(i.mode,a)),i.write(g)}for(var l=0,h=0;h<e.length;h++)l+=e[h].dataCount;if(g.getLengthInBits()>8*l)throw new Error("code length overflow. ("+g.getLengthInBits()+">"+8*l+")");for(g.getLengthInBits()+4<=8*l&&g.put(0,4);0!=g.getLengthInBits()%8;)g.putBit(!1);for(;;){if(g.getLengthInBits()>=8*l)break;if(g.put(b.PAD0,8),g.getLengthInBits()>=8*l)break;g.put(b.PAD1,8)}return b.createBytes(g,e)},b.createBytes=function(a,b){for(var c=0,d=0,e=0,g=new Array(b.length),h=new Array(b.length),j=0;j<b.length;j++){var k=b[j].dataCount,l=b[j].totalCount-k;d=Math.max(d,k),e=Math.max(e,l),g[j]=new Array(k);for(var m=0;m<g[j].length;m++)g[j][m]=255&a.buffer[m+c];c+=k;var n=f.getErrorCorrectPolynomial(l),o=new i(g[j],n.getLength()-1),p=o.mod(n);h[j]=new Array(n.getLength()-1);for(var m=0;m<h[j].length;m++){var q=m+p.getLength()-h[j].length;h[j][m]=q>=0?p.get(q):0}}for(var r=0,m=0;m<b.length;m++)r+=b[m].totalCount;for(var s=new Array(r),t=0,m=0;d>m;m++)for(var j=0;j<b.length;j++)m<g[j].length&&(s[t++]=g[j][m]);for(var m=0;e>m;m++)for(var j=0;j<b.length;j++)m<h[j].length&&(s[t++]=h[j][m]);return s};for(var c={MODE_NUMBER:1,MODE_ALPHA_NUM:2,MODE_8BIT_BYTE:4,MODE_KANJI:8},d={L:1,M:0,Q:3,H:2},e={PATTERN000:0,PATTERN001:1,PATTERN010:2,PATTERN011:3,PATTERN100:4,PATTERN101:5,PATTERN110:6,PATTERN111:7},f={PATTERN_POSITION_TABLE:[[],[6,18],[6,22],[6,26],[6,30],[6,34],[6,22,38],[6,24,42],[6,26,46],[6,28,50],[6,30,54],[6,32,58],[6,34,62],[6,26,46,66],[6,26,48,70],[6,26,50,74],[6,30,54,78],[6,30,56,82],[6,30,58,86],[6,34,62,90],[6,28,50,72,94],[6,26,50,74,98],[6,30,54,78,102],[6,28,54,80,106],[6,32,58,84,110],[6,30,58,86,114],[6,34,62,90,118],[6,26,50,74,98,122],[6,30,54,78,102,126],[6,26,52,78,104,130],[6,30,56,82,108,134],[6,34,60,86,112,138],[6,30,58,86,114,142],[6,34,62,90,118,146],[6,30,54,78,102,126,150],[6,24,50,76,102,128,154],[6,28,54,80,106,132,158],[6,32,58,84,110,136,162],[6,26,54,82,110,138,166],[6,30,58,86,114,142,170]],G15:1335,G18:7973,G15_MASK:21522,getBCHTypeInfo:function(a){for(var b=a<<10;f.getBCHDigit(b)-f.getBCHDigit(f.G15)>=0;)b^=f.G15<<f.getBCHDigit(b)-f.getBCHDigit(f.G15);return(a<<10|b)^f.G15_MASK},getBCHTypeNumber:function(a){for(var b=a<<12;f.getBCHDigit(b)-f.getBCHDigit(f.G18)>=0;)b^=f.G18<<f.getBCHDigit(b)-f.getBCHDigit(f.G18);return a<<12|b},getBCHDigit:function(a){for(var b=0;0!=a;)b++,a>>>=1;return b},getPatternPosition:function(a){return f.PATTERN_POSITION_TABLE[a-1]},getMask:function(a,b,c){switch(a){case e.PATTERN000:return 0==(b+c)%2;case e.PATTERN001:return 0==b%2;case e.PATTERN010:return 0==c%3;case e.PATTERN011:return 0==(b+c)%3;case e.PATTERN100:return 0==(Math.floor(b/2)+Math.floor(c/3))%2;case e.PATTERN101:return 0==b*c%2+b*c%3;case e.PATTERN110:return 0==(b*c%2+b*c%3)%2;case e.PATTERN111:return 0==(b*c%3+(b+c)%2)%2;default:throw new Error("bad maskPattern:"+a)}},getErrorCorrectPolynomial:function(a){for(var b=new i([1],0),c=0;a>c;c++)b=b.multiply(new i([1,g.gexp(c)],0));return b},getLengthInBits:function(a,b){if(b>=1&&10>b)switch(a){case c.MODE_NUMBER:return 10;case c.MODE_ALPHA_NUM:return 9;case c.MODE_8BIT_BYTE:return 8;case c.MODE_KANJI:return 8;default:throw new Error("mode:"+a)}else if(27>b)switch(a){case c.MODE_NUMBER:return 12;case c.MODE_ALPHA_NUM:return 11;case c.MODE_8BIT_BYTE:return 16;case c.MODE_KANJI:return 10;default:throw new Error("mode:"+a)}else{if(!(41>b))throw new Error("type:"+b);switch(a){case c.MODE_NUMBER:return 14;case c.MODE_ALPHA_NUM:return 13;case c.MODE_8BIT_BYTE:return 16;case c.MODE_KANJI:return 12;default:throw new Error("mode:"+a)}}},getLostPoint:function(a){for(var b=a.getModuleCount(),c=0,d=0;b>d;d++)for(var e=0;b>e;e++){for(var f=0,g=a.isDark(d,e),h=-1;1>=h;h++)if(!(0>d+h||d+h>=b))for(var i=-1;1>=i;i++)0>e+i||e+i>=b||(0!=h||0!=i)&&g==a.isDark(d+h,e+i)&&f++;f>5&&(c+=3+f-5)}for(var d=0;b-1>d;d++)for(var e=0;b-1>e;e++){var j=0;a.isDark(d,e)&&j++,a.isDark(d+1,e)&&j++,a.isDark(d,e+1)&&j++,a.isDark(d+1,e+1)&&j++,(0==j||4==j)&&(c+=3)}for(var d=0;b>d;d++)for(var e=0;b-6>e;e++)a.isDark(d,e)&&!a.isDark(d,e+1)&&a.isDark(d,e+2)&&a.isDark(d,e+3)&&a.isDark(d,e+4)&&!a.isDark(d,e+5)&&a.isDark(d,e+6)&&(c+=40);for(var e=0;b>e;e++)for(var d=0;b-6>d;d++)a.isDark(d,e)&&!a.isDark(d+1,e)&&a.isDark(d+2,e)&&a.isDark(d+3,e)&&a.isDark(d+4,e)&&!a.isDark(d+5,e)&&a.isDark(d+6,e)&&(c+=40);for(var k=0,e=0;b>e;e++)for(var d=0;b>d;d++)a.isDark(d,e)&&k++;var l=Math.abs(100*k/b/b-50)/5;return c+=10*l}},g={glog:function(a){if(1>a)throw new Error("glog("+a+")");return g.LOG_TABLE[a]},gexp:function(a){for(;0>a;)a+=255;for(;a>=256;)a-=255;return g.EXP_TABLE[a]},EXP_TABLE:new Array(256),LOG_TABLE:new Array(256)},h=0;8>h;h++)g.EXP_TABLE[h]=1<<h;for(var h=8;256>h;h++)g.EXP_TABLE[h]=g.EXP_TABLE[h-4]^g.EXP_TABLE[h-5]^g.EXP_TABLE[h-6]^g.EXP_TABLE[h-8];for(var h=0;255>h;h++)g.LOG_TABLE[g.EXP_TABLE[h]]=h;i.prototype={get:function(a){return this.num[a]},getLength:function(){return this.num.length},multiply:function(a){for(var b=new Array(this.getLength()+a.getLength()-1),c=0;c<this.getLength();c++)for(var d=0;d<a.getLength();d++)b[c+d]^=g.gexp(g.glog(this.get(c))+g.glog(a.get(d)));return new i(b,0)},mod:function(a){if(this.getLength()-a.getLength()<0)return this;for(var b=g.glog(this.get(0))-g.glog(a.get(0)),c=new Array(this.getLength()),d=0;d<this.getLength();d++)c[d]=this.get(d);for(var d=0;d<a.getLength();d++)c[d]^=g.gexp(g.glog(a.get(d))+b);return new i(c,0).mod(a)}},j.RS_BLOCK_TABLE=[[1,26,19],[1,26,16],[1,26,13],[1,26,9],[1,44,34],[1,44,28],[1,44,22],[1,44,16],[1,70,55],[1,70,44],[2,35,17],[2,35,13],[1,100,80],[2,50,32],[2,50,24],[4,25,9],[1,134,108],[2,67,43],[2,33,15,2,34,16],[2,33,11,2,34,12],[2,86,68],[4,43,27],[4,43,19],[4,43,15],[2,98,78],[4,49,31],[2,32,14,4,33,15],[4,39,13,1,40,14],[2,121,97],[2,60,38,2,61,39],[4,40,18,2,41,19],[4,40,14,2,41,15],[2,146,116],[3,58,36,2,59,37],[4,36,16,4,37,17],[4,36,12,4,37,13],[2,86,68,2,87,69],[4,69,43,1,70,44],[6,43,19,2,44,20],[6,43,15,2,44,16],[4,101,81],[1,80,50,4,81,51],[4,50,22,4,51,23],[3,36,12,8,37,13],[2,116,92,2,117,93],[6,58,36,2,59,37],[4,46,20,6,47,21],[7,42,14,4,43,15],[4,133,107],[8,59,37,1,60,38],[8,44,20,4,45,21],[12,33,11,4,34,12],[3,145,115,1,146,116],[4,64,40,5,65,41],[11,36,16,5,37,17],[11,36,12,5,37,13],[5,109,87,1,110,88],[5,65,41,5,66,42],[5,54,24,7,55,25],[11,36,12],[5,122,98,1,123,99],[7,73,45,3,74,46],[15,43,19,2,44,20],[3,45,15,13,46,16],[1,135,107,5,136,108],[10,74,46,1,75,47],[1,50,22,15,51,23],[2,42,14,17,43,15],[5,150,120,1,151,121],[9,69,43,4,70,44],[17,50,22,1,51,23],[2,42,14,19,43,15],[3,141,113,4,142,114],[3,70,44,11,71,45],[17,47,21,4,48,22],[9,39,13,16,40,14],[3,135,107,5,136,108],[3,67,41,13,68,42],[15,54,24,5,55,25],[15,43,15,10,44,16],[4,144,116,4,145,117],[17,68,42],[17,50,22,6,51,23],[19,46,16,6,47,17],[2,139,111,7,140,112],[17,74,46],[7,54,24,16,55,25],[34,37,13],[4,151,121,5,152,122],[4,75,47,14,76,48],[11,54,24,14,55,25],[16,45,15,14,46,16],[6,147,117,4,148,118],[6,73,45,14,74,46],[11,54,24,16,55,25],[30,46,16,2,47,17],[8,132,106,4,133,107],[8,75,47,13,76,48],[7,54,24,22,55,25],[22,45,15,13,46,16],[10,142,114,2,143,115],[19,74,46,4,75,47],[28,50,22,6,51,23],[33,46,16,4,47,17],[8,152,122,4,153,123],[22,73,45,3,74,46],[8,53,23,26,54,24],[12,45,15,28,46,16],[3,147,117,10,148,118],[3,73,45,23,74,46],[4,54,24,31,55,25],[11,45,15,31,46,16],[7,146,116,7,147,117],[21,73,45,7,74,46],[1,53,23,37,54,24],[19,45,15,26,46,16],[5,145,115,10,146,116],[19,75,47,10,76,48],[15,54,24,25,55,25],[23,45,15,25,46,16],[13,145,115,3,146,116],[2,74,46,29,75,47],[42,54,24,1,55,25],[23,45,15,28,46,16],[17,145,115],[10,74,46,23,75,47],[10,54,24,35,55,25],[19,45,15,35,46,16],[17,145,115,1,146,116],[14,74,46,21,75,47],[29,54,24,19,55,25],[11,45,15,46,46,16],[13,145,115,6,146,116],[14,74,46,23,75,47],[44,54,24,7,55,25],[59,46,16,1,47,17],[12,151,121,7,152,122],[12,75,47,26,76,48],[39,54,24,14,55,25],[22,45,15,41,46,16],[6,151,121,14,152,122],[6,75,47,34,76,48],[46,54,24,10,55,25],[2,45,15,64,46,16],[17,152,122,4,153,123],[29,74,46,14,75,47],[49,54,24,10,55,25],[24,45,15,46,46,16],[4,152,122,18,153,123],[13,74,46,32,75,47],[48,54,24,14,55,25],[42,45,15,32,46,16],[20,147,117,4,148,118],[40,75,47,7,76,48],[43,54,24,22,55,25],[10,45,15,67,46,16],[19,148,118,6,149,119],[18,75,47,31,76,48],[34,54,24,34,55,25],[20,45,15,61,46,16]],j.getRSBlocks=function(a,b){var c=j.getRsBlockTable(a,b);if(void 0==c)throw new Error("bad rs block @ typeNumber:"+a+"/errorCorrectLevel:"+b);for(var d=c.length/3,e=[],f=0;d>f;f++)for(var g=c[3*f+0],h=c[3*f+1],i=c[3*f+2],k=0;g>k;k++)e.push(new j(h,i));return e},j.getRsBlockTable=function(a,b){switch(b){case d.L:return j.RS_BLOCK_TABLE[4*(a-1)+0];case d.M:return j.RS_BLOCK_TABLE[4*(a-1)+1];case d.Q:return j.RS_BLOCK_TABLE[4*(a-1)+2];case d.H:return j.RS_BLOCK_TABLE[4*(a-1)+3];default:return void 0}},k.prototype={get:function(a){var b=Math.floor(a/8);return 1==(1&this.buffer[b]>>>7-a%8)},put:function(a,b){for(var c=0;b>c;c++)this.putBit(1==(1&a>>>b-c-1))},getLengthInBits:function(){return this.length},putBit:function(a){var b=Math.floor(this.length/8);this.buffer.length<=b&&this.buffer.push(0),a&&(this.buffer[b]|=128>>>this.length%8),this.length++}};var l=[[17,14,11,7],[32,26,20,14],[53,42,32,24],[78,62,46,34],[106,84,60,44],[134,106,74,58],[154,122,86,64],[192,152,108,84],[230,180,130,98],[271,213,151,119],[321,251,177,137],[367,287,203,155],[425,331,241,177],[458,362,258,194],[520,412,292,220],[586,450,322,250],[644,504,364,280],[718,560,394,310],[792,624,442,338],[858,666,482,382],[929,711,509,403],[1003,779,565,439],[1091,857,611,461],[1171,911,661,511],[1273,997,715,535],[1367,1059,751,593],[1465,1125,805,625],[1528,1190,868,658],[1628,1264,908,698],[1732,1370,982,742],[1840,1452,1030,790],[1952,1538,1112,842],[2068,1628,1168,898],[2188,1722,1228,958],[2303,1809,1283,983],[2431,1911,1351,1051],[2563,1989,1423,1093],[2699,2099,1499,1139],[2809,2213,1579,1219],[2953,2331,1663,1273]],o=function(){var a=function(a,b){this._el=a,this._htOption=b};return a.prototype.draw=function(a){function g(a,b){var c=document.createElementNS("http://www.w3.org/2000/svg",a);for(var d in b)b.hasOwnProperty(d)&&c.setAttribute(d,b[d]);return c}var b=this._htOption,c=this._el,d=a.getModuleCount();Math.floor(b.width/d),Math.floor(b.height/d),this.clear();var h=g("svg",{viewBox:"0 0 "+String(d)+" "+String(d),width:"100%",height:"100%",fill:b.colorLight});h.setAttributeNS("http://www.w3.org/2000/xmlns/","xmlns:xlink","http://www.w3.org/1999/xlink"),c.appendChild(h),h.appendChild(g("rect",{fill:b.colorDark,width:"1",height:"1",id:"template"}));for(var i=0;d>i;i++)for(var j=0;d>j;j++)if(a.isDark(i,j)){var k=g("use",{x:String(i),y:String(j)});k.setAttributeNS("http://www.w3.org/1999/xlink","href","#template"),h.appendChild(k)}},a.prototype.clear=function(){for(;this._el.hasChildNodes();)this._el.removeChild(this._el.lastChild)},a}(),p="svg"===document.documentElement.tagName.toLowerCase(),q=p?o:m()?function(){function a(){this._elImage.src=this._elCanvas.toDataURL("image/png"),this._elImage.style.display="block",this._elCanvas.style.display="none"}function d(a,b){var c=this;if(c._fFail=b,c._fSuccess=a,null===c._bSupportDataURI){var d=document.createElement("img"),e=function(){c._bSupportDataURI=!1,c._fFail&&_fFail.call(c)},f=function(){c._bSupportDataURI=!0,c._fSuccess&&c._fSuccess.call(c)};return d.onabort=e,d.onerror=e,d.onload=f,d.src="data:image/gif;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==",void 0}c._bSupportDataURI===!0&&c._fSuccess?c._fSuccess.call(c):c._bSupportDataURI===!1&&c._fFail&&c._fFail.call(c)}if(this._android&&this._android<=2.1){var b=1/window.devicePixelRatio,c=CanvasRenderingContext2D.prototype.drawImage;CanvasRenderingContext2D.prototype.drawImage=function(a,d,e,f,g,h,i,j){if("nodeName"in a&&/img/i.test(a.nodeName))for(var l=arguments.length-1;l>=1;l--)arguments[l]=arguments[l]*b;else"undefined"==typeof j&&(arguments[1]*=b,arguments[2]*=b,arguments[3]*=b,arguments[4]*=b);c.apply(this,arguments)}}var e=function(a,b){this._bIsPainted=!1,this._android=n(),this._htOption=b,this._elCanvas=document.createElement("canvas"),this._elCanvas.width=b.width,this._elCanvas.height=b.height,a.appendChild(this._elCanvas),this._el=a,this._oContext=this._elCanvas.getContext("2d"),this._bIsPainted=!1,this._elImage=document.createElement("img"),this._elImage.style.display="none",this._el.appendChild(this._elImage),this._bSupportDataURI=null};return e.prototype.draw=function(a){var b=this._elImage,c=this._oContext,d=this._htOption,e=a.getModuleCount(),f=d.width/e,g=d.height/e,h=Math.round(f),i=Math.round(g);b.style.display="none",this.clear();for(var j=0;e>j;j++)for(var k=0;e>k;k++){var l=a.isDark(j,k),m=k*f,n=j*g;c.strokeStyle=l?d.colorDark:d.colorLight,c.lineWidth=1,c.fillStyle=l?d.colorDark:d.colorLight,c.fillRect(m,n,f,g),c.strokeRect(Math.floor(m)+.5,Math.floor(n)+.5,h,i),c.strokeRect(Math.ceil(m)-.5,Math.ceil(n)-.5,h,i)}this._bIsPainted=!0},e.prototype.makeImage=function(){this._bIsPainted&&d.call(this,a)},e.prototype.isPainted=function(){return this._bIsPainted},e.prototype.clear=function(){this._oContext.clearRect(0,0,this._elCanvas.width,this._elCanvas.height),this._bIsPainted=!1},e.prototype.round=function(a){return a?Math.floor(1e3*a)/1e3:a},e}():function(){var a=function(a,b){this._el=a,this._htOption=b};return a.prototype.draw=function(a){for(var b=this._htOption,c=this._el,d=a.getModuleCount(),e=Math.floor(b.width/d),f=Math.floor(b.height/d),g=['<table style="border:0;border-collapse:collapse;">'],h=0;d>h;h++){g.push("<tr>");for(var i=0;d>i;i++)g.push('<td style="border:0;border-collapse:collapse;padding:0;margin:0;width:'+e+"px;height:"+f+"px;background-color:"+(a.isDark(h,i)?b.colorDark:b.colorLight)+';"></td>');g.push("</tr>")}g.push("</table>"),c.innerHTML=g.join("");var j=c.childNodes[0],k=(b.width-j.offsetWidth)/2,l=(b.height-j.offsetHeight)/2;k>0&&l>0&&(j.style.margin=l+"px "+k+"px")},a.prototype.clear=function(){this._el.innerHTML=""},a}();QRCode=function(a,b){if(this._htOption={width:256,height:256,typeNumber:4,colorDark:"#000000",colorLight:"#ffffff",correctLevel:d.H},"string"==typeof b&&(b={text:b}),b)for(var c in b)this._htOption[c]=b[c];"string"==typeof a&&(a=document.getElementById(a)),this._android=n(),this._el=a,this._oQRCode=null,this._oDrawing=new q(this._el,this._htOption),this._htOption.text&&this.makeCode(this._htOption.text)},QRCode.prototype.makeCode=function(a){this._oQRCode=new b(r(a,this._htOption.correctLevel),this._htOption.correctLevel),this._oQRCode.addData(a),this._oQRCode.make(),this._el.title=a,this._oDrawing.draw(this._oQRCode),this.makeImage()},QRCode.prototype.makeImage=function(){"function"==typeof this._oDrawing.makeImage&&(!this._android||this._android>=3)&&this._oDrawing.makeImage()},QRCode.prototype.clear=function(){this._oDrawing.clear()},QRCode.CorrectLevel=d}();
                                  var qrcode_address = new QRCode(document.getElementById('qrcode_address'),address, {width : 100,height : 100});
                                  var qrcode_seed = new QRCode(document.getElementById('qrcode_seed'),seed, {width : 100,height : 100});
                                
                                  function CopyAddress(){
                                    var v=address;var dummy = document.createElement('textarea');document.body.appendChild(dummy);dummy.value = v;dummy.select();document.execCommand('copy');dummy.style.display = 'none';document.body.removeChild(dummy);
                                  }
                                
                                  function CopySeed(){
                                    var v=seed;var dummy = document.createElement('textarea');document.body.appendChild(dummy);dummy.value = v;dummy.select();document.execCommand('copy');dummy.style.display = 'none';document.body.removeChild(dummy);
                                  }
                                
                                  function ShowSeed(){
                                    var seed_div = document.getElementById("seed_div");
                                    seed_div.style.display = 'block';
                                    var showseed_button = document.getElementById("showseed_button");
                                    showseed_button.style.display = 'none';
                                    var hideseed_button = document.getElementById("hideseed_button");
                                    hideseed_button.style.display = 'inline';
                                  }
                                
                                  function HideSeed(){
                                    var seed_div = document.getElementById("seed_div");
                                    seed_div.style.display = 'none';
                                    var showseed_button = document.getElementById("showseed_button");
                                    showseed_button.style.display = 'inline';
                                    var hideseed_button = document.getElementById("hideseed_button");
                                    hideseed_button.style.display = 'none';
                                  }
                                
                                  function GetValueDropDown(){
                                    var e = document.getElementById("mywallet");
                                    var value = e.value;
                                        
                                    var list = listwalet[value].split(";");
                                    address = list[1];
                                    seed = list[2];
                                
                                    document.getElementById('address').value = address;
                                    qrcode_address.clear();
                                    qrcode_address.makeCode(address);
                                
                                    document.getElementById('seed').value = seed;
                                    qrcode_seed.clear();
                                    qrcode_seed.makeCode(seed);
                                  }
                                </script>
                                
                                <div class="footer" style="display: flex; justify-content: space-evenly; background-color: #000; position: fixed; left: 0; bottom: 0; width: 100%; border-radius: 20px 20px 0 0;">
                                  <div>
                                    <p id="wallet_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("wallet")'>Wallet</p>
                                  </div>
                                  <div>
                                    <p id="setting_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("setting")'>Setting</p>
                                  </div>
                                </div>
                                
                                <script type='text/javascript'>
                                  var listt = window.location.href.split("/");
                                    for(var i=0;i<listt.length;i++){
                                        if(listt[i]==="wallet"){
                                            const element1 = document.getElementById("wallet_footer");
                                            const element2 = document.getElementById("setting_footer");
                                            element1.style = "cursor: pointer; font-weight: bold; color: white;"
                                            element2.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                        }
                                        else if(listt[i]==="setting"){
                                        const element1 = document.getElementById("wallet_footer");
                                            const element2 = document.getElementById("setting_footer");
                                            element1.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                            element2.style = "cursor: pointer; font-weight: bold; color: white;"
                                        }
                                    }
                                
                                  function Redirect(v){
                                    window.location.href='http://'+window.location.hostname+'/'+v;
                                  }
                                </script>
                                </body>
                                </html>
                               )=====";
    statusSleep = true;
    server.send(200, "text/html", HTML);
}


void handleSettingGET() {
  if(listDir(SPIFFS, "/", 0) != ""){
    if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
  }
  statusSleep = false;
  String HTML PROGMEM = R"=====(
                                <!DOCTYPE HTML><html lang='en-US'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8' /><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no' /></head><body style="background-color: #515A5A;">

                                <h2 style='text-align: center; margin: 50px 0 0 0; padding: 0; color: #FFF  ;'>Setting</h2>
                                <h2 style='text-align: center; margin: 0 0 5px 0; padding: 0; color: #FFF;'>Electronic Paper Wallet</h2>
                                <p style='text-align: center; margin: 0 0 20px 0; padding: 0; color: #F4D03F;'>(อย่าเชื่อมต่ออินเทอร์เน็ตและควรใช้งานเมื่ออยู่คนเดียวตามลำพัง!)</p>
                                
                                <div style="background-color: #273746; border-radius: 30px;">
                                  <h3 style='text-align: center; color: #FFF; margin:0; padding-top: 20px;'>Wifi</h3>
                                  <div style="width: 90%; margin: 0 auto 0 auto;">
                                    <p style='text-align: center; margin: 0 0 0 0; padding: 0; color: #F4D03F;'>(หากไม่ใส่รหัสผ่านจะใช้เป็น Public wifi การสื่อสารระหว่าง website และ อุปกรณ์นั้นจะใช้ http หากมีบุคคลที่สามเชื่อมต่อมาภายใน network ของอุปกรณ์สามารถที่จะดักข้อมูลระหว่างทางได้)</p>
                                  </div>
                                  <div style="display: flex; margin-top:20px; width: 100%;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="wifi_ssid" placeholder='SSID' onkeyup="BlockSpecialCharacter(event)" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                  <div style="display: flex; width: 100%; padding:20px 0 0 0;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="wifi_pass" placeholder='Password' onkeyup="BlockSpecialCharacter(event)" type="password" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                  <div style="display: flex; width: 100%; padding:20px 0 20px 0;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="wifi_pass2" placeholder='Confirm password' onkeyup="BlockSpecialCharacter(event)" type="password" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                </div>
                                
                                <div style="height: 20px;"></div>
                                
                                <div style="background-color: #273746; border-radius: 30px;">
                                  <h3 style='text-align: center; color: #FFF; margin:0; padding-top: 20px;'>Authentication</h3>
                                  <div style="width: 90%; margin: 0 auto 0 auto;">
                                    <p style='text-align: center; margin: 0 0 0 0; padding: 0; color: #F4D03F;'>(ใช้ Username และ Password ในการเข้าถึง electronic paper wallet ส่วนของ Username คือชื่อของอุปกรณ์ Password ใช้ในการเข้ารหัสและถอนรหัส Seed.)</p>
                                  </div>
                                  <div style="display: flex; margin-top:20px; width: 100%;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="auth_user" placeholder='Username' onkeyup="BlockSpecialCharacter(event)" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                
                                  <div style="display: flex; width: 100%; padding:20px 0 0 0;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="auth_pass" placeholder='Password' onkeyup="BlockSpecialCharacter(event)" type="password" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                  <div style="display: flex; width: 100%; padding:20px 0 20px 0;">
                                    <div style="width: 20%;"></div>
                                    <div style="width: 100%;">
                                      <input id="auth_pass2" placeholder='Confirm password' onkeyup="BlockSpecialCharacter(event)" type="password" style='text-align: center; width: 100%; height: 30px; margin: 0 auto 0 auto; border-radius: 10px; background-color: #2E4053; color: #FFF;'></input>
                                    </div>
                                    <div style="width: 20%;"></div>
                                  </div>
                                </div>
                              
                                <div style="height: 20px;"></div>
                                
                                <div style="background-color: #273746; border-radius: 30px;">
                                  <h3 style='text-align: center; color: #FFF; margin:0; padding-top: 20px;'>Shut down device</h3>
                                  <div style="width: 90%; margin: 0 auto 0 auto;">
                                    <p style='text-align: center; margin: 0 0 0 0; padding: 0; color: #F4D03F;'>(ปิดอุปกรณ์อัตโนมัติเมื่อครบกำหนดเวลาเพื่อป้องกันการ Brute force ภายใน Netwrok)</p>
                                  </div>
                                  <div style="display: flex; margin-top:20px; width: 100%;">
                              
                                    <div style=" margin: auto;">
                                      <br>
                                      <input type="radio" id="15min" name="fav_language" value="15">
                                      <label for="15min" style='color: #FFF;'>15 นาที</label><br><br>
                                      <input type="radio" id="30min" name="fav_language" value="30">
                                      <label for="30min" style='color: #FFF;'>30 นาที</label><br><br>
                                      <input type="radio" id="60min" name="fav_language" value="60">
                                      <label for="60min" style='color: #FFF;'>60 นาที</label><br><br>
                                      <input type="radio" id="notdoanything" name="fav_language" value="999999999">
                                      <label for="notdoanything" style='color: #FFF;'>ไม่ปิด</label><br><br>
                                    </div>
                                  </div>
                                </div>
                                
                                <div style='padding:20px 0 80px 0;'>
                                  <center><button style='width:150px; height:35px; background-color:#1E8449; color: #FFF; border-radius: 10px;' onclick='SAVE()'>Save</button></center>
                                </div>
                              
                                <script type='text/javascript'>
                                  function SAVE(){
                                    var myHeaders = new Headers();
                                    myHeaders.append("Content-Type", "application/json");
                                    myHeaders.append("Access-Control-Allow-Headers", "*");
                                    if(document.getElementById("wifi_ssid").value != "" && document.getElementById("auth_user").value != "" && document.getElementById("auth_pass").value != "" && document.getElementById("wifi_pass").value == document.getElementById("wifi_pass2").value && document.getElementById("auth_pass").value == document.getElementById("auth_pass2").value && ( document.getElementById("15min").checked || document.getElementById("30min").checked || document.getElementById("60min").checked || document.getElementById("notdoanything").checked )){
                                      var listradio = ["15min","30min","60min","notdoanything"]
                                      var shutdown = "30"
                                      for(var i=0;i<listradio.length;i++){
                                        if(document.getElementById(listradio[i]).checked){
                                          shutdown = document.getElementById(listradio[i]).value
                                        }
                                      }
                                      var raw = JSON.stringify({"wifi_ssid": document.getElementById("wifi_ssid").value,"wifi_pass": document.getElementById("wifi_pass").value,"username_auth": document.getElementById("auth_user").value,"password_auth": document.getElementById("auth_pass").value,"shutdown":shutdown});
                                      var requestOptions = {method: 'POST',headers: myHeaders,body: raw,redirect: 'follow'};
                                      alert("กำลัง update setting.")
                                      fetch('http://'+window.location.hostname+'/setting', requestOptions).then(response => response.text()).then((result) => {
                                        if(result == 'ok'){
                                          alert("Update Setting สำเร็จ.\nระบบกำลัง restart รอสักครู่...\nหลังจากอุปกรณ์เปิดแล้วเชื่อมต่อไปยัง wifi ที่ตั้งค่าไว้.")
                                          window.location.href='http://'+window.location.hostname+'/wallet';
                                        }
                                      }).catch((error) => {console.log('error', error); alert("Setting ไม่ถูกบันทึก!\nตรวจสอบการเชื่อมต่อ Wifi ของอุปกรณ์")});
                                    }
                                    else{
                                      alert("กรอกข้อมูลให้ครบถ้วน!")
                                    }
                                  }
                              
                                  function BlockSpecialCharacter(e){
                                    var text = e.target.value;
                                    var text_new = text;
                                    var iChars = "\\\',|\";";
                                    for (var i=0; i < text.length; i++) {
                                      if(iChars.includes(text[i])){
                                        console.log(text[i]);
                                        text_new = text_new.replace(text[i], "");
                                        alert("ห้ามใช้อักษร | ; , \' \" `")
                                      }
                                    }
                                    e.target.value = text_new;
                                  }
                                </script>
                              
                              <div class="footer" style="display: flex; justify-content: space-evenly; background-color: #000; position: fixed; left: 0; bottom: 0; width: 100%; border-radius: 20px 20px 0 0;">
                                <div>
                                  <p id="wallet_footer" style="cursor: pointer; font-weight: bold; color: white;" onclick='Redirect("wallet")'>Wallet</p>
                                </div>
                                <div>
                                  <p id="setting_footer" style="cursor: pointer; font-weight: bold; color: white;" onclick='Redirect("setting")'>Setting</p>
                                </div>
                              </div>
                              
                              <script type='text/javascript'>
                              var listt = window.location.href.split("/");
                                for(var i=0;i<listt.length;i++){
                                    if(listt[i]==="wallet"){
                                        const element1 = document.getElementById("wallet_footer");
                                        const element2 = document.getElementById("setting_footer");
                                        element1.style = "cursor: pointer; font-weight: bold; color: white;"
                                        element2.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                    }
                                    else if(listt[i]==="setting"){
                                    const element1 = document.getElementById("wallet_footer");
                                        const element2 = document.getElementById("setting_footer");
                                        element1.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                        element2.style = "cursor: pointer; font-weight: bold; color: white;"
                                    }
                                }
                              
                              function Redirect(v){
                                    window.location.href='http://'+window.location.hostname+'/'+v;
                              }
                              </script>
                              </body>
                              </html>
                                )=====";
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
        // if the file didn't open, print an error:
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());
        String msg = error.c_str();
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

        String auth = readFile(SPIFFS, "/config_auth.txt");
        
        if(auth == ""){
          writeFile(SPIFFS, "/config_wifi.txt", conf_wifi.c_str());
          writeFile(SPIFFS, "/config_auth.txt", conf_auth.c_str());
          writeFile(SPIFFS, "/config_shutdown.txt", conf_shutdown.c_str());
        }
        else{
          //Decrypt
          String de = "";
          String key = www_password;
          String wal = readFile(SPIFFS, "/config_wallet.txt");
          for(int i=0;i<getValue(wal, '|', 0).toInt();i++){            
            de = de + Decrypt(getValue(wal, '|', i+1),key);
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
            en = en + Encrypt(text.substring((len*i), (len*i)+len),key) +"|";
          }
          en.remove(en.length()-1, 1);
          writeFile(SPIFFS, "/config_wallet.txt", en.c_str());
          writeFile(SPIFFS, "/config_auth.txt", conf_auth.c_str());
          writeFile(SPIFFS, "/config_wifi.txt", conf_wifi.c_str());
          writeFile(SPIFFS, "/config_shutdown.txt", conf_shutdown.c_str());
        }
    }
    statusSleep = true;
    server.send(200, "text/plain", "ok");
    delay(1000);
    Resatrt();
}


void handleHelp(){
  String HTML PROGMEM = R"=====(
                                <!DOCTYPE HTML><html lang='en-US'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8' /><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no' /></head>
                                <body style="background-color: #515A5A;">
                                <h2 style='text-align: center; margin: 50px 0 0 0; padding: 0; color: #FFF  ;'>Electronic Paper Wallet</h2>
                                <p style='text-align: center; margin: 0 0 100px 0; padding: 0; color: #F4D03F;'>(อย่าเชื่อมต่ออินเทอร์เน็ตและควรใช้งานเมื่ออยู่คนเดียวตามลำพัง!)</p>
                                
                                <div style="background-color: #273746; border-radius: 30px;">
                                    <div style="padding-bottom: 50px;"></div>
                                    <h1 style='text-align: center; color: #FFF; margin:0;'>HELP</h1>
                                    <div style="width: 90%; margin: auto;">
                                      <div style="padding-bottom: 20px;"></div>
                                      <p style='text-align: left; margin: 0 0 0 0; padding: 10px 0 0 0; color: #F4D03F;'>⠀⠀⠀1. หากเข้าถึงอุปกรณ์ไม่ได้ เหตุเพราะลืมรหัสผ่าน Wifi รหัสผ่าน Wifi ไม่ได้ถูกเข้ารหัสไว้คุณสามารถอ่านรหัส Wifi ได้จาก spiffs ไฟล์ชื่อ config_wifi.txt เมื่ออ่านได้ข้อมูลมาเสร็จแล้ว flash firmware ของอุปกรณ์ลงไปใหม่แล้ว login wifi ด้วยรหัสเดิม.</p>
                                      <div style="padding-bottom: 20px;"></div>
                                      <p style='text-align: left; margin: 0 0 0 0; padding: 10px 0 0 0; color: #F4D03F;'>⠀⠀⠀2. หากเข้าถึง wallet ไม่ได้ เหตุเพราะลืมรหัสผ่าน รหัสผ่าน wallet ไม่ได้ถูกเข้ารหัสไว้คุณสามารถอ่านรหัส wallet ได้จาก spiffs ไฟล์ชื่อ config_auth.txt เมื่ออ่านได้ข้อมูลมาเสร็จแล้ว flash firmware ของอุปกรณ์ลงไปใหม่แล้วใช้ password เดิม.</p>
                                      <div style="padding-bottom: 20px;"></div>
                                      <p style='text-align: left; margin: 0 0 0 0; padding: 10px 0 0 0; color: #F4D03F;'>⠀⠀⠀3. หากเข้าถึง wallet ไม่ได้ เหตุเพราะลบออกไป เสียใจด้วยกรณีนี้ไม่สามารถกู้คืนได้.</p>
                                      <div style="padding-bottom: 50px;"></div>
                                    </div>
                                </div>
                                <div style="padding-bottom: 80px;"></div>
                                
                                <div class="footer" style="display: flex; justify-content: space-evenly; background-color: #000; position: fixed; left: 0; bottom: 0; width: 100%; border-radius: 20px 20px 0 0;">
                                  <div>
                                    <p id="wallet_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("wallet")'>Wallet</p>
                                  </div>
                                  <div>
                                    <p id="setting_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("setting")'>Setting</p>
                                  </div>
                                </div>
                                
                                <script type='text/javascript'>
                                  var listt = window.location.href.split("/");
                                  for(var i=0;i<listt.length;i++){
                                      if(listt[i]==="wallet"){
                                          const element1 = document.getElementById("wallet_footer");
                                          const element2 = document.getElementById("setting_footer");
                                          element1.style = "cursor: pointer; font-weight: bold; color: white;"
                                          element2.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                      }
                                      else if(listt[i]==="setting"){
                                      const element1 = document.getElementById("wallet_footer");
                                          const element2 = document.getElementById("setting_footer");
                                          element1.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                          element2.style = "cursor: pointer; font-weight: bold; color: white;"
                                      }
                                  }
                                  function Redirect(v){
                                      window.location.href='http://'+window.location.hostname+'/'+v;
                                  }
                                </script>
                                </body>
                                </html>
                                )=====";
  server.send(404, "text/html", HTML);
}


void handleNotFound(){
  String HTML PROGMEM = R"=====(
                                <!DOCTYPE HTML><html lang='en-US'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8' /><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no' /></head>
                                <body style="background-color: #515A5A;">
                                <script type='text/javascript'>
                                  var listwalet = ['Select wallet.:Select wallet.:Select wallet.','BTC Umbrel:0x12345:0x54321','BTC Hold:0x67890:09876','ETH Defi:0x11111:0x22222','BNB Defi:0x11111:0x22222','Matic Defi:0x11111:0x22222','SOL Defi:0x11111:0x22222']
                                </script>
                                <h2 style='text-align: center; margin: 50px 0 0 0; padding: 0; color: #FFF  ;'>Electronic Paper Wallet</h2>
                                <p style='text-align: center; margin: 0 0 100px 0; padding: 0; color: #F4D03F;'>(อย่าเชื่อมต่ออินเทอร์เน็ตและควรใช้งานเมื่ออยู่คนเดียวตามลำพัง!)</p>
                                
                                <div style="background-color: #273746; border-radius: 30px;">
                                    <div style="padding-bottom: 50px;"></div>
                                    <h1 style='text-align: center; color: #FFF; margin:0;'>ERROR 404!</h1>
                                    <div style="width: 90%; margin: auto;">
                                        <p style='text-align: center; margin: 0 0 0 0; padding: 10px 0 0 0; color: #F4D03F;'>ไม่มี page ที่ต้องการเรียกตรวจสอบ url ให้ถูกต้อง</p>
                                    </div>
                                
                                    <div style="padding-bottom: 50px;"></div>
                                </div>
                                
                                <div class="footer" style="display: flex; justify-content: space-evenly; background-color: #000; position: fixed; left: 0; bottom: 0; width: 100%; border-radius: 20px 20px 0 0;">
                                  <div>
                                    <p id="wallet_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("wallet")'>Wallet</p>
                                  </div>
                                  <div>
                                    <p id="setting_footer" style="cursor: pointer; font-weight: bold; color: #6F6F6F;" onclick='Redirect("setting")'>Setting</p>
                                  </div>
                                </div>
                                
                                <script type='text/javascript'>
                                var listt = window.location.href.split("/");
                                  for(var i=0;i<listt.length;i++){
                                      if(listt[i]==="wallet"){
                                          const element1 = document.getElementById("wallet_footer");
                                          const element2 = document.getElementById("setting_footer");
                                          element1.style = "cursor: pointer; font-weight: bold; color: white;"
                                          element2.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                      }
                                      else if(listt[i]==="setting"){
                                      const element1 = document.getElementById("wallet_footer");
                                          const element2 = document.getElementById("setting_footer");
                                          element1.style = "cursor: pointer; font-weight: bold; color: #6F6F6F;"
                                          element2.style = "cursor: pointer; font-weight: bold; color: white;"
                                      }
                                  }
                                
                                function Redirect(v){
                                      window.location.href='http://'+window.location.hostname+'/'+v;
                                }
                                </script>
                                </body>
                                </html>
                                )=====";
  server.send(404, "text/html", HTML);
}

void setup(void){
  Serial.begin(115200);
  Serial.println("################### START ###################");
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  String dataconfig_wifi = readFile(SPIFFS, "/config_wifi.txt");
  Serial.println("config : " + dataconfig_wifi);
  if(dataconfig_wifi != ""){
    DynamicJsonDocument doc_wifi(1024);
    deserializeJson(doc_wifi, dataconfig_wifi);
    DeserializationError error_wifi = deserializeJson(doc_wifi, dataconfig_wifi);
    if (error_wifi) {
        Serial.print(F("Error parsing JSON "));
        Serial.println(error_wifi.c_str());
    } else {
        String wifi_ssid = doc_wifi["wifi_ssid"];
        String wifi_pass = doc_wifi["wifi_pass"];
          
        WiFi.softAP(wifi_ssid.c_str(), wifi_pass.c_str());
        WiFi.softAPConfig(local_ip, gateway, subnet);
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP "+wifi_ssid+" IP address: ");
        Serial.println(myIP);

        String dataconfig_auth = readFile(SPIFFS, "/config_auth.txt");
        Serial.println("config: " + dataconfig_auth);
        if(dataconfig_auth != ""){
          DynamicJsonDocument doc_auth(1024);
          deserializeJson(doc_auth, dataconfig_auth);
          DeserializationError error_auth = deserializeJson(doc_auth, dataconfig_auth);
          if (error_auth) {
              Serial.print(F("Error parsing JSON "));
              Serial.println(error_auth.c_str());
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
        Serial.print("AP IP address: ");
        Serial.println(myIP);
   }


  String dataconfig_shutdown = readFile(SPIFFS, "/config_shutdown.txt");
  Serial.println("config : " + dataconfig_shutdown);
   if(dataconfig_shutdown != ""){
    DynamicJsonDocument doc_shutdown(1024);
    deserializeJson(doc_shutdown, dataconfig_shutdown);
    DeserializationError error_wifi = deserializeJson(doc_shutdown, dataconfig_shutdown);
    if (error_wifi) {
        Serial.print(F("Error parsing JSON "));
        Serial.println(error_wifi.c_str());
    } else {
        String buff = doc_shutdown["shutdown"];
        shutdown_time = buff.toInt();
    }
   }
   else{
      shutdown_time = 999999999;
   }




#ifdef ESP8266
  if (MDNS.begin("esp8266")) {
#else
  if (MDNS.begin("esp32")) {
#endif
    Serial.println("MDNS responder started");
  }

  server.enableCORS();
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));



  
  server.on("/", [](){
    if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
    String path = "";
    if(readFile(SPIFFS, "/config_wifi.txt") == ""){
      path = "setting";
    }
    else {
      path = "wallet";
    }
    String HTML = "<!DOCTYPE HTML><html lang='en-US'><head><meta charset='UTF-8'><script type='text/javascript'>window.location.href = 'http://'+window.location.hostname+'/"+path+"';</script></head><body></body></html>";
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

void loop(void){
  //deep leep  
  if ((millis() > shutdown_time * 60 * 1000) && statusSleep) { 
    esp_deep_sleep_start(); 
  }  

  server.handleClient();
}
