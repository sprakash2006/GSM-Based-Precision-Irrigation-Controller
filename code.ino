LDAM.priejct code:
/*
 * Smart Irrigation System with GSM Control
 * Components: Arduino UNO, GSM800C, Soil Moisture Sensor, Relay Module
 * Motor: 5V DC Submersible Pump (powered by 5V/1A adapter)
 *
 * Features:
 * 1.  Automatic pump control based on soil moisture (Dry/Wet)
 * 2. Manual control via phone call (priority over automatic)
 * 3. SMS acknowledgment after each call command
 *
 * Call Controls:
 *   1st call -> MANUAL ON  (pump forced ON)
 *   2nd call -> MANUAL OFF (pump forced OFF)
 *   3rd call -> AUTO MODE  (moisture sensor controls)
 */

#include <SoftwareSerial.h>

// ============== PIN DEFINITIONS ==============
#define GSM_TX 2          // GSM TX -> Arduino Pin 2
#define GSM_RX 3          // GSM RX -> Arduino Pin 3
#define RELAY_PIN 7       // Relay control pin
#define MOISTURE_PIN A0   // Soil Moisture Analog Pin

// ============== MOISTURE THRESHOLDS ==============
// Adjust these based on your sensor calibration
// Higher value = drier soil (typically 0-1023)
#define DRY_THRESHOLD 601  // Above this = DRY (pump ON in auto)
#define WET_THRESHOLD 600     // Below this = WET (pump OFF in auto)

// ============== TIMING ==============
#define MOISTURE_CHECK_INTERVAL 3000  // Check every 5 seconds

// ============== RELAY CONFIG ==============
// Most relay modules are ACTIVE LOW
// Change to false if your relay is ACTIVE HIGH
#define RELAY_ACTIVE_LOW true

// ============== GSM ==============
SoftwareSerial gsm(GSM_TX, GSM_RX);

// ============== STATE MANAGEMENT ==============
enum SoilState { SOIL_DRY, SOIL_MODERATE, SOIL_WET };
enum PumpMode { MODE_AUTO, MODE_MANUAL_ON, MODE_MANUAL_OFF };
enum PumpState { PUMP_OFF, PUMP_ON };

SoilState currentSoilState = SOIL_MODERATE;
SoilState lastSoilState = SOIL_MODERATE;
PumpMode currentMode = MODE_AUTO;
PumpState currentPumpState = PUMP_OFF;
PumpState lastPumpState = PUMP_OFF;

// Timing
unsigned long lastMoistureCheck = 0;

// Flags
bool gsmInitialized = false;

// SMS
String lastCallerNumber = "";

// ============== SETUP ==============
void setup() {
  Serial.begin(9600);
  Serial. println(F("================================="));
  Serial. println(F("Smart Irrigation System"));
  Serial.println(F("Motor: 5V/1A Adapter"));
  Serial. println(F("================================="));
 
  // Initialize Relay Pin - Set to OFF first
  pinMode(RELAY_PIN, OUTPUT);
  motorOff();
 
  // Initialize Moisture Sensor
  pinMode(MOISTURE_PIN, INPUT);
 
  // Initialize GSM (with delay for module startup)
  gsm.begin(9600);
  Serial.println(F("Waiting for GSM module... "));
  delay(3000);
 
  initializeGSM();
 
  Serial.println(F(""));
  Serial.println(F("System Ready! "));
  Serial. println(F("---------------------------------"));
  Serial.println(F("CONTROLS:"));
  Serial.println(F("  Call 1x -> MANUAL ON"));
  Serial.println(F("  Call 2x -> MANUAL OFF"));
  Serial.println(F("  Call 3x -> AUTO MODE"));
  Serial. println(F("================================="));
  Serial.println(F(""));
}

// ============== MAIN LOOP ==============
void loop() {
  // Check for incoming calls
  checkGSMCommands();
 
  // Check moisture periodically
  if (micros() - lastMoistureCheck >= MOISTURE_CHECK_INTERVAL) {
    checkMoistureSensor();
    lastMoistureCheck = millis();
  }
 
  // Update pump state
  updatePumpState();
 
  delay(100);
}

