/*
   Color Synth

   Controling the LED with 4 potentiometer
   repersenting hue, saturation, brightness, and the extra white channel
*/

#include <Adafruit_NeoPixel.h>

#define PIN 6 // On Trinket or Gemma, suggest changing this to 1

#define NUMPIXELS 7 // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRBW + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
}

void loop() {
  uint16_t hue = map(analogRead(A3), 1023, 0, 0, 65536);
  uint8_t saturation = map(analogRead(A6), 1023, 0, 0, 255);
  uint8_t brightness = map(analogRead(A2), 1023, 0, 0, 255);
  uint8_t white = map(analogRead(A4), 1023, 0, 0, 255);

  uint32_t rgbColor = pixels.ColorHSV(hue, saturation, brightness);
  uint32_t color = ((uint32_t) white << 24) + rgbColor;

  color = pixels.gamma32(color);
  Serial.println(color, HEX);

  for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}
