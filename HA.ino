

#include <FS.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

//--------------------------------------------------------------------------------
char ssid[40];
char pwd[40];

int restore_state = 1;
char savedstate[22] = "0,0,0,0";
int s0, s1, s2, s3;
String p;
const int rx = 3;
const int tx = 1;
int pins[] = {D5, D6, D7, D8};
int numPins = 4;
int pin, state;
WiFiClient client;
IPAddress myIP;
const char *ap_ssid = "RelayBoard";
const char *ap_pwd = "";
AsyncWebServer server(80);

//--------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(100);
  initGPIO();
  delay(100);
  Serial.println("\n\n\nRelay Program Started");

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          if(json.containsKey("ssid") )
            strcpy(ssid, json["ssid"]);
          if(json.containsKey("pwd") )  
            strcpy(pwd, json["pwd"]);
          if(json.containsKey("restore_state") )  
            restore_state = json["restore_state"];                         
          if(json.containsKey("savedstate") )  
            strcpy(savedstate, json["savedstate"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

 

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
  int ctr = 0;
  while (WiFi.status() != WL_CONNECTED && ctr < 30) {
    delay(500);
    Serial.print(".");
    ctr++;
  }
  if(WiFi.status() == WL_CONNECTED) {
    myIP = WiFi.localIP();
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: "); Serial.println(myIP);
  } else {
    Serial.println(".");
    Serial.print("Could not connect to WiFi: "); Serial.println(ssid);
    Serial.println("*** Retrying ***");
    WiFi.mode(WIFI_OFF);
    delay(3000);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pwd);
    int ctr = 0;
    while (WiFi.status() != WL_CONNECTED && ctr < 30) {
      delay(500);
      Serial.print(".");
      ctr++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      myIP = WiFi.localIP();
      Serial.println();
      Serial.println("WiFi connected");
      Serial.println("IP address: "); Serial.println(myIP);
      
    } else {
      Serial.println();
      Serial.print("Could not connect to WiFi: "); Serial.println(ssid);
      Serial.println("Starting Access Point ...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ap_ssid);
      myIP = WiFi.softAPIP();
      Serial.print("Access Point IP address: ");
      Serial.println(myIP);
    }
  }
  String ipaddr = String(myIP);
  
  //MDNS.addService("http","tcp",80);
  if (!MDNS.begin("relay")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("Access the relay board at  http://relay.local/");  





  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = ""
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Wi-Fi Relay Board</title>"
    "<script>"
    "loading = true;"
    "var xhttp = new XMLHttpRequest();"
    "xhttp.onreadystatechange = function() {"
    "    if (this.readyState == 4 && this.status == 200) {"
    "       var resp = JSON.parse(xhttp.responseText);"
    "       if(loading) { "
    "         document.getElementById('ssid').value = resp['ssid'];"
    "         document.getElementById('pwd').value = resp['pwd'];"
    "         if(resp['restore_state'] === '1') document.getElementsByName('restore_state')[0].checked = true;"
    "         else document.getElementsByName('restore_state')[1].checked = true;"
    "         document.getElementById('msg').innerHTML = resp['msg'];"
    "         loading=false"
    "       }"
    "       var loadstates = resp['currentstate'].split(',');"
    "       for(var j=0; j< loadstates.length; j++) {"
    "         var chkname = 'load_' + (j+1);"
    "         if(loadstates[j] === '1') document.getElementsByName(chkname)[1].checked = true;"
    "         else document.getElementsByName(chkname)[0].checked = true;"
    "       }" 
    "    }"
    "};"
    
    "function getConfig() {"
      "xhttp.open('GET', '/getconfig', true);"
      "xhttp.send();"
    "}"

    "function restart() {"
      "xhttp.open('GET', '/restart', true);"
      "xhttp.send();"
    "} "
    "function loadControl(chk) {"
      "var load = chk.name.split('_')[1];"
      "var state = chk.value;"
      "xhttp.open('GET', '/relay-control/?l=' + load + ',' + state, true);"
      "xhttp.send();"
    "} "
    "function allloads(state) {"
      "xhttp.open('GET', '/relay-control/?l=0' + ',' + state, true);"
      "xhttp.send();"
    "} "
    "function clearmsg() {"
    "  document.getElementById('msg').innerHTML = '';" 
    "} "
    "function save() {"
    "  loading=true;"  
    "  var ssid = document.getElementById('ssid').value;"
    "  var pwd  = document.getElementById('pwd').value;"
    "  var restore_state  = document.getElementsByName('restore_state')[0].checked;"
    "  xhttp.open('GET', '/save?ssid=' + ssid "
    "+ '&pwd=' + pwd "
    "+ '&restore_state=' + (restore_state? '1' : '0') "
    ", true);"
    "  xhttp.send();"
    "}"
    
    
    "</script>"
    "</head><body onLoad='getConfig()'><h3 style='color:green'>Wi-Fi Relay Board</h3>"
    "<table>"
    "<tr><td><B>SSID</B></td><td colspan='2'> : <input type='text' name='ssid' id='ssid' onChange='clearmsg();'></td></tr>"
    "<tr><td><B>Password</B></td><td colspan='2'> : <input type='password' name='pwd' id='pwd' onChange='clearmsg();'></td></tr>"
    "<tr><td><B>State on Boot</B></td><td>: <input type='radio' name='restore_state' value='1' checked onClick='clearmsg();'> RESTORE  &nbsp;</td><td><input type='radio' name='restore_state' value='0' onClick='clearmsg();'> OFF </td></tr>"
    "<tr><td>&nbsp;</td><td>&nbsp;</td></tr>"
    "<tr><td></td><td><input type='button' value='Restart' onClick='restart()'>&nbsp;&nbsp;&nbsp;&nbsp;<input type='button' value='Save' onClick='save()'></td><td id='msg'></td></td></tr>"
    "</table><BR>"
    "<HR>"
    "<table>"
    "<tr><td><B>Load 1</B></td><td>: &nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_1' value='0' checked onClick='loadControl(this);'> OFF  &nbsp;</td><td>&nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_1' value='1' onClick='loadControl(this);'> ON </td></tr>"
    "<tr><td><B>Load 2</B></td><td>: &nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_2' value='0' checked onClick='loadControl(this);'> OFF  &nbsp;</td><td>&nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_2' value='1' onClick='loadControl(this);'> ON </td></tr>"
    "<tr><td><B>Load 3</B></td><td>: &nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_3' value='0' checked onClick='loadControl(this);'> OFF  &nbsp;</td><td>&nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_3' value='1' onClick='loadControl(this);'> ON </td></tr>"
    "<tr><td><B>Load 4</B></td><td>: &nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_4' value='0' checked onClick='loadControl(this);'> OFF  &nbsp;</td><td>&nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_4' value='1' onClick='loadControl(this);'> ON </td></tr>"
    "<tr><td><B>ALL</B></td><td>: &nbsp;&nbsp;&nbsp;&nbsp; </td><td><input type='button' name='alloff' value='OFF' onClick='allloads(0);'> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; <input type='button' name='allon' value='ON' onClick='allloads(1);'> </td></tr>"
    "</table><BR>"
    "<HR>"
    
    "</body></html>";

    request->send(200, "text/html", html);
  });

  server.on("/relay-control", HTTP_GET, [](AsyncWebServerRequest *request){
    String st = "NA";
    if(request->hasParam("l")) {
      int cpin, cstate, nstate;
      AsyncWebParameter* p = request->getParam("l");
      st = p->value().c_str();
      cpin = st.substring(0, 1).toInt();
      cstate = st.substring(2).toInt();
              
      if(cpin < 5 && cpin > -1) {
        if(cpin > 0) {
          if(cstate == 0) {
            digitalWrite(pins[cpin-1], LOW);
            delayMicroseconds(10000);
          } else {
            digitalWrite(pins[cpin-1], HIGH);
            delayMicroseconds(10000);
          }

        } else {
          if(cstate == 0){
            for (int i = 0; i < 4; i++) {
              digitalWrite(pins[i], LOW);
              delayMicroseconds(10000);
            }
          }else{
            for (int i = 0; i < 4; i++) {
              digitalWrite(pins[i], HIGH);
              delayMicroseconds(10000);
            }
          }
        }
        Serial.print(getStatus());
      }
    }
  
    if(request->hasParam("t")) {
      AsyncWebParameter* p = request->getParam("t");
      st = p->value().c_str();
      int pin = st.toInt();
      int nstate = digitalRead(pins[pin-1]);
      if(nstate == HIGH){
        nstate = LOW;
      } else {
        nstate = HIGH;
      }
      digitalWrite(pins[pin-1], nstate);
      Serial.print(getStatus());
    }

    delayMicroseconds(50000);
    
    String currentstate = getStatus();
    
    String resp = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"restore_state\" : \"" + String(restore_state) 
    + "\" , \"savedstate\" : \"" + String(savedstate) 
    + "\" , \"currentstate\" : \"" + currentstate 
    + "\" , \"msg\" : \""  
    + "\" }";          
    request->send(200, "application/json", resp);
    saveConfig();
  });


  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("ssid")) {
      AsyncWebParameter* p = request->getParam("ssid");
      String ssid1 = String(p->value()).c_str();
      ssid1.toCharArray(ssid, 1+ssid1.length());
    }
    if(request->hasParam("pwd")) {
      AsyncWebParameter* p = request->getParam("pwd");
      String pwd1 = String(p->value()).c_str();
      pwd1.toCharArray(pwd, 1+pwd1.length());
    }
    if(request->hasParam("restore_state")) {
      AsyncWebParameter* p = request->getParam("restore_state");
      String restore_state1 = String(p->value()).c_str();
      restore_state = restore_state1.equals("1");
    }
    String msg = "<font color='red'>Saved!</font>";
    //String currentstate = getStatus();
    String st = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"restore_state\" : \"" + String(restore_state) 
    + "\" , \"savedstate\" : \"" + String(savedstate) 
    + "\" , \"currentstate\" : \"" + savedstate 
    + "\" , \"msg\" : \"" + msg 
    + "\" }";          
    request->send(200, "application/json", st);
    saveConfig();
  });

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    loadControl(0, 0);
    delayMicroseconds(500000);
    ESP.restart();  
  });

  server.on("/getconfig", HTTP_GET, [](AsyncWebServerRequest *request){
    //String currentstate = getStatus();
    String st = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\", \"restore_state\" : \"" + String(restore_state) 
    + "\", \"savedstate\" : \"" + String(savedstate) 
    + "\", \"currentstate\" : \"" + savedstate 
    + "\", \"msg\" : \"" 
    + "\" }";
    request->send(200, "application/json", st);
  });


  server.on("/resetconfig", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "Config Reset");
    Serial.println("deleting config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    ESP.reset();   
  });


  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send(404);
  });

  server.begin();
    
  delay(1000);
