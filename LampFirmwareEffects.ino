#include "HomeSpan.h"
#include <FastLED.h>

#define PAIRING_CODE "XXXXXXXX"   

// Pin definitions
#define LED_PIN 9
#define BUTTON_PIN 4
#define NUM_LEDS 8

// LED array
CRGB leds[NUM_LEDS];

// Effect modes enumeration
enum EffectMode {
  EFFECT_NONE = 0,
  EFFECT_COLORFUL_TWINKLE = 1,
  EFFECT_AURORA = 2,
  EFFECT_TWINKLE = 3
};

////////////////////////////////////////////
// Reset HomeSpan stored data
////////////////////////////////////////////
void resetHomeSpanData() {
  Serial.println("Resetting HomeSpan stored data...");
  homeSpan.deleteStoredValues();
  Serial.println("HomeSpan data reset complete!");
}

////////////////////////////////////////////
// RGB Lightbulb Service with Effects
////////////////////////////////////////////
struct RGBLightbulb : Service::LightBulb {
  SpanCharacteristic *power;
  SpanCharacteristic *brightness;
  SpanCharacteristic *hue;
  SpanCharacteristic *saturation;

  bool lastButtonState = LOW;
  unsigned long lastButtonTime = 0;
  const unsigned long debounceDelay = 50;

  // Smooth brightness transition system
  float currentBrightness = 0.0f;
  float targetBrightness = 0.0f;

  // Base colors for solid mode
  CRGB currentBaseColor;
  CRGB targetBaseColor;

  // Effect tracking
  EffectMode currentEffect = EFFECT_NONE;
  EffectMode targetEffect = EFFECT_NONE;

  // Aurora effect variables
  uint16_t auroraPhase = 0;
  uint16_t auroraPhase2 = 0;

  // Timing for smooth transitions
  unsigned long lastUpdateTime = 0;
  const unsigned long updateInterval = 20;
  const float fadeSpeed = 0.07f;
  const float twinkleBrightness = 0.6f;

  // Rendering timer
  unsigned long lastRenderTime = 0;

  // Gamma correction
  uint8_t gammaCorrect(uint8_t value) {
    return dim8_video(value);
  }

  // Get render interval based on effect
  unsigned long getEffectInterval(EffectMode mode) {
    switch (mode) {
      case EFFECT_COLORFUL_TWINKLE: return 30;
      case EFFECT_AURORA:           return 35;
      case EFFECT_TWINKLE:          return 50;
      default:                      return 50;
    }
  }

  // Detect effect from HSV
  EffectMode detectEffect(float h, int s, int v) {
    if (v < 1) return EFFECT_NONE;
    if (abs(h - 352.0f) <= 1.0f && abs(s - 3) <= 1)   return EFFECT_COLORFUL_TWINKLE;
    if (abs(h - 9.0f)   <= 1.0f && abs(s - 5) <= 1)   return EFFECT_AURORA;
    if (abs(h - 17.0f)  <= 1.0f && abs(s - 7) <= 1)   return EFFECT_TWINKLE;
    return EFFECT_NONE;
  }

  // Convert HSV to RGB (V=100% for base)
  CRGB hueToRGB(float h, int s) {
    float hNorm = h / 360.0f;
    float sNorm = s / 100.0f;
    float vNorm = 1.0f;
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

  // Apply brightness
  CRGB applyBrightness(CRGB color, float brightness) {
    return CRGB(
      (uint8_t)(color.r * brightness),
      (uint8_t)(color.g * brightness),
      (uint8_t)(color.b * brightness)
    );
  }

  // Effects
  void renderColorfulTwinkle() {
    uint8_t fadeAmount = map((uint8_t)(currentBrightness * 255), 0, 255, 8, 1);
    fadeToBlackBy(leds, NUM_LEDS, fadeAmount);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (random8() < 3 && leds[i].getLuma() < (uint8_t)(100 * currentBrightness + 1)) {
        uint8_t minB = (uint8_t)(80 * currentBrightness);
        uint8_t maxB = (uint8_t)(255 * currentBrightness);
        leds[i] += CHSV(random8(), 180, random8(minB, maxB + 1));
      }
    }
  }

