#include "HomeSpan.h"
#include <FastLED.h>

#define PAIRING_CODE "XXXXXXXX"   

// Pin definitions
#define LED_PIN 9
#define BUTTON_PIN 4
#define NUM_LEDS 8

// LED array
CRGB leds[NUM_LEDS];

////////////////////////////////////////////
// Reset HomeSpan stored data
////////////////////////////////////////////
void resetHomeSpanData() {
  Serial.println("Resetting HomeSpan stored data...");
  homeSpan.deleteStoredValues();
  Serial.println("HomeSpan data reset complete!");
}

////////////////////////////////////////////
// Simple RGB Lightbulb Service
////////////////////////////////////////////
struct RGBLightbulb : Service::LightBulb {
  SpanCharacteristic *power;
  SpanCharacteristic *brightness;
  SpanCharacteristic *hue;
  SpanCharacteristic *saturation;

  bool lastButtonState = LOW;
  unsigned long lastButtonTime = 0;
  const unsigned long debounceDelay = 50;

  // Smooth brightness transition
  float currentBrightness = 0.0f;
  float targetBrightness = 0.0f;

  // Color transition
  CRGB currentColor;
  CRGB targetColor;

  // Timing
  unsigned long lastUpdateTime = 0;
  const unsigned long updateInterval = 20;
  const float fadeSpeed = 0.07f;

  // Gamma correction
  uint8_t gammaCorrect(uint8_t value) {
    return dim8_video(value);
  }

  // Convert HSV to RGB
  CRGB hueToRGB(float h, int s, int v) {
    float hNorm = h / 360.0f;
    float sNorm = s / 100.0f;
    float vNorm = v / 100.0f;
    
    float c = vNorm * sNorm;
    float x = c * (1.0f - fabs(fmod(hNorm * 6.0f, 2.0f) - 1.0f));
    float m = vNorm - c;
    float r, g, b;
    float h6 = hNorm * 6.0f;

    if (h6 < 1.0f)      { r = c; g = x; b = 0; }
    else if (h6 < 2.0f) { r = x; g = c; b = 0; }
    else if (h6 < 3.0f) { r = 0; g = c; b = x; }
    else if (h6 < 4.0f) { r = 0; g = x; b = c; }
    else if (h6 < 5.0f) { r = x; g = 0; b = c; }
    else                { r = c; g = 0; b = x; }

    return CRGB(
      gammaCorrect((uint8_t)((r + m) * 255.0f)),
      gammaCorrect((uint8_t)((g + m) * 255.0f)),
      gammaCorrect((uint8_t)((b + m) * 255.0f))
    );
  }

  RGBLightbulb() : Service::LightBulb() {
    power = new Characteristic::On(0);
    brightness = new Characteristic::Brightness(100);
    hue = new Characteristic::Hue(0);
    saturation = new Characteristic::Saturation(100);

    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(255);
    FastLED.setCorrection(TypicalLEDStrip);
    FastLED.setDither(BINARY_DITHER);
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);

    currentColor = CRGB::Black;
    targetColor = CRGB::White;
    currentBrightness = targetBrightness = 0.0f;

    FastLED.clear();
    FastLED.show();

    Serial.println("RGB Lightbulb initialized");
  }

  boolean update() {
    boolean newPower = power->getNewVal();
    int newBrightness = brightness->getNewVal();
    float newHue = hue->getNewVal();
    int newSaturation = saturation->getNewVal();

    Serial.printf("HomeKit: Power=%s, H=%.1f, S=%d%%, V=%d%%\n",
                  newPower ? "ON" : "OFF", newHue, newSaturation, newBrightness);

    if (newPower) {
      float adjBright = (newBrightness <= 1) ? 0.02f : newBrightness / 100.0f;
      targetBrightness = adjBright;
      targetColor = hueToRGB(newHue, newSaturation, 100);
    } else {
      Serial.println("Turning OFF");
      targetBrightness = 0.0f;
    }
    
    return true;
  }

  void loop() {
    // Smooth transitions
    if (millis() - lastUpdateTime >= updateInterval) {
      lastUpdateTime = millis();

      // Brightness transition
      if (abs(currentBrightness - targetBrightness) > 0.001f) {
        float diff = targetBrightness - currentBrightness;
        currentBrightness += diff * fadeSpeed;
        if (abs(diff) < 0.01f) currentBrightness = targetBrightness;
        currentBrightness = constrain(currentBrightness, 0.0f, 1.0f);
      }

      // Color transition
      if (currentColor != targetColor) {
        currentColor = blend(currentColor, targetColor, (uint8_t)(fadeSpeed * 255.0f));
      }

      // Update LEDs
      if (currentBrightness > 0.001f) {
        CRGB displayColor = CRGB(
          (uint8_t)(currentColor.r * currentBrightness),
          (uint8_t)(currentColor.g * currentBrightness),
          (uint8_t)(currentColor.b * currentBrightness)
        );
        fill_solid(leds, NUM_LEDS, displayColor);
      } else {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
      }
      FastLED.show();
    }

    // Button handling
    bool btn = digitalRead(BUTTON_PIN);
    if (btn != lastButtonState && millis() - lastButtonTime > debounceDelay) {
      if (btn == HIGH) {
        bool wasOn = power->getVal();
        power->setVal(!wasOn);
        
        if (!wasOn) {
          float h = hue->getVal();
          int s = saturation->getVal();
          int v = brightness->getVal();
          float adj = (v <= 1) ? 0.02f : v / 100.0f;
          targetBrightness = adj;
          targetColor = hueToRGB(h, s, 100);
          Serial.println("Button: ON");
        } else {
          targetBrightness = 0.0f;
          Serial.println("Button: OFF");
        }
      }
      lastButtonTime = millis();
    }
    lastButtonState = btn;
  }
};

////////////////////////////////////////////
// Setup
////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== HomeSpan RGB Lamp v2.0 ===");

  // Uncomment to reset pairing
  // resetHomeSpanData();

  homeSpan.begin(Category::Lighting, "Smart RGB Lamp");
  homeSpan.setPairingCode(PAIRING_CODE);           
  Serial.println("Pairing Code: " PAIRING_CODE);   
  homeSpan.setQRID("Lamp");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Manufacturer("your_manufacturer");
      new Characteristic::SerialNumber("your_serial_number");
      new Characteristic::Model("SmartLamp RGB");
      new Characteristic::FirmwareRevision("2.0");

  new RGBLightbulb();

  Serial.println("\n=== Setup Complete ===");
  Serial.println("Ready for HomeKit pairing!");
}

void loop() {
  homeSpan.poll();
}