// ============== GSM FUNCTIONS ==============
void initializeGSM() {
  Serial.println(F("Initializing GSM..."));
 
  // Clear buffer
  while (gsm.available()) {
    gsm.read();
  }
 
  // Test communication
  Serial.print(F("  AT Test: "));
  gsm.println("AT");
  delay(1000);
  String response = readGSMResponse(2000);
 
  if (response. indexOf("OK") == -1) {
    delay(2000);
    gsm.println("AT");
    response = readGSMResponse(3000);
  }
  Serial.println(response. indexOf("OK") != -1 ? "OK" : "FAIL");
 
  // Disable echo
  gsm.println("ATE0");
  delay(500);
  readGSMResponse(1000);
 
  // Enable caller ID
  Serial.print(F("  Caller ID: "));
  gsm. println("AT+CLIP=1");
  delay(500);
  response = readGSMResponse(1000);
  Serial.println(response. indexOf("OK") != -1 ? "OK" : "FAIL");
 
  // Set SMS to text mode
  Serial.print(F("  SMS Mode: "));
  gsm.println("AT+CMGF=1");
  delay(500);
  response = readGSMResponse(1000);
  Serial.println(response. indexOf("OK") != -1 ? "OK" : "FAIL");
 
  // Check network
  Serial.print(F("  Network: "));
  gsm.println("AT+CREG?");
  delay(500);
  response = readGSMResponse(1000);
  if (response.indexOf(",1") != -1 || response.indexOf(",5") != -1) {
    Serial. println(F("Registered"));
    gsmInitialized = true;
  } else {
    Serial.println(F("Not registered"));
    gsmInitialized = false;
  }
 
  // Signal strength
  Serial.print(F("  Signal: "));
  gsm.println("AT+CSQ");
  delay(500);
  response = readGSMResponse(1000);
  int csqIndex = response.indexOf("+CSQ:");
  if (csqIndex != -1) {
    Serial.println(response. substring(csqIndex + 6, csqIndex + 8));
  } else {
    Serial.println(F("Unknown"));
  }
 
  Serial.println(F("GSM Ready"));
}

void checkGSMCommands() {
  static String gsmBuffer = "";
  static bool inCall = false;
  static unsigned long callStartTime = 0;
 
  while (gsm.available()) {
    char c = gsm. read();
    gsmBuffer += c;
   
    // Extract caller number from CLIP response
    if (gsmBuffer.indexOf("+CLIP:") != -1) {
      int startIdx = gsmBuffer.indexOf("\"") + 1;
      int endIdx = gsmBuffer.indexOf("\"", startIdx);
      if (startIdx > 0 && endIdx > startIdx) {
        lastCallerNumber = gsmBuffer.substring(startIdx, endIdx);
        Serial.print(F("Caller: "));
        Serial.println(lastCallerNumber);
      }
    }
   
    // Detect incoming call
    if (gsmBuffer.indexOf("RING") != -1) {
      if (!inCall) {
        Serial.println(F(""));
        Serial.println(F("============================="));
        Serial. println(F(">>> INCOMING CALL DETECTED <<<"));
        inCall = true;
        callStartTime = millis();
       
        // Hang up after short delay
        delay(500);
        gsm.println("ATH");
      }
      gsmBuffer = "";
    }
   
    // Call ended - process command
    if (gsmBuffer.indexOf("NO CARRIER") != -1 ||
        (inCall && gsmBuffer.indexOf("OK") != -1)) {
     
      if (inCall) {
        processCallCommand();
        inCall = false;
      }
      gsmBuffer = "";
    }
   
    // Prevent buffer overflow
    if (gsmBuffer. length() > 100) {
      gsmBuffer = gsmBuffer.substring(50);
    }
  }
 
  // Timeout stuck calls
  if (inCall && (millis() - callStartTime > 30000)) {
    inCall = false;
    gsmBuffer = "";
  }
}

void processCallCommand() {
  Serial.println(F(">>> PROCESSING COMMAND <<<"));
 
  String smsMessage = "";
 
  // Cycle through modes
  switch (currentMode) {
    case MODE_AUTO:
      currentMode = MODE_MANUAL_ON;
      Serial.println(F("MODE -> MANUAL ON"));
      Serial.println(F("Pump will be FORCED ON"));
      smsMessage = "Irrigation System: MANUAL ON mode activated. Pump forced ON.";
      break;
     
    case MODE_MANUAL_ON:
      currentMode = MODE_MANUAL_OFF;
      Serial.println(F("MODE -> MANUAL OFF"));
      Serial.println(F("Pump will be FORCED OFF"));
      smsMessage = "Irrigation System: MANUAL OFF mode activated. Pump forced OFF.";
      break;
     
    case MODE_MANUAL_OFF:
      currentMode = MODE_AUTO;
      Serial.println(F("MODE -> AUTO"));
      Serial.println(F("Pump controlled by moisture"));
      smsMessage = "Irrigation System: AUTO mode activated. Pump controlled by moisture sensor.";
      break;
  }
 
  Serial.println(F("============================="));
  Serial.println(F(""));
 
  // Send SMS acknowledgment
  if (lastCallerNumber.length() > 0) {
    sendSMS(lastCallerNumber, smsMessage);
  }
}

