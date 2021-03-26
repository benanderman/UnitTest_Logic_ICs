// Which type of chip to test, with leading 0 removed (the 0 would indicate an octal number)
#define CHIP 00
// Whether to continue testing if one set of inputs has the wrong output
#define STOP_AFTER_FAIL true

// On all chips, Arduino pins 2-7 should be connected to the chip's pins 1-6,
// and Arduino pins 8-13 to chip pins 13-8. The second set is in reverse order,
// because the gate input/output order is reversed on the right side of the chip.
// And of course, connect chip pin 7 to GND, and chip pin 14 to 5V.

#if CHIP == 30
  const int outputPins[] = {2, 3, 4, 5, 6, 7, 9, 10};
  const int inputPins[] = {13};
#elif CHIP == 2
  // For some reason the NOR pinout is different than the rest of the quad gate chips
  const int outputPins[] = {3, 4, 6, 7, 9, 10, 12, 13};
  const int inputPins[] = {2, 5, 8, 11};
#elif CHIP == 0 || CHIP == 8 || CHIP == 32 || CHIP == 86
  const int outputPins[] = {2, 3, 5, 6, 8, 9, 11, 12};
  const int inputPins[] = {4, 7, 10, 13};
#elif CHIP == 4
  const int outputPins[] = {2, 4, 6, 8, 10, 12};
  const int inputPins[] = {3, 5, 7, 9, 11, 13};
#else
  #error "Invalid CHIP"
#endif

const int outputCount = sizeof(outputPins) / sizeof(*outputPins);
const int inputCount = sizeof(inputPins) / sizeof(*inputPins);

void setup() {
  Serial.begin(57600);
}

void loop() {
  for (int i = 0; i < outputCount; i++) {
    pinMode(outputPins[i], OUTPUT);
  }
  for (int i = 0; i < inputCount; i++) {
    pinMode(inputPins[i], INPUT);
  }
  
  bool allPassed = true;
  for (int value = 0; value < (1 << outputCount); value++) {
    for (int i = 0; i < outputCount; i++) {
      digitalWrite(outputPins[i], value >> (outputCount - 1 - i) & 1);
    }
    int result = 0;
    for (int i = 0; i < inputCount; i++) {
      result = (result << 1) | (digitalRead(inputPins[i]) ? 1 : 0);
    }
    int expected = getExpected(value);
    if (result != expected) {
      char buf[128];
      sprintf(buf, "0x%02X should be 0x%02X but was 0x%02X", value, expected, result);
      Serial.println(buf);
      allPassed = false;
      #if STOP_AFTER_FAIL
        break;
      #endif
    }
  }
  if (allPassed) {
    Serial.println("All passed");
  }

  // Protect against prolonged shorts, if it's the wrong chip, in backwards, etc.
  for (int i = 0; i < outputCount; i++) {
    pinMode(outputPins[i], INPUT);
  }
  delay(3000);
}

int getExpected(int value) {
  // Single gate chip (8-input AND)
  #if CHIP == 30
    return (value == 255) ? 1 : 0;

  // Quad gate chips
  #elif CHIP == 0 || CHIP == 2 || CHIP == 8 || CHIP == 32 || CHIP == 86
    int result = 0;
    for (int i = 3; i >= 0; i--) {
      bool v1 = value & (1 << (i * 2));
      bool v2 = value & (1 << (i * 2 + 1));
      #if CHIP == 0
        bool gateResult = !(v1 && v2);
      #elif CHIP == 2
        bool gateResult = !(v1 || v2);
      #elif CHIP == 8
        bool gateResult = v1 && v2;
      #elif CHIP == 32
        bool gateResult = v1 || v2;
      #elif CHIP == 86
        bool gateResult = v1 ^ v2;
      #endif
      result = (result << 1) | (gateResult ? 1 : 0);
    }
    return result;

  // Hex inverter gates
  #elif CHIP == 4
    return ~value & 0b111111;
    
  #else
    #error "Invalid CHIP"
  #endif
}
