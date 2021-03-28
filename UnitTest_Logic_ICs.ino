// Which type of chip to test, with leading 0 removed (the 0 would indicate an octal number)
#define CHIP 0
// Whether to continue testing if one set of inputs has the wrong output
#define STOP_AFTER_FAIL true

// On all chips, connect wires from top to bottom on the left, and top to bottom
// on the right (on DIP-14 chips that means 1-6, 13-8, on DIP-16, 1-7, 15-9).
// On the Arduino, those connect to pins 2-13, A0-A1.
// And of course, connect bottom left of the chip to GND, and top right to 5V.

// DIP-14 chips
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

// DIP-16 chips
#elif CHIP == 139
  // Pins are out of order to make logic simpler below
  const int outputPins[] = {2, 4, 3, 9, 11, 10};
  const int inputPins[] = {8, 7, 6, 5, A1, A0, 13, 12};
#elif CHIP == 157
  const int outputPins[] = {9, 2, 3, 4, 6, 7, 10, 11, 13, A0};
  const int inputPins[] = {5, 8, 12, A1};

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
  // Quad gate chips
  #if CHIP == 0 || CHIP == 2 || CHIP == 8 || CHIP == 32 || CHIP == 86
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
    
  // Single gate chip (8-input AND)
  #elif CHIP == 30
    return (value == 255) ? 1 : 0;

  // Dual decoder
  #elif CHIP == 139
  int v1 = (value >> 3) & 0b11;
  int v2 = value & 0b11;
  bool en1 = value & 0b100000;
  bool en2 = value & 0b000100;
  // Enable pins are inverted
  int r1 = en1 ? 0 : 1 << v1;
  int r2 = en2 ? 0 : 1 << v2;
  // Outputs are inverted, but we don't want to invert the rest of the int's bits
  return ~((r1 << 4) | r2) & 0xFF;

  // Quad 2-line to 1-line data selectors / multiplexers
  #elif CHIP == 157
  bool g = value & (1 << 9);
  bool ab = value & (1 << 8);
  bool v1 = value & (1 << (7 - ab));
  bool v2 = value & (1 << (5 - ab));
  bool v3 = value & (1 << (3 - ab));
  bool v4 = value & (1 << (1 - ab));
  return g ? 0 : (v1 << 3) | (v2 << 2) | (v3 << 1) | v4;
  
  #else
    #error "Invalid CHIP"
  #endif
}