  void renderAurora() {
    for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t b1 = sin8(auroraPhase + i * 20);
      uint8_t b2 = sin8(auroraPhase2 + i * 35);
      uint8_t bright = (b1 + b2) / 2;
      uint8_t hue = map(sin8(auroraPhase / 4 + i * 10), 0, 255, 90, 140);
      leds[i] = applyBrightness(CHSV(hue, 200, bright), currentBrightness);
    }
    auroraPhase += 3;
    auroraPhase2 += 5;
  }

  void renderTwinkle() {
    uint8_t fadeAmount = map((uint8_t)(currentBrightness * 255), 0, 255, 8, 1);
    fadeToBlackBy(leds, NUM_LEDS, fadeAmount);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (random8() < 3 && leds[i].getLuma() < (uint8_t)(100 * currentBrightness + 1)) {
        uint8_t minB = (uint8_t)(80 * currentBrightness);
        uint8_t maxB = (uint8_t)(255 * currentBrightness);
        leds[i] += CHSV(random8(20, 40), 180, random8(minB, maxB + 1));
      }
    }
  }

  // Smooth transitions
  void updateTransitions() {
    if (millis() - lastUpdateTime < updateInterval) return;
    lastUpdateTime = millis();

    if (abs(currentBrightness - targetBrightness) > 0.001f) {
      float diff = targetBrightness - currentBrightness;
      currentBrightness += diff * fadeSpeed;
      if (abs(diff) < 0.01f) currentBrightness = targetBrightness;
      currentBrightness = constrain(currentBrightness, 0.0f, 1.0f);
    }

    if (currentEffect == EFFECT_NONE && currentBaseColor != targetBaseColor) {
      currentBaseColor = blend(currentBaseColor, targetBaseColor, (uint8_t)(fadeSpeed * 255.0f));
    }
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

    currentBaseColor = CRGB::Black;
    targetBaseColor = CRGB::White;
    currentBrightness = targetBrightness = 0.0f;

    FastLED.clear();
    FastLED.show();

    Serial.println("RGB Lightbulb initialized with effects");
    Serial.println("Effect triggers:");
    Serial.println("  H=352, S=3%  -> Colorful Twinkle");
    Serial.println("  H=9,   S=5%  -> Aurora");
    Serial.println("  H=17,  S=7%  -> Twinkle");
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
      targetEffect = detectEffect(newHue, newSaturation, newBrightness);

      if (targetEffect != EFFECT_NONE) {
        const char* names[] = {"None", "Colorful Twinkle", "Aurora", "Twinkle"};
        Serial.printf("Effect: %s (V=%d%%)\n", names[targetEffect], newBrightness);
        currentEffect = targetEffect;
        if (targetEffect == EFFECT_COLORFUL_TWINKLE || targetEffect == EFFECT_TWINKLE) {
          targetBrightness = twinkleBrightness;
        }
      } else {
        if (currentEffect != EFFECT_NONE) {
          Serial.println("Switching to solid color");
          currentBaseColor = CRGB::Black;
        }
        currentEffect = EFFECT_NONE;
        targetBaseColor = hueToRGB(newHue, newSaturation);
      }
    } else {
      Serial.println("Turning OFF");
      targetBrightness = 0.0f;
    }
    return true;
  }

  void loop() {
    updateTransitions();

    if (currentBrightness <= 0.001f && currentEffect != EFFECT_NONE && targetBrightness == 0.0f) {
      currentEffect = EFFECT_NONE;
      Serial.println("Effect stopped");
    }

    unsigned long now = millis();
    if (currentBrightness > 0.001f) {
      bool fading = abs(currentBrightness - targetBrightness) > 0.001f ||
                    (currentEffect == EFFECT_NONE && currentBaseColor != targetBaseColor);
      unsigned long interval = (currentEffect != EFFECT_NONE) ? getEffectInterval(currentEffect) : 100;
      if (fading) interval = min(20UL, interval);

      if (now - lastRenderTime >= interval) {
        lastRenderTime = now;
        if (currentEffect != EFFECT_NONE) {
          switch (currentEffect) {
            case EFFECT_COLORFUL_TWINKLE: renderColorfulTwinkle(); break;
            case EFFECT_AURORA:           renderAurora();         break;
            case EFFECT_TWINKLE:          renderTwinkle();        break;
          }
        } else {
          fill_solid(leds, NUM_LEDS, applyBrightness(currentBaseColor, currentBrightness));
        }
        FastLED.show();
      }
    } else if (now - lastRenderTime >= 100) {
      lastRenderTime = now;
      FastLED.clear();
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
          targetEffect = detectEffect(h, s, v);
          if (targetEffect != EFFECT_NONE) {
            currentEffect = targetEffect;
            if (targetEffect == EFFECT_COLORFUL_TWINKLE || targetEffect == EFFECT_TWINKLE) {
              targetBrightness = twinkleBrightness;
            }
            Serial.println("Button: Effect ON");
          } else {
            currentEffect = EFFECT_NONE;
            targetBaseColor = hueToRGB(h, s);
            Serial.println("Button: Color ON");
          }
        } else {
          targetBrightness = 0.0f;
          Serial.println("Button: OFF");
        }
        Serial.printf("Button: Light %s\n", !wasOn ? "ON" : "OFF");
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
  Serial.println("\n=== HomeSpan RGB Lamp v3.0 ===");

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
      new Characteristic::Model("SmartLamp RGB Effects");
      new Characteristic::FirmwareRevision("3.0");

  new RGBLightbulb();

  Serial.println("\n=== Setup Complete ===");
  Serial.println("Ready for HomeKit pairing!");
  Serial.println("Enter code in Home app");
}

void loop() {
  homeSpan.poll();
}