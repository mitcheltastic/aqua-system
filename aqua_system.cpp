#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h" // NEW: Built-in library for Time

// ==========================================
//          USER CONFIGURATION
// ==========================================
// hape zaky
#define WIFI_SSID "wifi id"       
#define WIFI_PASSWORD "wifi pass" 

#define API_KEY "API KEY" 
#define DATABASE_URL "DB URL" 

// --- TIME CONFIGURATION (For Indonesia/WIB) ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200; // UTC+7 (7 * 3600)
const int   daylightOffset_sec = 0; // No Daylight Savings in Indo

// ==========================================
//            HARDWARE CONFIGURATION
// ==========================================
#define TRIG_PIN    5
#define ECHO_PIN    18
#define RAIN_PIN    35    
#define SOIL_PIN    34    
#define BUZZER_HIGH 12    
#define BUZZER_LOW  13    
#define LED_GREEN   26
#define LED_YELLOW  27
#define LED_RED     25    

const int SOIL_DRY = 3175;
const int SOIL_WET = 2000;
const int RAIN_HEAVY_THRESH = 1500;  
const int RAIN_LIGHT_THRESH = 2500;
const int WATER_DANGER_CM = 45;   
const int WATER_WARN_CM   = 55;   
const float SOUND_SPEED = 0.0343;

// ==========================================
//               OBJECTS & VARS
// ==========================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json; 

unsigned long lastScreenUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long buzzerTimer = 0;
unsigned long lastFirebaseUpload = 0; 
unsigned long lastHistoryLog = 0; 

int systemState = 0; 
int lastSystemState = -1; 
int buzzerStep = 0;  
bool lcdScreenOne = true; 
bool wifiConnected = false;

// --- PROTOTYPES ---
float getDistance();
void silenceBuzzers();
void playPJLAlarm(unsigned long currentMillis);
void sendToFirebase(float dist, int rain, int soil, int state);
String getFormattedTime(); // NEW Helper

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);
  pinMode(BUZZER_HIGH, OUTPUT);
  pinMode(BUZZER_LOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tryCount = 0;
  while (WiFi.status() != WL_CONNECTED && tryCount < 20) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(tryCount % 16, 1);
    lcd.print(".");
    tryCount++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    lcd.print("WiFi Connected!");
    Serial.println("\nWiFi Connected");
    
    // NEW: Init Time Sync
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    Firebase.signUp(&config, &auth, "", ""); 
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
  } else {
    lcd.print("WiFi Failed :(");
  }
  
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  // -------------------------------------------------
  // 1. SENSOR READING (100ms)
  // -------------------------------------------------
  static int rainRaw = 4095;
  static int soilPercent = 0;
  static float distance = 0.0;

  if (currentMillis - lastSensorRead > 100) { 
    lastSensorRead = currentMillis;
    
    rainRaw = analogRead(RAIN_PIN);
    
    int soilRaw = analogRead(SOIL_PIN);
    soilPercent = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    distance = getDistance();
    
    // --- HYBRID LOGIC ---
    if (distance < WATER_DANGER_CM) {
       systemState = 2; 
    }
    else if (distance < WATER_WARN_CM && (rainRaw < RAIN_HEAVY_THRESH || soilPercent > 80)) {
       systemState = 2; 
    }
    else if (distance < WATER_WARN_CM || rainRaw < RAIN_LIGHT_THRESH || soilPercent > 50) {
       systemState = 1; 
    }
    else {
       systemState = 0; 
    }
  }

  // -------------------------------------------------
  // 2. SMART UPLOAD
  // -------------------------------------------------
  if (wifiConnected) {
    bool timeToSend = (currentMillis - lastFirebaseUpload > 5000);
    bool stateChanged = (systemState != lastSystemState);
    
    if (timeToSend || stateChanged) {
       lastFirebaseUpload = currentMillis;
       lastSystemState = systemState; 
       
       if(stateChanged) Serial.println(">> STATE CHANGE: Uploading...");
       
       sendToFirebase(distance, rainRaw, soilPercent, systemState);
    }
  }

  // -------------------------------------------------
  // 3. OUTPUT CONTROL
  // -------------------------------------------------
  if (systemState == 0) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    silenceBuzzers(); 
  } 
  else if (systemState == 1) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED, LOW);
    silenceBuzzers(); 
  } 
  else if (systemState == 2) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
    playPJLAlarm(currentMillis);
  }

  // -------------------------------------------------
  // 4. LCD DASHBOARD
  // -------------------------------------------------
  if (currentMillis - lastScreenUpdate > 3000) {
    lastScreenUpdate = currentMillis;
    lcdScreenOne = !lcdScreenOne; 
    lcd.clear();
    
    if (lcdScreenOne) {
      lcd.setCursor(0,0);
      if(systemState == 2) lcd.print("STATUS: DANGER!");
      else if(systemState == 1) lcd.print("STATUS: WARNING");
      else lcd.print("STATUS: SAFE");
      
      lcd.setCursor(0,1);
      lcd.print("W:"); lcd.print((int)distance); 
      lcd.print("cm S:"); lcd.print(soilPercent); lcd.print("%");
    } else {
      lcd.setCursor(0,0);
      lcd.print("Rain Intensity:");
      lcd.setCursor(0,1);
      if(rainRaw < RAIN_HEAVY_THRESH) lcd.print(">> HEAVY <<");
      else if(rainRaw < RAIN_LIGHT_THRESH) lcd.print(">> Moderate <<");
      else lcd.print(">> None/Light <<");
    }
  }
}

