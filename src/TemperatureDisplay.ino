#include <ESP32Servo.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Servo setup
Servo myServo;
const int servoPin = 18;

// Button pins
const int upButtonPin = 4;
const int downButtonPin = 5;
const int calibrateButtonPin = 19;  

// --- OLED Display ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- BME280 Sensor ---
Adafruit_BME280 bme;

// States
bool calibrationMode = false;
bool calibratingDown = true;  
bool isMoving = false;
bool movingUp = false;
float blindPosition = 0.0;  // 0 to 100

// Timing
unsigned long upTime = 2000;    
unsigned long downTime = 2000;
unsigned long movementStartTime = 0;

// EEPROM Addresses
const int EEPROM_UPTIME_ADDR = 0;
const int EEPROM_DOWNTIME_ADDR = 4; 

// Wi-Fi credentials
const char* ssid = "BenWIFI";
const char* password = "BEN12345678";

// HTTP Server
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 1000, 2000);

  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(calibrateButtonPin, INPUT_PULLUP);

  EEPROM.begin(64);
  EEPROM.get(EEPROM_UPTIME_ADDR, upTime);
  EEPROM.get(EEPROM_DOWNTIME_ADDR, downTime);

  if (upTime == 0xFFFFFFFF || downTime == 0xFFFFFFFF) {
    upTime = 2000;
    downTime = 2000;
  }

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor!");
    while (true);
  }

    // Wi-Fi connection
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 10000; // 10 seconds

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed. Continuing without web server...");
  }


    // Serve temperature data
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    float temperature = bme.readTemperature();
    String tempJson = "{\"temperature\": " + String(temperature, 1) + "}";
    request->send(200, "application/json", tempJson);
  });

    // Simulate up button press
  server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!isMoving) {
      Serial.println("Simulated Up Button Pressed");
      myServo.write(180);
      isMoving = true;
      movingUp = true;
      movementStartTime = millis();
      request->send(200, "text/plain", "Moving Up...");
    } else {
      request->send(200, "text/plain", "Already moving");
    }
  });

  // Simulate down button press
  server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!isMoving) {
      Serial.println("Simulated Down Button Pressed");
      myServo.write(0);
      isMoving = true;
      movingUp = false;
      movementStartTime = millis();
      request->send(200, "text/plain", "Moving Down...");
    } else {
      request->send(200, "text/plain", "Already moving");
    }
  });
  // Start server
  server.begin();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Setup Complete");
  display.display();
  delay(1000);
}

void loop() {
  static unsigned long buttonPressStart = 0;
  static bool calibrateButtonHeld = false;

  bool upPressed = !digitalRead(upButtonPin);
  bool downPressed = !digitalRead(downButtonPin);
  bool calibratePressed = !digitalRead(calibrateButtonPin);

  float temperature = bme.readTemperature();

  if (calibratePressed && !calibrateButtonHeld) {
    buttonPressStart = millis();
    calibrateButtonHeld = true;
  }

  if (!calibratePressed && calibrateButtonHeld) {
    unsigned long pressDuration = millis() - buttonPressStart;
    calibrateButtonHeld = false;

    if (pressDuration > 2000) {
      calibrationMode = true;
      calibratingDown = true;
      Serial.println("Entered Calibration Mode!");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Calibration Mode");
      display.display();
    }
  }

  if (calibrationMode) {
    if (!isMoving && downPressed) {
      myServo.write(0);
      movementStartTime = millis();
      isMoving = true;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Moving Down...");
      display.display();
    }
    if (!isMoving && upPressed) {
      myServo.write(180);
      movementStartTime = millis();
      isMoving = true;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Moving Up...");
      display.display();
    }

    if (calibratePressed && isMoving) {
      unsigned long movementDuration = millis() - movementStartTime;
      isMoving = false;
      myServo.write(90);

      if (calibratingDown) {
        downTime = movementDuration;
        EEPROM.put(EEPROM_DOWNTIME_ADDR, downTime);
        EEPROM.commit();
        calibratingDown = false;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Down Calibrated");
        display.println("Move Up & Press");
        display.display();
        delay(500);
      } else {
        upTime = movementDuration;
        EEPROM.put(EEPROM_UPTIME_ADDR, upTime);
        EEPROM.commit();
        calibrationMode = false;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Calibration Done!");
        display.display();
        delay(500);
      }
    }
    return;
  }

  static bool lastUpState = false;
  static bool lastDownState = false;
  static unsigned long movementStart = 0;

  bool currentUpState = upPressed && !downPressed;
  bool currentDownState = downPressed && !upPressed;

  if (currentUpState && !lastUpState) {
    if (!isMoving) {
      if (blindPosition >= 100.0) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Already at top");
        display.display();
      } else {
        float remainingPercent = 100.0 - blindPosition;
        unsigned long duration = upTime * (remainingPercent / 100.0);
        myServo.write(180);
        isMoving = true;
        movingUp = true;
        movementStart = millis();
        movementStartTime = millis();
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Going Up...");
        display.display();
      }
    } else if (isMoving && movingUp) {
      unsigned long elapsed = millis() - movementStartTime;
      myServo.write(90);
      isMoving = false;
      float percentMoved = (elapsed / (float)upTime) * (100.0 - blindPosition);
      blindPosition += percentMoved;
      if (blindPosition > 100.0) blindPosition = 100.0;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Stopped Up Early");
      display.display();
    }
  }

  if (currentDownState && !lastDownState) {
    if (!isMoving) {
      if (blindPosition <= 0.0) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Already at bottom");
        display.display();
      } else {
        float remainingPercent = blindPosition;
        unsigned long duration = downTime * (remainingPercent / 100.0);
        myServo.write(0);
        isMoving = true;
        movingUp = false;
        movementStart = millis();
        movementStartTime = millis();
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Going Down...");
        display.display();
      }
    } else if (isMoving && !movingUp) {
      unsigned long elapsed = millis() - movementStartTime;
      myServo.write(90);
      isMoving = false;
      float percentMoved = (elapsed / (float)downTime) * blindPosition;
      blindPosition -= percentMoved;
      if (blindPosition < 0.0) blindPosition = 0.0;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Stopped Down Early");
      display.display();
    }
  }

  if (isMoving) {
    unsigned long elapsed = millis() - movementStartTime;
    float targetTime = movingUp ? upTime * ((100.0 - blindPosition) / 100.0) : downTime * (blindPosition / 100.0);
    if (elapsed >= targetTime) {
      myServo.write(90);
      isMoving = false;
      blindPosition = movingUp ? 100.0 : 0.0;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(movingUp ? "Reached Top" : "Reached Bottom");
      display.display();
    }
  }

  if (!isMoving && !currentUpState && !currentDownState) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(temperature, 1);
    display.println(" C");
    display.setCursor(0, 16);
    display.print("Position: ");
    display.print((int)blindPosition);
    display.println("%");
    display.display();
  }

  lastUpState = currentUpState;
  lastDownState = currentDownState;
  delay(50);
}