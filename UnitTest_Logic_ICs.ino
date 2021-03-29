// Which type of chip to test, with leading 0 removed (the 0 would indicate an octal number)
#define CHIP 0
// Whether to continue testing if one set of inputs has the wrong output
#define STOP_AFTER_FAIL true

// On all chips, connect wires from top to bottom on the left, and top to bottom
// on the right (on DIP-14 chips that means 1-6, 13-8, on DIP-16, 1-7, 15-9).
// On the Arduino, those connect to pins 2-13, A0-A1.
// And of course, connect bottom left of the chip to GND, and top right to 5V.

// Chip -> Arduino pin mappings
const int DIP_14[] = {
  -1, // Not used; chip pins start at 1
  2, 3, 4, 5, 6, 7, // Chip pins 1-6, the left side
  -1, // Not used; pin 7 is GND
  13, 12, 11, 10, 9, 8 // Chip pins 8-13 (14 is VCC)
};

const int DIP_16[] = {
  -1, // Not used; chip pins start at 1
  2, 3, 4, 5, 6, 7, 8, // Chip pins 1-7, the left side
  -1, // Not used; pin 8 is GND
  A1, A0, 13, 12, 11, 10, 9 // Chip pins 9-15 (16 is VCC)
};

const int DIP_20[] = {
  -1, // Not used; chip pins start at 1
  2, 3, 4, 5, 6, 7, 8, 9, 10, // Chip pins 1-9, the left side
  -1, // Not used; pin 10 is GND
  A5, A4, A3, A2, A1, A0, 13, 12, 11, // Chip pins 1-19 (20 is VCC)
};

// Input / output (in terms of the Arduino) pins for each chip,
// defined using chip pin numbers, and mapped to Arduino pins in setup().

// DIP-14 stateless chips
#if CHIP == 30
  int outputPins[] = {1, 2, 3, 4, 5, 6, 12, 11};
  int inputPins[] = {8};
#elif CHIP == 2
  // For some reason the NOR pinout is different than the rest of the quad gate chips
  int outputPins[] = {2, 3, 5, 6, 12, 11, 9, 8};
  int inputPins[] = {1, 4, 13, 10};
#elif CHIP == 0 || CHIP == 8 || CHIP == 32 || CHIP == 86
  int outputPins[] = {1, 2, 4, 5, 13, 12, 10, 9};
  int inputPins[] = {3, 6, 11, 8};
#elif CHIP == 4
  int outputPins[] = {1, 3, 5, 13, 11, 9};
  int inputPins[] = {2, 4, 6, 12, 10, 8};

// DIP-16 stateless chips
#elif CHIP == 139
  // Pins are out of order to make logic simpler below
  int outputPins[] = {1, 3, 2, 15, 13, 14};
  int inputPins[] = {7, 6, 5, 4, 9, 10, 11, 12};
#elif CHIP == 157
  int outputPins[] = {15, 1, 2, 5, 14, 11, 3, 6, 13, 10};
  int inputPins[] = {4, 7, 12, 9};
#elif CHIP == 283
  // A4-1, B4-1, C0
  int outputPins[] = {12, 14, 3, 5, 11, 15, 2, 6, 7};
  // C4, E4-1
  int inputPins[] = {9, 10, 13, 1, 4};

// DIP-16 stateful chips
#elif CHIP == 161
  int outputPins[] = {1, 2, 3, 4, 5, 6, 7, 10, 9};
  int inputPins[] = {15, 11, 12, 13, 14};
#elif CHIP == 173
  int outputPins[] = {1, 2, 9, 10, 7, 15, 14, 13, 12, 11};
  int inputPins[] = {3, 4, 5, 6};

// DIP-20 stateful chips
#elif CHIP == 245
  // Not technically stateful, but requires careful ordering of operations to not short pins
  int outputPins[] = {1, 19};
  int inputPins[] = {2, 3, 4, 5, 6, 7, 8, 9, 18, 17, 16, 15, 14, 13, 12, 11};
#elif CHIP == 273
  int outputPins[] = {3, 4, 7, 8, 13, 14, 17, 18, 1, 11};
  int inputPins[] = {2, 5, 6, 9, 12, 15, 16, 19};

#else
  #error "Invalid CHIP"
#endif

const int outputCount = sizeof(outputPins) / sizeof(*outputPins);
const int inputCount = sizeof(inputPins) / sizeof(*inputPins);