//  Serial.end();  // Have to end() it as RX and TX are being used for Digital IO 
//  delay(100);

  

  if(restore_state) {
    Serial.println("Restoring state from config");
    restoreState();
  } else {
    loadControl(0, 0);
  }
}

//--------------------------------------------------------------------------------

void restoreState() {
  for(int i=0; i<4; i++){
    loadControl(i + 1, String(savedstate).substring(2 * i, 2 * i + 1).toInt());
  } 
}

//--------------------------------------------------------------------------------

void saveConfig() {
  String sts = getStatus();
  strcpy(savedstate, sts.c_str());
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["ssid"] = ssid;
  json["pwd"] = pwd;
  json["restore_state"] = restore_state;
  json["savedstate"] = savedstate;
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }
  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
}

//--------------------------------------------------------------------------------

void initGPIO() {
  pinMode(D4, OUTPUT);
  for(int i=0; i < 4; i++)
  {
    pinMode(pins[i], OUTPUT);
    delay(100);
    digitalWrite(pins[i], LOW);
//    delayMicroseconds(50000);
  }
}

//--------------------------------------------------------------------------------

void loop(){
    digitalWrite(D4, 0);
    delay(100);
    digitalWrite(D4, 1);
    delay(5000);
  
}

//--------------------------------------------------------------------------------

void loadControl(int load, int newloadstate) {
//  Serial.println("In loadControl");
  if(load < 4  && load> -1) {
    if(load == 0) {
      for (int i = 1; i < 5; i++) {
        loadControl(i, newloadstate);
        delayMicroseconds(50000);
      }
    } else {
      digitalWrite(pins[load], newloadstate);
    }
  }
}


//--------------------------------------------------------------------------------



String getStatus() {
  s0 = digitalRead(D5);
  s1 = digitalRead(D6);
  s2 = digitalRead(D7);
  s3 = digitalRead(D8);

  String msg = (String(s0==0) + "," + String(s1==0) + "," + String(s2==0) + "," + String(s3==0));
  return msg;
}