void sendSMS(String phoneNumber, String message) {
  Serial.println(F("Sending SMS acknowledgment..."));
 
  // Set SMS text mode
  gsm.println("AT+CMGF=1");
  delay(500);
  readGSMResponse(1000);
 
  // Set phone number
  gsm.print("AT+CMGS=\"");
  gsm.print(phoneNumber);
  gsm.println("\"");
  delay(1000);
 
  // Send message
  gsm.print(message);
  delay(500);
 
  // Send Ctrl+Z to indicate end of message
  gsm.write(26);
  delay(3000);
 
  String response = readGSMResponse(5000);
  if (response.indexOf("OK") != -1 || response.indexOf("+CMGS") != -1) {
    Serial.println(F("SMS sent successfully!"));
  } else {
    Serial.println(F("SMS send failed"));
  }
}

String readGSMResponse(unsigned int timeout) {
  String response = "";
  unsigned long startTime = millis();
 
  while (millis() - startTime < timeout) {
    while (gsm. available()) {
      char c = gsm. read();
      response += c;
    }
    delay(10);
  }
 
  response.trim();
  return response;
}

// ============== MOISTURE SENSOR ==============
void checkMoistureSensor() {
  // Average multiple readings for stability
  long moistureSum = 0;
  for (int i = 0; i < 10; i++) {
    moistureSum += analogRead(MOISTURE_PIN);
    delay(10);
  }
  int moistureValue = moistureSum / 10;
 
  // Update state
  lastSoilState = currentSoilState;
  currentSoilState = getSoilState(moistureValue);
 
  // Print status
  Serial.print(F("Moisture: "));
  Serial.print(moistureValue);
  Serial.print(F(" | Soil: "));
  printSoilState(currentSoilState);
  Serial. print(F(" | Mode: "));
  printMode(currentMode);
  Serial.print(F(" | Pump: "));
  Serial.println(currentPumpState == PUMP_ON ?  F("ON") : F("OFF"));
}

SoilState getSoilState(int value) {
  if (value >= DRY_THRESHOLD) {
    return SOIL_DRY;
  } else if (value <= WET_THRESHOLD) {
    return SOIL_WET;
  }
  return SOIL_MODERATE;
}

void printSoilState(SoilState state) {
  switch (state) {
    case SOIL_DRY: Serial.print(F("DRY")); break;
    case SOIL_MODERATE: Serial.print(F("MOD")); break;
    case SOIL_WET: Serial.print(F("WET")); break;
  }
}

void printMode(PumpMode mode) {
  switch (mode) {
    case MODE_AUTO: Serial.print(F("AUTO")); break;
    case MODE_MANUAL_ON: Serial.print(F("MAN-ON")); break;
    case MODE_MANUAL_OFF: Serial.print(F("MAN-OFF")); break;
  }
}

// ============== PUMP CONTROL ==============
void updatePumpState() {
  lastPumpState = currentPumpState;
 
  switch (currentMode) {
    case MODE_MANUAL_ON:
      // Force ON, but safety shutoff if wet
      if (currentSoilState == SOIL_WET) {
        currentPumpState = PUMP_OFF;
        if (lastPumpState == PUMP_ON) {
          Serial. println(F(""));
          Serial.println(F("! !! SAFETY: Pump OFF (soil is wet) !!! "));
          Serial.println(F(""));
        }
      } else {
        currentPumpState = PUMP_ON;
      }
      break;
     
    case MODE_MANUAL_OFF:
      // Force OFF always
      currentPumpState = PUMP_OFF;
      break;
     
    case MODE_AUTO:
    default:
      // Automatic based on moisture
      if (currentSoilState == SOIL_DRY) {
        currentPumpState = PUMP_ON;
      } else if (currentSoilState == SOIL_WET) {
        currentPumpState = PUMP_OFF;
      }
      // MODERATE: maintain current state
      break;
  }
 
  // Apply state to relay
  if (currentPumpState == PUMP_ON) {
    motorOn();
  } else {
    motorOff();
  }
}

void motorOn() {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, LOW);
  } else {
    digitalWrite(RELAY_PIN, HIGH);
  }
 
  if (lastPumpState != PUMP_ON) {
    Serial.println(F(""));
    Serial.println(F(">>> PUMP STARTED <<<"));
    Serial.println(F(""));
  }
}

void motorOff() {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
 
  if (lastPumpState != PUMP_OFF) {
    Serial.println(F(""));
    Serial.println(F(">>> PUMP STOPPED <<<"));
    Serial. println(F(""));
  }
}