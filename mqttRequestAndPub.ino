/*
  Button

  Turns on and off a light emitting diode(LED) connected to digital pin 13,
  when pressing a pushbutton attached to pin 2.

  The circuit:
  - LED attached from pin 13 to ground
  - pushbutton attached to pin 2 from +5V
  - 10K resistor attached to pin 2 from ground

  - Note: on most Arduinos there is already an LED on the board
    attached to pin 13.

  created 2005
  by DojoDave <http://www.0j0.org>
  modified 30 Aug 2011
  by Tom Igoe

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Button
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
// Define hardware type, size, and output pins:
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN D3
const char* UNIQUE_ID = "dfg";
const int buttonPin = D2;
unsigned long delayStart = 0; // the time the delay started
unsigned long delayInterval = 1000*60;
// Create a new instance of the MD_Parola class with hardware SPI connection:
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
 
// Connect to the WiFi
const char* ssid = "goodmanwifi2.4";
const char* password = "goodman1234";
const char* mqtt_server = "67.169.167.183";
boolean messageRecived = true;
 
WiFiClient espClient;
PubSubClient client(espClient);
 
boolean buttonUp = true;
void callback(char* topic, byte* payload, unsigned int length) {
  char* payloadString = (char*)payload;
  payloadString[length] = '\0';
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.displayClear();
  myDisplay.print(payloadString);
  Serial.println(payloadString);
  messageRecived = true;
}
 
 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(UNIQUE_ID,"agoodman","Dinomug96")) {
      Serial.println("connected");
      // ... and subscribe to topic
      client.subscribe(UNIQUE_ID);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
 
void setup()
{
   Serial.begin(115200);
 
  int numberOfNetworks = WiFi.scanNetworks();
 
  for(int i =0; i<numberOfNetworks; i++){
 
      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
 
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  pinMode(buttonPin, INPUT);
  // Intialize the object:
  myDisplay.begin();
  // Set the intensity (brightness) of the display (0-15):
  myDisplay.setIntensity(0);
  // Clear the display:
  myDisplay.displayClear();
  sendMessage();
}
 
void loop()
{
  if(!messageRecived){
    if((millis() - delayStart) > 1000*60){
      sendMessage();
    }
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }else{
    if((millis() - delayStart) > delayInterval){
      sendMessage();
    }
  }
}

void sendMessage(){
  while (!client.connected()) {
    reconnect();
  }
  boolean rc = client.publish("requestData", UNIQUE_ID);
  messageRecived = false;
  delayStart = millis();
  Serial.println("requestSent");
}