// -------------------------------------------------
//              HELPER FUNCTIONS
// -------------------------------------------------

// NEW: Helper to get string like "2024-12-09 14:30:05"
String getFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "N/A"; // Return error if time not yet synced
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void sendToFirebase(float dist, int rain, int soil, int state) {
  if (Firebase.ready()) {
    
    // PART 1: LIVE
    Firebase.RTDB.setFloat(&fbdo, "/AQUA/Current/water", dist);
    Firebase.RTDB.setInt(&fbdo, "/AQUA/Current/soil", soil);
    Firebase.RTDB.setInt(&fbdo, "/AQUA/Current/rain", rain);
    
    String statusStr = "SAFE";
    if(state == 1) statusStr = "WARNING";
    if(state == 2) statusStr = "DANGER";
    Firebase.RTDB.setString(&fbdo, "/AQUA/Current/status", statusStr);

    // PART 2: HISTORY
    bool timeToLog = (millis() - lastHistoryLog > 300000); 
    bool criticalEvent = (state == 2); 
    
    if (timeToLog || criticalEvent) {
       lastHistoryLog = millis(); 
       
       json.clear(); 
       json.add("water", dist);
       json.add("soil", soil);
       json.add("rain", rain);
       json.add("status", statusStr);
       
       // NEW: Add Human Readable Time
       String timeStr = getFormattedTime();
       json.add("timestamp", timeStr);
       
       Firebase.RTDB.pushJSON(&fbdo, "/AQUA/History", &json);
       Serial.print(">> HISTORY LOG SAVED: ");
       Serial.println(timeStr);
    }
  }
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); 
  if (duration == 0) return 999.0; 
  return (duration * SOUND_SPEED) / 2;
}

void silenceBuzzers() {
  noTone(BUZZER_HIGH);
  noTone(BUZZER_LOW);
  buzzerStep = 0; 
}

void playPJLAlarm(unsigned long currentMillis) {
  if (currentMillis - buzzerTimer > 100) { 
    if (buzzerStep == 0) { tone(BUZZER_HIGH, 2000); buzzerTimer = currentMillis; buzzerStep = 1; }
    else if (buzzerStep == 1 && (currentMillis - buzzerTimer > 600)) { noTone(BUZZER_HIGH); buzzerTimer = currentMillis; buzzerStep = 2; }
    else if (buzzerStep == 2 && (currentMillis - buzzerTimer > 100)) { tone(BUZZER_LOW, 1500); buzzerTimer = currentMillis; buzzerStep = 3; }
    else if (buzzerStep == 3 && (currentMillis - buzzerTimer > 600)) { noTone(BUZZER_LOW); buzzerTimer = currentMillis; buzzerStep = 4; }
    else if (buzzerStep == 4 && (currentMillis - buzzerTimer > 100)) { buzzerStep = 0; }
  }
}