void setup() {
  Serial.begin(57600);
  
  bool invalidConfig = false;
  const int *mapping;
  if (outputCount + inputCount > 18) {
    invalidConfig = true;
  } else if (outputCount + inputCount > 14) {
    mapping = DIP_20;
  } else if (outputCount + inputCount > 12) {
    mapping = DIP_16;
  } else {
    mapping = DIP_14;
  }

  // Use the mapping for the type of chip to convert chip pins to Arduino pins
  for (int i = 0; !invalidConfig && i < outputCount; i++) {
    outputPins[i] = mapping[outputPins[i]];
  }
  for (int i = 0; !invalidConfig && i < inputCount; i++) {
    inputPins[i] = mapping[inputPins[i]];
  }

  if (invalidConfig) {
    Serial.println("Invalid config!");
    while (true) {}
  }
}

void loop() {
  for (int i = 0; i < outputCount; i++) {
    pinMode(outputPins[i], OUTPUT);
  }
  for (int i = 0; i < inputCount; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
  }
  
  #if CHIP == 161
    bool passed = test161();
  #elif CHIP == 173
    bool passed = test173();
  #elif CHIP == 245
    bool passed = test245();
  #elif CHIP == 273
    bool passed = test273();
  #else
    bool passed = testStateless();
  #endif

  // Protect against prolonged shorts, if it's the wrong chip, in backwards, etc.
  for (int i = 0; i < outputCount; i++) {
    pinMode(outputPins[i], INPUT);
  }

  if (passed) {
    Serial.println("All passed");
  }
  
  delay(3000);
}

bool testStateless() {
  bool passed = true;
  for (int value = 0; value < (1 << outputCount); value++) {
    setOutputsToValue(value);
    int result = getInputValues();
    int expected = getExpected(value);
    if (result != expected) {
      char buf[128];
      sprintf(buf, "0x%02X should be 0x%02X but was 0x%02X", value, expected, result);
      Serial.println(buf);
      passed = false;
      #if STOP_AFTER_FAIL
        break;
      #endif
    }
  }
  return passed;
}

int getInputValues() {
  int result = 0;
  for (int i = 0; i < inputCount; i++) {
    result = (result << 1) | (digitalRead(inputPins[i]) ? 1 : 0);
  }
  return result;
}

void setOutputsToValue(int value) {
  for (int i = 0; i < outputCount; i++) {
    digitalWrite(outputPins[i], value >> (outputCount - 1 - i) & 1);
  }
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
    int a = (value >> 4) & 0b1111;
    int b = value & 0b1111;
    int result = ab ? b : a;
    
    return g ? 0 : result;

  // 4-bit adder
  #elif CHIP == 283
    int a = value >> 5;
    int b = (value >> 1) & 0b1111;
    int c = value & 1;
    return a + b + c;
  
  #else
    return 0;
  #endif
}

bool test161() {
  bool passed = true;
  char buf[128];
  
  const int MR = outputPins[0]; // Master Reset
  const int CP = outputPins[1]; // Clock Pulse
  const int P[] = {outputPins[2], outputPins[3], outputPins[4], outputPins[5]}; // Parallel data
  const int CEP = outputPins[6]; // Count enable
  const int CET = outputPins[7]; // Count enable
  const int PE = outputPins[8]; // Parallel enable

  digitalWrite(MR, LOW);
  digitalWrite(MR, HIGH);

  digitalWrite(CP, LOW);
  digitalWrite(PE, HIGH);
  for (int i = 0; i < 16; i++) {
    for (int en = 0; en < 4; en++) {
      digitalWrite(CEP, en & 2);
      digitalWrite(CET, en & 1);
      digitalWrite(CP, HIGH);
      digitalWrite(CP, LOW);
    }
    bool carry = i + 1 == 15;
    int result = getInputValues();
    int expected = (i + 1) % 16 + (carry ? 16 : 0);
    if (result != expected) {
      sprintf(buf, "Counting, expected 0x%02X but got 0x%02X", expected, result);
      Serial.println(buf);
      passed = false;
      #if STOP_AFTER_FAIL
        return passed;
      #endif
    }
  }

  digitalWrite(PE, LOW);
  for (int i = 0; i < 16; i++) {
    for (int p = 3; p >= 0; p--) {
      digitalWrite(P[p], (i >> p) & 1);
    }
    digitalWrite(CP, HIGH);
    digitalWrite(CP, LOW);
    bool carry = i == 15;
    int result = getInputValues();
    int expected = i + (carry ? 16 : 0);
    if (result != expected) {
      sprintf(buf, "Preset loading, expected 0x%02X but got 0x%02X", expected, result);
      Serial.println(buf);
      passed = false;
      #if STOP_AFTER_FAIL
        return passed;
      #endif
    }

    digitalWrite(MR, LOW);
    digitalWrite(MR, HIGH);
    result = getInputValues();
    if (result != 0) {
      sprintf(buf, "Resetting, expected 0 but got 0x%02X", expected, result);
      Serial.println(buf);
      passed = false;
      #if STOP_AFTER_FAIL
        return passed;
      #endif
    }
  }
  
  return passed;
}

