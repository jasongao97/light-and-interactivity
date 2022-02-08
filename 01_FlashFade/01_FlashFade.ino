/*
   Flash fade
   Simulate a capacitor discharge circuit
   in a camera flash light

   Fade curve reference:
   http://hyperphysics.phy-astr.gsu.edu/hbase/electric/capdis.html
*/

const int ledPin = 2;
const int buttonPin = 21;
const float timeConstant = 0.2;

unsigned long lastFlashStart;

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(buttonPin) == LOW) {
    lastFlashStart = millis();
  }

  float elapsedTime = float(millis() - lastFlashStart) / 1000;

  // calculate the brightness level with the following formula:
  // Vc = V0 * e ^ (-t / RC)
  float level = exp(-elapsedTime / timeConstant) * 255;

  Serial.println(level);
  analogWrite(ledPin, level);
  delay(10);
}
