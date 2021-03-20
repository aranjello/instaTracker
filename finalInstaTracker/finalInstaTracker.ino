#include <WiFiManager.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN    D6
#define versionNum 1

char stringUpdate[50];
char unique_ID[50];
WiFiManager wm; // global wm instance
ESP8266WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

WiFiClient espClient;
PubSubClient client(espClient);

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

bool firstResponse = false;
bool firstLoop = true;
bool errorOn = false;

int timeTime = 60000;
int instaTime = 60000;
int lastCount = 0;
int brightVal = 0;

bool haveClock = false;
bool doTime = false;
bool responseWait = false;

long currentTime;
long hourTimer = 0;
long responseTimer = 0;

const char* mqtt_server = "goodTimes.mywire.org";

char postForms[4096];
char text[50];
char userName[50];
bool flashing = true;

void update_started() {
  sprintf(text,"update");
  P.displayReset();
  P.displayAnimate();
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

bool updateFirware(){
    
    t_httpUpdate_return ret;
    sprintf(stringUpdate,"http://goodtimes.mywire.org:5000/?version=%d",versionNum);
    ret=ESPhttpUpdate.update(espClient,stringUpdate);     // **************** This is the line that "does the business"   
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char* payloadString = (char*)payload;
  payloadString[length] = '\0';
  Serial.print("received message: ");
  Serial.println(payloadString);
  if(!strcmp("recieved",payloadString)){
    Serial.println("got response to message");
    responseWait = false;
  }
  if(!strcmp("error",payloadString)){
    Serial.println("got error");
    sprintf(text,"error 1");
    errorOn = true;
  }else{
    errorOn = false;
    lastCount = atoi(payloadString);
    sprintf(text,"%d",lastCount);
  }
  sendMQTTRequest("gotMessage");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(unique_ID,"agoodman","Dinomug96")) {
      Serial.println("connected");
      // ... and subscribe to topic
      client.subscribe(userName);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void getTime(char *psz, bool f = true)
// Code for reading clock time
{
  timeClient.update();
  int myTime = timeClient.getHours();
  sprintf(psz, "%2d%c%02d", (myTime > 12 ? myTime - 12 : myTime), (f ? ':' : ' '), timeClient.getMinutes());
}

void handleRoot() {
  sprintf(postForms,"<html>\
  <head>\
    <title>Instatracker setup page</title>\
    <style>\
      body { background-color: light blue; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; text-align: center;}\
      form { display: inline-block;}\
    </style>\
  </head>\
  <body onload=\"myFunction()\">\
    <h1>Instatracker setup page</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
      <label for=\"fname\">Insta Username:</label><br>\
      <input type=\"text\" name=\"username\" value=\"%s\"><br><br>\
      <label for=\"fname\">Brightness:</label><br>\
      <input type=\"number\" min=\"0\" max=\"15\" name=\"brightness\" value=\"%d\"><br><br>\
      <label for=\"fname\">Display a Clock?</label>\
      <input type=\"checkbox\" id=\"vehicle1\" name=\"vehicle1\" value=\"Bike\" %s onclick=\"myFunction()\"><br><br>\
      <label for=\"fname\" id=\"text3\" style=\"display:none\">Insta time:</label><br>\
      <input type=\"number\" min=\"0\" id=\"text4\"step=\"any\" min=\"0\" name=\"hello\" value=\"%.2f\" style=\"display:none\"><br><br>\
      <label for=\"fname\" id=\"text\" style=\"display:none\">Clock Time:</label><br>\
      <input type=\"number\" min=\"0\" id=\"text2\" step=\"any\" min=\"0\" name=\"new\" value=\"%.2f\" style=\"display:none\"><br><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
    <script>\
      function myFunction() {\
        var checkBox = document.getElementById(\"vehicle1\");\
        var text1 = document.getElementById(\"text\");\
        var text2 = document.getElementById(\"text2\");\
        var text3 = document.getElementById(\"text3\");\
        var text4 = document.getElementById(\"text4\");\
        if (checkBox.checked == true){\
          text1.style.display = \"inline-block\";\
          text2.style.display = \"inline-block\";\
          text3.style.display = \"inline-block\";\
          text4.style.display = \"inline-block\";\
        } else {\
           text1.style.display = \"none\";\
           text2.style.display = \"none\";\
           text3.style.display = \"none\";\
           text4.style.display = \"none\";\
        }\
      }\
    </script>\
  </body>\
</html>",userName,brightVal,(haveClock ? "checked":""),instaTime/60000.0,timeTime/60000.0);
  server.send(200, "text/html", postForms);
}

void handleForm() {

  haveClock = false;
  if (server.method() == HTTP_POST) {
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      if(server.argName(i) == "vehicle1"){
        haveClock = true;
      }
      Serial.println(haveClock);
      if(server.argName(i) == "brightness"){
        brightVal = server.arg(i).toInt();
        P.setIntensity(brightVal);
      }
      if(server.argName(i) == "hello"){
        //sprintf(text, "%s", server.arg(i).c_str());
        instaTime = server.arg(i).toFloat()*60000;
        currentTime = millis() + instaTime;
        Serial.println(instaTime);
      }
      if(server.argName(i) == "new"){
        timeTime = server.arg(i).toFloat()*60000;
        Serial.println(timeTime);
      }
      if(server.argName(i) == "username"){
        client.unsubscribe(userName);
        sprintf(userName,"%s",server.arg(i).c_str());
        client.subscribe(userName);
      }
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(200, "text/html", "<meta http-equiv = \"refresh\" content = \"0;url = \/\" />");
    Serial.println(message);
    sendMQTTRequest(userName);
    firstResponse = true;
    sprintf(text,"loading");
    P.displayReset();
  }
}

void handleNotFound() {
  server.send(200, "text/html", "<meta http-equiv = \"refresh\" content = \"0;url = \/\" />");
}

void setup() {
  P.begin();
  P.setIntensity(brightVal);
  P.displayClear();
  P.displayReset();
  P.setTextAlignment(PA_CENTER);
  P.setZoneEffect(0,true,PA_FLIP_UD);
  P.setZoneEffect(0,true,PA_FLIP_LR);
  P.setTextEffect(PA_NO_EFFECT,PA_NO_EFFECT);
  P.setPause(0);
  P.setSpeed(250);
  P.setTextBuffer(text);
  sprintf(userName,"%s","Enter Username");
  sprintf(text,"%s","test");
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  Serial.begin(115200);
  Serial.setDebugOutput(true);  
  delay(3000);
  Serial.println("\n Starting");
  std::vector<const char *> menu = {"wifi"};
  wm.setMenu(menu);
  wm.setClass("invert");
  //wm.resetSettings();
  bool res = wm.autoConnect("instaTracker","followers");
  if(!res) {
    Serial.println("Failed to connect or hit timeout");
    ESP.restart();
  }
  sprintf(unique_ID,"%s",WiFi.macAddress().c_str());
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  updateFirware();
  // put your setup code here, to run once:
  HTTPClient http;
  // Send request
  http.begin("http://ip-api.com/json/?fields=status,offset");
  Serial.println(http.GET());
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, http.getString());
  http.end();
  int offset = doc["offset"];
  Serial.println("offset is");
  Serial.println(offset);
  timeClient.begin();
  timeClient.setTimeOffset(offset);
  sprintf(text, "%s", WiFi.localIP().toString().c_str());
  P.setTextEffect(PA_SCROLL_RIGHT,PA_SCROLL_RIGHT);
  server.on("/", handleRoot);

  server.on("/postform/", handleForm);

  server.onNotFound(handleNotFound);
  server.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  firstResponse = false;
  currentTime = millis();
  
}

void sendMQTTRequest(char* request){
  Serial.println("request attempt");
  if(!client.connected()){
    reconnect();
  }
  boolean rc = client.publish("requestData", request);
  //sprintf(text,"%d",millis());
  Serial.println("sent request");
  if(strcmp(request,"gotMessage")){
    responseWait = true;
    responseTimer = millis();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  client.loop();
  if(firstResponse){
    if(!client.connected()){
      reconnect();
    }
    if(millis() - hourTimer > 3600000){
      sendMQTTRequest(userName);
      hourTimer = millis();
    }
    P.setTextEffect(PA_NO_EFFECT,PA_NO_EFFECT);
    if(haveClock){
      if(P.displayAnimate() && doTime && millis() - currentTime > timeTime){
        currentTime = millis();
        doTime = false;
        Serial.println("Time time done");
        P.setPause(0);
        P.setSpeed(10);
        sprintf(text,"%d",lastCount);
        //P.setTextBuffer(text);
      }else if(P.displayAnimate() && !doTime && millis() - currentTime > instaTime){
        currentTime = millis();
        doTime = true;
        Serial.println("insta time Done");
        P.setPause(0);
        P.setSpeed(250);
        //P.setTextBuffer(szTime);
      }
      if(doTime){
        if(P.displayAnimate()){
          flashing = !flashing;
          getTime(text,flashing);
          P.displayReset();
        }
      }else{
        if(P.displayAnimate()){
          P.displayReset();
        }
      }
    }else{
      if(!errorOn && lastCount != 0){
         sprintf(text,"%d",lastCount);
      }
      if(P.displayAnimate()){
          P.displayReset();
      }
    }
    if(responseWait && (millis() - responseTimer > 30000)){
      Serial.println("no response,resend");
      sendMQTTRequest(userName);
    }
  }else{
    if(P.displayAnimate()){
      //sprintf(szTime, "%s", WiFi.localIP().toString().c_str());
      P.displayReset();
    }
  }
}