bool test173() {
  bool passed = true;
  char buf[128];
  
  const int M = outputPins[0]; // Enable 1
  const int N = outputPins[1]; // Enable 2
  const int G1 = outputPins[2]; // Data enable 1
  const int G2 = outputPins[3]; // Data enable 2
  const int CLK = outputPins[4]; // Parallel enable
  const int CLR = outputPins[5]; // Clear
  const int D[] = {outputPins[6], outputPins[7], outputPins[8], outputPins[9]}; // Data inputs
  const int F[] = {M, N, G1, G2}; // Function inputs

  digitalWrite(CLR, HIGH);
  digitalWrite(CLR, LOW);
  int state = 0;

  digitalWrite(CLK, LOW);
  for (int value = 0; value < 16; value++) {
    for (int f = 0; f < 16; f++) {
      for (int i = 0; i < 4; i++) {
        digitalWrite(D[i], (1 << (3 - i)) & value);
        digitalWrite(F[i], (1 << (3 - i)) & f);
      }
      digitalWrite(CLK, HIGH);
      digitalWrite(CLK, LOW);

      int result = getInputValues();
      bool enabled = (f & 0b1100) == 0;
      bool dataEnabled = (f & 0b11) == 0;
      if (dataEnabled) {
        state = value;
      }
      int expected = enabled ? state : 0xF;

      if (result != expected) {
        sprintf(buf, "With functions 0x%02X, expected 0x%02X but got 0x%02X", f, expected, result);
        Serial.println(buf);
        passed = false;
        #if STOP_AFTER_FAIL
          return passed;
        #endif
      }
  
      digitalWrite(CLR, HIGH);
      digitalWrite(CLR, LOW);
      state = 0;
      digitalWrite(M, LOW);
      digitalWrite(N, LOW);
      result = getInputValues();
      if (result != 0) {
        sprintf(buf, "Resetting, expected 0 but got 0x%02X", result);
        Serial.println(buf);
        passed = false;
        #if STOP_AFTER_FAIL
          return passed;
        #endif
      }
    }
  }
  return passed;
}

bool test245() {
  bool passed = true;
  char buf[128];
  
  const int DIR = outputPins[0]; // Direction
  const int OE = outputPins[1]; // Output enable
  for (int value = 0; value < 256; value++) {
    for (int f = 0; f < 4; f++) {
      // Reset all A / B pins to inputs before changing direction and OE
      for (int i = 0; i < inputCount; i++) {
        pinMode(inputPins[i], INPUT_PULLUP);
      }
    
      bool dir = f & 1;
      bool oe = f & 2;
      digitalWrite(DIR, dir);
      digitalWrite(OE, oe);
      for (int i = 0; i < 8; i++) {
        int pin = inputPins[i + (dir ? 0 : 8)];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, value & (1 << (7 - i)));
      }
      int result = 0;
      for (int i = 0; i < 8; i++) {
        int pin = inputPins[i + (dir ? 8 : 0)];
        result = result | (digitalRead(pin) << (7 - i));
      }
      int expected = oe ? 0xFF : value;

      if (result != expected) {
        sprintf(buf, "With functions 0x%02X, expected 0x%02X but got 0x%02X", f, expected, result);
        Serial.println(buf);
        passed = false;
        #if STOP_AFTER_FAIL
          return passed;
        #endif
      }
    }
  }
  
  return passed;
}

bool test273() {
  bool passed = true;
  char buf[128];
  
  const int MR = outputPins[8]; // Master Reset
  const int CP = outputPins[9]; // Clock Pulse

  digitalWrite(MR, LOW);
  digitalWrite(MR, HIGH);
  for (int value = 0; value < 256; value++) {
    for (int i = 0; i < 8; i++) {
      digitalWrite(outputPins[i], value & (1 << (7 - i)));
    }
    digitalWrite(CP, HIGH);
    digitalWrite(CP, LOW);
    
    int result = getInputValues();
    if (result != value) {
      sprintf(buf, "Set 0x%02X but got 0x%02X", value, result);
      Serial.println(buf);
      passed = false;
      #if STOP_AFTER_FAIL
        return passed;
      #endif
    }

    digitalWrite(MR, LOW);
    digitalWrite(MR, HIGH);
    result = getInputValues();
    if (result != 0) {
      sprintf(buf, "Resetting, expected 0 but got 0x%02X", result);
      Serial.println(buf);
      passed = false;
      #if STOP_AFTER_FAIL
        return passed;
      #endif
    }
  }
  return passed;
}
