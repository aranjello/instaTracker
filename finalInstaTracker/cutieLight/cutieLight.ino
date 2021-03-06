#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>

#define checkPin D1
#define ledPin D6
#define numLeds 3
#define versionNum 1

char stringUpdate[50];
char unique_ID[50];

int currColor = 0;
unsigned long currTime;
unsigned long lightTime = 0;
int frame = 0;
float MaximumBrightness = 50;
int SpeedFactor = 100; // I don't actually know what would look good
int colors[][3] = {{250,0,0}
                  ,{255,35,0}
                  ,{255,98,0}
                  ,{35,173,0}
                  ,{0,173,173}
                  ,{174,81,189}
                  ,{222,68,51}
                  ,{0,0,102}
                  ,{0,0,0}
                  ,{255,255,255}
                  };
int bewoowColors[][3] = {
             {255,35,0}
            ,{0,173,173}
            ,{174,81,189}
            };

int buttonState;          
int lastButtonState = HIGH;

bool messageBack = false;

unsigned long lastDebounceTime = 0; 
unsigned long debounceDelay = 50;  
unsigned long messageStart = 0;

WiFiManager wm; // global wm instance

WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "goodTimes.mywire.org";

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numLeds, ledPin, NEO_GRB + NEO_KHZ400);


void update_started() {
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
    sprintf(stringUpdate,"http://goodtimes.mywire.org:5000/cutieLamp/?version=%d",versionNum);
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
  if(strcmp(topic,"currColor") == 0){
    char* payloadString = (char*)payload;
    payloadString[length] = '\0';
    Serial.println("received message");
    if(atoi(payloadString) >= 0){
      currColor = atoi(payloadString);
      if(!client.connected()){
        reconnect();
      }
      char col[10];
      sprintf(col,"%d",-1);
      boolean rc = client.publish("currColor", col);
    }
    if(atoi(payloadString) == -2){
      messageBack = false;
    }

  }
  if(strcmp(topic,"update") == 0){
    updateFirware();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(unique_ID,"agoodman","Dinomug96")) {
      Serial.println("connected");
      // ... and subscribe to topic
      client.subscribe("currColor");
      client.subscribe("update");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendMQTTRequest(){
  Serial.println("request attempt");
  if(!client.connected()){
    reconnect();
  }
  char col[10];
  sprintf(col,"%d",currColor);
  boolean rc = client.publish("currColor", col);
  //sprintf(text,"%d",millis());
  Serial.println("sent request1");
  messageBack = true;
  messageStart = millis();
}

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  Serial.begin(115200);
  Serial.setDebugOutput(true);  
  delay(3000);
  Serial.println("\n Starting");
  std::vector<const char *> menu = {"wifi"};
  wm.setMenu(menu);
  wm.setClass("invert");
  bool res = wm.autoConnect("AutoConnectAP","password");
  if(!res) {
    Serial.println("Failed to connect or hit timeout");
    ESP.restart();
  }
  sprintf(unique_ID,"%s",WiFi.macAddress().c_str());
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  //updateFirware();
  // put your setup code here, to run once:
  pinMode(checkPin, INPUT_PULLUP);
  strip.begin();
  strip.setBrightness(85);  // Lower brightness and save eyeballs!
  strip.show(); // Initialize all pixels to 'off'
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("starting");
}

void loop() {
  delay(5);
  client.loop();
  if(!client.connected()){
    reconnect();
  }
   // read the state of the switch into a local variable:
  int reading = digitalRead(checkPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == LOW) {
        currColor = (currColor+1)%(sizeof(colors) / sizeof(colors[0]));
        sendMQTTRequest();
      }
    }
  }
  currTime = millis();
   if(currTime%1 == 0){
    strip.setBrightness(map(analogRead(A0),0,1023,0,255));
  }
  if(currColor == (sizeof(colors) / sizeof(colors[0])-1)){
      if(currTime - lightTime > 100){
        frame += 1;
        lightTime = millis();
      }
      for (int ledNumber=0; ledNumber<numLeds; ledNumber++) {
        int ledNum = (ledNumber+frame)%(sizeof(bewoowColors) / sizeof(bewoowColors[0]));
        strip.setPixelColor(ledNumber, bewoowColors[ledNum][0], bewoowColors[ledNum][1], bewoowColors[ledNum][2]);
      }
  }else{
      // read the analog in value
      // print the readings in the Serial Monitor
   
    for (int ledNumber=0; ledNumber<numLeds; ledNumber++) {
        strip.setPixelColor(ledNumber, colors[currColor][0], colors[currColor][1], colors[currColor][2]);
    }
  }
  strip.show();
  //Wait a bit before continuing to breathe
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
  if(messageBack && (millis() - messageStart > 5000)){
    sendMQTTRequest();
  }
}
