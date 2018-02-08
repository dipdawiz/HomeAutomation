#include <FS.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>

//--------------------------------------------------------------------------------
char ssid[40];
char pwd[40];
char mqtt_server[20] = "172.20.1.120";
int enable_mqtt = 0;
int restore_state = 1;
char savedstate[22] = "0,0,0,0,0|0,0,0,0,0";
int  mqtt_port = 1883;
int st0, st1, st2, st3, st4, s0, s1, s2, s3, s4, r0, r1, r2, r3, r4;
String p;
const int rx = 3;
const int tx = 1;
const char subscribeTopic[] = "relay-control";
const char publishTopic[] = "relay-status";
int pins[] = {rx, D5, D7, D1, D3, D0, D6, tx, D2, D4};
int numPins = 10;
int pin, state;
WiFiClient client;
Adafruit_MQTT_Subscribe *subscription;
void MQTT_connect();
Adafruit_MQTT_Client mqtt(&client, mqtt_server, mqtt_port, "", "");
Adafruit_MQTT_Subscribe mqtt_sub = Adafruit_MQTT_Subscribe(&mqtt, subscribeTopic, 1);
Adafruit_MQTT_Publish mqtt_pub = Adafruit_MQTT_Publish(&mqtt, publishTopic, 1);
IPAddress myIP;
const char *ap_ssid = "RelayBoard";
const char *ap_pwd = "";
AsyncWebServer server(80);

