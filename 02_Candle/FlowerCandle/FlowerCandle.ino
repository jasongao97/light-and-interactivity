/*
   Flower Candle
   - by Jason Gao
   - Fed, 2022

   Hardwares:
   - Arduino Nano 33 IoT
   - NeoPixel Jewel - 7 x 5050 RGBW LED w/ Integrated Drivers - Warm White - ~3000K

   Features:
   - Flame movement
   - Flickr
   - Turning on/off the candle by fliping the candle

   CIE1931 Implementation from:
   https://github.com/tigoe/LightProjects/tree/main/FadeCurves/CIE1931Fade
*/

#include <Adafruit_NeoPixel.h>
#include <Arduino_LSM6DS3.h>

#define PIN 16
#define NUMPIXELS 7

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRBW + NEO_KHZ800);

// An extra highlight brightness representing the flame
int brightness = 0;
int flameBrightness = 0;

// Brightness for each LED
// also storing the next brightness level for animation
uint8_t brightnesses[NUMPIXELS];
uint8_t nextBrightnesses[NUMPIXELS];
int flameLEDIndex = 0;

int candleStatus = 0; // 0:off, 1:start, 2:on

byte cie1931[256]; // pre-calculated PWM levels

unsigned long lastTimeOn = 0;
unsigned long lastTimeOff = 0;

unsigned long lastTimeUpdateLED = 0;
unsigned long lastTimeMoveFlame = 0;

void setup() {
  IMU.begin(); // initialize the accelometer
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  fillCIETable(); // pre calculate the led fade map
}

void loop() {
  if (IMU.accelerationAvailable()) {
    float x, y, z;
    IMU.readAcceleration(x, y, z);

    // Turn off the light
    if (z < -0.8) {
      // only if z < -0.8 for more than 500 milliseconds
      if (millis() - lastTimeOn > 500 && candleStatus != 0) {
        candleStatus = 0;
      }
    } else {
      lastTimeOn = millis();
    }

    // Turn on the light
    if (z > 0.8) {
      // only if z > 0.8 for more than 500 milliseconds
      if (millis() - lastTimeOff > 500 && candleStatus == 0) {
        candleStatus = 1;
        flameLEDIndex = 0;
      }
    } else {
      lastTimeOff = millis();
    }

    // TBD
    // More flicker when shaking
    float shake = x * x + y * y;
  }

  // For every second
  // there is a 1/4 chance for the flame to move to an outer LED
  // and move back in the next second
  if (millis() - lastTimeMoveFlame > 1000) {
    if (flameLEDIndex == 0) {
      if (random(0, 4) < 1) {
        // randomlly select the outer LED
        flameLEDIndex = random(1, NUMPIXELS);
      }
    } else {
      flameLEDIndex = 0;
    }

    lastTimeMoveFlame = millis();
  }

  // Modify the color every 10 milliseconds
  // which should offer animation at 100 fps
  if (millis() - lastTimeUpdateLED > 10) {
    switch (candleStatus) {
      case 0:
        // Gradually turning off
        applyChange(-4, -8);
        break;
      case 1:
        // Gradually turning on
        applyChange(3, 6);
        // Change the candleState to ON when reach the threshold brightness
        if (brightness > 100) candleStatus = 2;
        break;
      case 2:
        flicker();
        // Animate the movement of flame change
        moveFlame();
        break;
    }

    // Record the update moment
    lastTimeUpdateLED = millis();
  }

  // Update pixels
  for (int i = 0; i < NUMPIXELS; i++) {
    // Calculate the brightness with the cie1931 fade map
    byte white = cie1931[brightnesses[i]];
    // Add more warmness to the light
    byte red = map(white, 0, 255, 0, 100);
    byte green = map(white, 0, 255, 0, 20);

    pixels.setPixelColor(i, pixels.Color(red, green, 0, white));
  }
  pixels.show();
}

/**
   Apply gradually change to every pixel
*/
void applyChange(int change, int flameChange) {
  brightness += change;
  brightness = constrain(brightness, 0, 255);

  flameBrightness += flameChange;
  flameBrightness = constrain(flameBrightness, 0, 255);

  for (int i = 0; i < NUMPIXELS; i++) {
    if (i == flameLEDIndex) {
      brightnesses[i] = flameBrightness;
    } else {
      brightnesses[i] = brightness;
    }
  }
}

/**
   Randomlly apply flicker
*/
void flicker() {
  brightness += random(-1, 2);
  brightness = constrain(brightness, 80, 120);

  flameBrightness += random(-2, 3);
  flameBrightness = constrain(flameBrightness, 160, 255);
}

/**
   Animate the flame movement
*/
void moveFlame() {
  // Set the next brightness level of each pixel
  for (int i = 0; i < NUMPIXELS; i++) {
    if (i == flameLEDIndex) {
      nextBrightnesses[i] = flameBrightness;
    } else {
      nextBrightnesses[i] = brightness;
    }
  }

  // Gradualy move the flame by 1 step
  for (int i = 0; i < NUMPIXELS; i++) {
    if (nextBrightnesses[i] > brightnesses[i]) {
      brightnesses[i] += 1;
    }
    if (nextBrightnesses[i] < brightnesses[i]) {
      brightnesses[i] -= 1;
    }
  }
}

void fillCIETable() {
  /*
    For CIE, the following formulas have  Y as luminance, and
    Yn is the luminance of a white reference (basically, max luminance).
    This assumes a perceived lightness value L* between 0 and 100,
    and  a luminance value Y of 0-1.0.
    if L* > 8:  Y = ((L* + 16) / 116)^3 * Yn
    if L* <= 8: Y = L* *903.3 * Yn
  */
  // set the range of values:
  float maxValue = 255;
  // scaling factor to convert from 0-100 to 0-maxValue:
  float scalingFactor = 100 / maxValue;
  // luminance value:
  float Y = 0.0;

  // iterate over the array and calculate the right value for it:
  for (int l = 0; l <= maxValue; l++) {
    // you need to scale L from a 0-255 range to a 0-100 range:
    float lScaled = float(l) * scalingFactor;
    if ( lScaled <= 8 ) {
      Y = (lScaled / 903.3);
    } else {
      float foo = (lScaled + 16) / 116.0;
      Y = pow(foo, 3);
    }
    // multiply to get 0-maxValue, and fill in the table:
    cie1931[l] = Y * maxValue;
  }
}
