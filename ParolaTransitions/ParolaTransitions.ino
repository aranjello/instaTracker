#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>



const char *ssid = "MaxWiFi";
const char *password = "4153887413";

const long utcOffsetInSeconds = 14400;


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CS_PIN    D6

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

int currEffect = 1;

textEffect_t  effect[] =
{
  PA_PRINT,
  PA_SCAN_HORIZ,
  PA_SCROLL_LEFT,
  PA_WIPE,
  PA_SCROLL_UP_LEFT,
  PA_SCROLL_UP,
  PA_OPENING_CURSOR,
  PA_GROW_UP,
  PA_MESH,
  PA_SCROLL_UP_RIGHT,
  PA_BLINDS,
  PA_CLOSING,
  PA_RANDOM,
  PA_GROW_DOWN,
  PA_SCAN_VERT,
  PA_SCROLL_DOWN_LEFT,
  PA_WIPE_CURSOR,
  PA_DISSOLVE,
  PA_OPENING,
  PA_CLOSING_CURSOR,
  PA_SCROLL_DOWN_RIGHT,
  PA_SCROLL_RIGHT,
  PA_SLICE,
  PA_SCROLL_DOWN,
};

long currentMillis = millis();

char szTime[9];
bool flashing = true;
void getTime(char *psz, bool f = true)
// Code for reading clock time
{
  timeClient.update();
  int myTime = timeClient.getHours();
  sprintf(psz, "%2d%c%02d", (myTime > 12 ? myTime - 12 : myTime), (f ? ':' : ' '), timeClient.getMinutes());
}
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  
  while ( WiFi.status() != WL_CONNECTED ) {
  delay ( 500 );
  Serial.print ( "." );
  }
  
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
  // put your setup code here, to run once:
  P.begin();
  P.setIntensity(0);
  P.displayClear();
  P.displayReset();
  P.setTextAlignment(PA_CENTER);
  P.setZoneEffect(0,true,PA_FLIP_UD);
  P.setZoneEffect(0,true,PA_FLIP_LR);
  P.setTextEffect(effect[2],effect[2]);
  P.setPause(2000);
  P.setSpeed(100);
  P.setTextBuffer(szTime);
  currentMillis = millis();
}

void loop() {
  if(P.displayAnimate()){
    getTime(szTime);
    P.displayReset();
  }
}