//--------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
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
          if(json.containsKey("mqtt_server") )  
            strcpy(mqtt_server, json["mqtt_server"]);
          if(json.containsKey("enable_mqtt") )  
            enable_mqtt = json["enable_mqtt"]; 
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
//    enable_mqtt = 1;
    myIP = WiFi.localIP();
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: "); Serial.println(myIP);
    
  } else {
    Serial.println();
    Serial.print("Could not connect to WiFi:"); Serial.println(ssid);
    Serial.println("Starting Access Point ...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid);
    myIP = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(myIP);
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

  if(enable_mqtt) mqtt.subscribe(&mqtt_sub);



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
    "         document.getElementById('mqtt_server').value = resp['mqtt_server'];"
    "         if(resp['enable_mqtt'] === '1') document.getElementsByName('enable_mqtt')[0].checked = true;"
    "         else document.getElementsByName('enable_mqtt')[1].checked = true;"
    "         if(resp['restore_state'] === '1') document.getElementsByName('restore_state')[0].checked = true;"
    "         else document.getElementsByName('restore_state')[1].checked = true;"
    "         document.getElementById('msg').innerHTML = resp['msg'];"
    "       }"
    "       var loadstates = (resp['currentstate'].split('|')[0]).split(',');"
    "       for(var j=0; j< loadstates.length; j++) {"
    "          var chkname = 'load_' + (j+1);"
    "          if(loadstates[j] === '1') document.getElementsByName(chkname)[1].checked = true;"
    "          else document.getElementsByName(chkname)[0].checked = true;"
    "       }"   
    "    }"
    "};"
    "function init(poll) {"
      "loading = poll;"
      "xhttp.open('GET', '/getconfig', true);"
      "xhttp.send();"
    "} "
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
    "loading = true;"
    "  var ssid = document.getElementById('ssid').value;"
    "  var pwd  = document.getElementById('pwd').value;"
    "  var mqtt_server  = document.getElementById('mqtt_server').value;"
    "  var enable_mqtt  = document.getElementsByName('enable_mqtt')[0].checked;"
    "  var restore_state  = document.getElementsByName('restore_state')[0].checked;"
    "  xhttp.open('GET', '/save?ssid=' + ssid "
    "+ '&pwd=' + pwd "
    "+ '&mqtt_server=' + mqtt_server "
    "+ '&enable_mqtt=' + (enable_mqtt? '1' : '0') "
    "+ '&restore_state=' + (restore_state? '1' : '0') "
    ", true);"
    "  xhttp.send();"
    "}"

    "window.setInterval(function(){"
    "   init(false);"
    "}, 3000);"
    
    "</script>"
    "</head><body onLoad='init(true)'><h3 style='color:green'>Wi-Fi Relay Board</h3>"
    "<table>"
    "<tr><td><B>SSID</B></td><td colspan='2'> : <input type='text' name='ssid' id='ssid' onChange='clearmsg();'></td></tr>"
    "<tr><td><B>Password</B></td><td colspan='2'> : <input type='password' name='pwd' id='pwd' onChange='clearmsg();'></td></tr>"
    "<tr><td><B>MQTT Server</B></td><td colspan='2'> : <input type='text' name='mqtt_server' id='mqtt_server' onChange='clearmsg();'></td></tr>"
    "<tr><td><B>MQTT</B></td><td>: <input type='radio' name='enable_mqtt' value='1' checked onClick='clearmsg();'> ENABLE  &nbsp;</td><td><input type='radio' name='enable_mqtt' value='0' onClick='clearmsg();'> DISABLE </td></tr>"
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
    "<tr><td><B>Load 5</B></td><td>: &nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_5' value='0' checked onClick='loadControl(this);'> OFF  &nbsp;</td><td>&nbsp;&nbsp;&nbsp;&nbsp;<input type='radio' name='load_5' value='1' onClick='loadControl(this);'> ON </td></tr>"
    "<tr><td><B>ALL</B></td><td>: &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type='button' name='alloff' value='OFF' onClick='allloads(0);'> &nbsp;</td><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type='button' name='allon' value='ON' onClick='allloads(1);'> </td></tr>"
    "</table><BR>"
    "<HR>"
    
    "</body></html>";

    request->send(200, "text/html", html);
  });

  server.on("/relay-control", HTTP_GET, [](AsyncWebServerRequest *request){
    String st = "NA";
    if(request->hasParam("r")) {
      AsyncWebParameter* p = request->getParam("r");
      st = p->value().c_str();
      for (int i = 0; i < st.length(); i++) {
        if (st.substring(i, i+1) == ",") {
          pin = st.substring(0, i).toInt();
          state = st.substring(i+1).toInt();
          break;
        }
      }
      if(pin<6 && pin>-1) {
        if(pin == 0) {
          for (int i = 0; i < 5; i++) {
            digitalWrite(pins[i], state);
          }
        } else digitalWrite(pins[pin-1], state);
      }
    }

    if(request->hasParam("t")) {
      AsyncWebParameter* p = request->getParam("t");
      st = p->value().c_str();
      pin = st.toInt();
      if(pin < 6 && pin>0) {
        int pinstate = digitalRead(pins[pin-1]);
        if(pinstate == LOW) state = HIGH;
        else state = LOW;
        digitalWrite(pins[pin-1], state);
      }
    }

    if(request->hasParam("l")) {
      int cpin, cstate, nstate;
      AsyncWebParameter* p = request->getParam("l");
      st = p->value().c_str();
      for (int i = 0; i < st.length(); i++) {
        if (st.substring(i, i+1) == ",") {
          cpin = st.substring(0, i).toInt();
          cstate = st.substring(i+1).toInt();
          break;
        }
      }
      if(cpin < 6 && cpin>-1) {
        if(cpin == 0) {
          for (int i = 1; i < 6; i++) {
            int loadstate = digitalRead(pins[4 + i]);
            if(loadstate == cstate) {
              int pinstate = digitalRead(pins[i-1]);
              if(pinstate == LOW) nstate = HIGH;
              else nstate = LOW;
              delayMicroseconds(50000);
              digitalWrite(pins[i-1], nstate);
            }
          }
        } else {
          int loadstate = digitalRead(pins[4 + cpin]);
          if(loadstate == cstate) {
            int pinstate = digitalRead(pins[cpin-1]);
            if(pinstate == LOW) nstate = HIGH;
            else nstate = LOW;
            digitalWrite(pins[cpin-1], nstate);
          }
        }
      }
    }
    delayMicroseconds(100000);
    String currentstate = getStatus();
    saveConfig();
    String resp = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"mqtt_server\" : \"" + String(mqtt_server) 
    + "\" , \"enable_mqtt\" : \"" + String(enable_mqtt) 
    + "\" , \"restore_state\" : \"" + String(restore_state) 
    + "\" , \"savedstate\" : \"" + String(savedstate) 
    + "\" , \"currentstate\" : \"" + currentstate 
    + "\" , \"msg\" : \""  
    + "\" }";          
    request->send(200, "application/json", resp);
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
    if(request->hasParam("mqtt_server")) {
      AsyncWebParameter* p = request->getParam("mqtt_server");
      String mqtt_server1 = String(p->value()).c_str();
      mqtt_server1.toCharArray(mqtt_server, 1+mqtt_server1.length());
    }
    if(request->hasParam("enable_mqtt")) {
      AsyncWebParameter* p = request->getParam("enable_mqtt");
      String enable_mqtt1 = String(p->value()).c_str();
      enable_mqtt = enable_mqtt1.equals("1");
    }
    if(request->hasParam("restore_state")) {
      AsyncWebParameter* p = request->getParam("restore_state");
      String restore_state1 = String(p->value()).c_str();
      restore_state = restore_state1.equals("1");
    }
    String msg = "<font color='red'>Saved!</font>";
    String currentstate = getStatus();
    String st = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"mqtt_server\" : \"" + String(mqtt_server) 
    + "\" , \"enable_mqtt\" : \"" + String(enable_mqtt) 
    + "\" , \"restore_state\" : \"" + String(restore_state) 
    + "\" , \"savedstate\" : \"" + String(savedstate) 
    + "\" , \"currentstate\" : \"" + currentstate 
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
    String currentstate = getStatus();
    String st = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"mqtt_server\" : \"" + String(mqtt_server) 
    + "\", \"enable_mqtt\" : \"" + String(enable_mqtt) 
    + "\", \"restore_state\" : \"" + String(restore_state) 
    + "\", \"savedstate\" : \"" + String(savedstate) 
    + "\", \"currentstate\" : \"" + currentstate 
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
  Serial.end();  // Have to end() it as RX and TX are being used for Digital IO 
  delay(100);

  initGPIO();

  if(restore_state) restoreState();
  else loadControl(0, 0);
  
  if(enable_mqtt) {
    mqtt.subscribe(&mqtt_sub);
    sendStatus(getStatus());
  }
}

//--------------------------------------------------------------------------------

void restoreState() {
  for(int i=0; i<5; i++) loadControl(1 + i, String(savedstate).substring(2 * i, 2 * i + 1).toInt());
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
  json["mqtt_server"] = mqtt_server;
  json["enable_mqtt"] = enable_mqtt;
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
  pinMode(rx, FUNCTION_3);
  pinMode(tx, FUNCTION_3);
  for(int i=0; i < 5; i++)
  {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }

  for(int i=5; i < 10; i++)
  {
    pinMode(pins[i], INPUT);
  }

}

//--------------------------------------------------------------------------------

void readMQTTMessage() {
  //Serial.begin(115200);
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &mqtt_sub) {
      p = String((char *)mqtt_sub.lastread);
      char cmd = p.charAt(0);
      pin = p.substring(2, 3).toInt();
      if(cmd != 't') state = p.substring(4).toInt();
      Serial.print("p="); Serial.println(p);
      Serial.print("cmd="); Serial.print(cmd); Serial.print(", pin="); Serial.print(pin); Serial.print(", state="); Serial.println(state); 
      if(cmd == 'r') relayControl(pin, state);
      else if(cmd == 't') relayToggle(pin);
      else if(cmd == 'l') loadControl(pin, state);
      delay(100);
      String sts = (sendStatus(getStatus()));
      strcpy(savedstate, sts.c_str());
      saveConfig();
    }
  }
