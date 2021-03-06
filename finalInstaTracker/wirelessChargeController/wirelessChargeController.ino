#include <Adafruit_NeoPixel.h>
#include <Servo.h>

#define checkPin D2
#define ledPin D5
#define numLeds 4
#define servoPin D8

Servo myservo; // create servo object to control a servo

int frame = 0;
float MaximumBrightness = 50;
float SpeedFactor = 0.008; // I don't actually know what would look good
float StepDelay = 5; // ms for a step delay on the ligh

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numLeds, ledPin, NEO_GRB + NEO_KHZ400);
  // put your setup code here, to run once:
  void setup() {
  pinMode(D3, OUTPUT);    // sets the digital pin 13 as output
  pinMode(D2, INPUT);
  myservo.attach(servoPin);
  strip.begin();
  strip.setBrightness(85);  // Lower brightness and save eyeballs!
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(9600);
  Serial.println("starting");
}

void loop() {
  // put your main code here, to run repeatedly:
  int val = digitalRead(D2);
  float intensity = (sin(frame * SpeedFactor)+1)/2 * MaximumBrightness;
  if(val){
      strip.setBrightness(intensity);
      myservo.write(179);
  }else{
      myservo.write(0);
      strip.setBrightness(0);
  }

  for (int ledNumber=0; ledNumber<numLeds; ledNumber++) {
    strip.setPixelColor(ledNumber, 0, 0, 255);
  }
  strip.show();
  //Wait a bit before continuing to breathe
  delay(StepDelay);
  frame += 1;
      
}

/* Controlling a servo position using a potentiometer (variable resistor) */
