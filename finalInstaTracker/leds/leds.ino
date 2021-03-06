#include <Adafruit_NeoPixel.h>

#define PIN D5
#define interuptPin D1
#define numLeds 4

int frame = 0;
int red = random(0,255);
int green = random(0,255);
int blue = random(0,255);

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numLeds, PIN, NEO_GRB + NEO_KHZ400);

void ICACHE_RAM_ATTR handleInterrupt() {
  Serial.println("Interrupt Detected");
  red = random(0,255);
  green = random(0,255);
  blue = random(0,255);
}

void setup() {
  strip.begin();
  strip.setBrightness(85);  // Lower brightness and save eyeballs!
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(115200); 
  pinMode(interuptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interuptPin), handleInterrupt, RISING);
}

void loop() {
//Written by: Jason Yandell

float MaximumBrightness = 50;
float SpeedFactor = 0.008; // I don't actually know what would look good
float StepDelay = 5; // ms for a step delay on the lights

// Make the lights breathe
// Intensity will go from 10 - MaximumBrightness in a "breathing" manner
float intensity = (sin(frame * SpeedFactor)+1)/2 * MaximumBrightness;
strip.setBrightness(intensity);
// Now set every LED to that color
for (int ledNumber=0; ledNumber<numLeds; ledNumber++) {
strip.setPixelColor(ledNumber, red, green, blue);
}

strip.show();
//Wait a bit before continuing to breathe
delay(StepDelay);
frame += 1;
}