//  Serial.end();
}

//--------------------------------------------------------------------------------

void relayControl(int relay, int newstate) {
//  Serial.println("In relayControl");
  if(relay<6 && relay> -1) {
    if(relay == 0) {
      for (int i = 0; i < 5; i++) {
        digitalWrite(pins[i], newstate);
      }
    } else digitalWrite(pins[relay-1], newstate);
  }
}

//--------------------------------------------------------------------------------

void relayToggle(int pin) {
  int nstate;
//    Serial.println("In relayToggle");
  if(pin < 6  && pin> 0) {
    int pinstate = digitalRead(pins[pin-1]);
    if(pinstate == LOW) nstate = HIGH;
    else nstate = LOW;
    digitalWrite(pins[pin-1], nstate);
  }
}

//--------------------------------------------------------------------------------

void loadControl(int load, int newloadstate) {
//  Serial.println("In loadControl");
  if(load < 6  && load> -1) {
    if(load == 0) {
      for (int i = 1; i < 6; i++) {
        loadControl(i, newloadstate);
        delayMicroseconds(50000);
      }
    } else {
      int loadstate = digitalRead(pins[4 + load]);
      if(loadstate == newloadstate) {   // Since loadstate is inverted output from MID400
        relayToggle(load);
      }
    }
  }
}

//--------------------------------------------------------------------------------

void loop() {
  if(enable_mqtt) 
  {
    MQTT_connect();
    readMQTTMessage();
    if(! mqtt.ping()) {
      mqtt.disconnect();
    }
    sendStatus(getStatus());
  }
}

//--------------------------------------------------------------------------------

void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (!enable_mqtt || mqtt.connected()) {
    return;
  }
  //Serial.begin(115200);
  Serial.println("Connecting to MQTT... ");

  int retries = 10;
  while (enable_mqtt && (ret = mqtt.connect()) != 0) { // connect will return 0 for connected
     Serial.print(mqtt.connectErrorString(ret));
     Serial.print(". Retrying MQTT connection ... # "); Serial.println(10-retries + 1);
     mqtt.disconnect();
     delay(100);  // wait 10 seconds
     retries--;
     if (retries < 0) {
       enable_mqtt = 0;
       delayMicroseconds(100000);
       saveConfig();
     }
  }
  if(ret == 0) Serial.println("MQTT Connected!");
  else Serial.println("Unable to connect to MQTT Server. Disabling MQTT!");
}

//--------------------------------------------------------------------------------

String sendStatus(String msg)
{
//  Serial.println(msg);
  char msgBuf[25];
  msg.toCharArray(msgBuf, 25);
  mqtt_pub.publish(msgBuf);
  saveConfig();
  return msg;
}

//--------------------------------------------------------------------------------

String getStatus() {
  s0 = digitalRead(D0);
  s1 = digitalRead(D6);
  s2 = digitalRead(tx);
  s3 = digitalRead(D2);
  s4 = digitalRead(D4);
  r0 = digitalRead(rx);
  r1 = digitalRead(D5);
  r2 = digitalRead(D7);
  r3 = digitalRead(D1);
  r4 = digitalRead(D3);


  String msg = (String(s0==0) + "," + String(s1==0) + "," + String(s2==0) + "," + String(s3==0) + "," + String(s4==0)  + "|");
  msg += (String(r0) + "," + String(r1) + "," + String(r2) + "," + String(r3) + "," + String(r4) );
  return msg;
}

//--------------------------------------------------------------------------------
