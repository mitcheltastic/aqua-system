# AQUA System - Atmospheric, Quantitative & Underground Analysis

## 📋 Project Overview

**AQUA** is an advanced IoT-based environmental monitoring system designed to track and analyze atmospheric, quantitative, and underground water conditions. The system provides real-time monitoring of water levels, soil moisture, and rainfall intensity with intelligent alerting mechanisms to prevent flooding and water-related disasters.

### Key Capabilities
- **Water Level Detection**: Ultrasonic sensor for precise water depth measurement
- **Soil Moisture Monitoring**: Real-time soil moisture percentage tracking
- **Rain Detection**: Multi-level rainfall intensity classification
- **Smart Alerting**: Three-tier alert system (Safe, Warning, Danger) with visual and audible notifications
- **Cloud Integration**: Real-time Firebase Realtime Database synchronization
- **Data Logging**: Persistent historical logging with timestamps
- **Local Dashboard**: 16x2 LCD display for on-site monitoring

---

## 🎯 System Features

### Sensor Readings & Thresholds

| Component | Sensor Type | Range | Purpose |
|-----------|------------|-------|---------|
| **Water Level** | Ultrasonic (HC-SR04) | 0-400+ cm | Measures water depth |
| **Soil Moisture** | Capacitive Soil Sensor | 0-100% | Detects soil saturation |
| **Rainfall** | Analog Rain Sensor | 4095-0 | Measures rain intensity |

### Alert System

```
🟢 SAFE (State 0)
   ├─ Water Level: > 55 cm AND
   ├─ Soil Moisture: < 50% AND
   └─ Rainfall: Light/None

🟡 WARNING (State 1)
   ├─ Water Level: 45-55 cm OR
   ├─ Soil Moisture: 50-80% OR
   └─ Rainfall: Moderate

🔴 DANGER (State 2)
   ├─ Water Level: < 45 cm OR
   ├─ Soil Moisture: > 80% (with rising water) OR
   └─ Rainfall: Heavy
```

### Output Indicators

| Output | State 0 | State 1 | State 2 |
|--------|---------|---------|---------|
| **LED Green** | ON | OFF | OFF |
| **LED Yellow** | OFF | ON | OFF |
| **LED Red** | OFF | OFF | ON |
| **Buzzer** | Silent | Silent | Alarm Pattern |

---

## 🔧 Hardware Components

### Microcontroller
- **ESP32** (Dual-core, WiFi-enabled)

### Sensors
- **Ultrasonic Sensor (HC-SR04)**: Water level measurement
- **Capacitive Soil Moisture Sensor**: Soil saturation detection
- **Analog Rain Sensor**: Rainfall intensity detection

### Output Devices
- **16x2 LCD Display (I2C)**: Real-time data visualization
- **RGB LED Module**: Status indication (Green/Yellow/Red)
- **Dual Buzzers (2kHz & 1.5kHz)**: Audible alarm system

### Connectivity
- **WiFi Module**: Built-in ESP32 WiFi
- **Firebase Realtime Database**: Cloud data storage

### Pin Configuration

```cpp
// Sensors
#define TRIG_PIN    5      // Ultrasonic trigger
#define ECHO_PIN    18     // Ultrasonic echo
#define RAIN_PIN    35     // Analog rain sensor
#define SOIL_PIN    34     // Analog soil sensor

// Outputs
#define BUZZER_HIGH 12     // 2kHz alarm
#define BUZZER_LOW  13     // 1.5kHz alarm
#define LED_GREEN   26
#define LED_YELLOW  27
#define LED_RED     25
```

---

## 📡 Cloud Integration

### Firebase Realtime Database Structure

```
AQUA/
├── Current/
│   ├── water: float (cm)
│   ├── soil: int (%)
│   ├── rain: int (0-4095)
│   └── status: string ("SAFE"/"WARNING"/"DANGER")
│
└── History/
    └── [timestamp-based entries]
        ├── water: float
        ├── soil: int
        ├── rain: int
        ├── status: string
        └── timestamp: string (YYYY-MM-DD HH:MM:SS)
```

### Data Upload Strategy
- **Live Updates**: Every 5 seconds (periodic sync)
- **Event-Triggered**: Immediately on state change
- **History Logging**: Every 5 minutes or on DANGER state
- **Timestamps**: Indonesia/WIB timezone (UTC+7)

---

## 🛠️ Setup & Installation

### Prerequisites
- Arduino IDE or PlatformIO
- ESP32 Board Package installed
- Required Libraries:
  ```
  - Wire.h (Built-in)
  - LiquidCrystal_I2C
  - WiFi.h (Built-in)
  - Firebase_ESP_Client
  - time.h (Built-in)
  ```

### Installation Steps

1. **Clone/Download the Repository**
   ```bash
   git clone https://github.com/mitcheltastic/aqua-system.git
   cd aqua-system
   ```

2. **Install Required Libraries** (Arduino IDE)
   - Sketch → Include Library → Manage Libraries
   - Search and install:
     - `LiquidCrystal_I2C` by Frank de Brabander
     - `Firebase Arduino Client Library` by Mobizt

3. **Configure Credentials**
   Edit `aqua_system.cpp` and update:
   ```cpp
   #define WIFI_SSID "your_wifi_ssid"
   #define WIFI_PASSWORD "your_wifi_password"
   #define API_KEY "your_firebase_api_key"
   #define DATABASE_URL "your_firebase_database_url"
   ```

4. **Hardware Connections**
   - Connect sensors and outputs according to pin configuration
   - Verify I2C address of LCD display (default: 0x27)
   - Power ESP32 with 5V supply

5. **Upload Code**
   - Select ESP32 board and COM port in Arduino IDE
   - Click Upload

6. **Monitor Serial Output**
   - Open Serial Monitor (115200 baud)
   - Verify WiFi and Firebase connections

---

## 📊 LCD Display Modes

### Screen 1 (Rotates every 3 seconds)
```
STATUS: SAFE
W:65cm S:45%
```
- Shows system status and current readings
- Water depth (W), Soil moisture (S)

### Screen 2
```
Rain Intensity:
>> Moderate <<
```
- Detailed rainfall classification
- Options: HEAVY, Moderate, None/Light

---

## 🚀 Operation

### Startup Sequence
1. System initializes all GPIO pins
2. LCD displays "Connecting WiFi"
3. Connects to configured WiFi network
4. Synchronizes time with NTP server (pool.ntp.org)
5. Authenticates with Firebase
6. Begins sensor polling

### Runtime Loop
- **Every 100ms**: Read sensors and update system state
- **Every 3s**: Update LCD display (alternating screens)
- **Every 5s**: Upload current readings to Firebase
- **On state change**: Immediately upload to Firebase
- **Continuous**: Monitor danger state and trigger alarms

---

## 🔊 Alarm Pattern (DANGER State)

```
High Beep (2000Hz):  ▓▓▓▓░░░
  Duration:          600ms
  Silence:           100ms

Low Beep (1500Hz):   ▓▓▓▓░░░
  Duration:          600ms
  Silence:           100ms

[Pattern repeats]
```

---

## 📱 Monitoring Dashboard

The Firebase Realtime Database can be monitored via:
- **Firebase Console** (Web Dashboard)
- **Mobile Apps**: Firebase-integrated mobile applications
- **Custom Web Interface**: Using Firebase REST API

---

## 🧪 Testing & Calibration

### Sensor Calibration

**Soil Moisture:**
- `SOIL_DRY = 3175` (Dry reading)
- `SOIL_WET = 2000` (Wet reading)
- Adjust values based on your sensor

**Water Level Thresholds:**
- `WATER_DANGER_CM = 45` (Critical level)
- `WATER_WARN_CM = 55` (Warning level)

**Rainfall Thresholds:**
- `RAIN_HEAVY_THRESH = 1500` (Heavy rain)
- `RAIN_LIGHT_THRESH = 2500` (Light rain)

### Manual Testing
1. **Test LED outputs**: Manually trigger system states
2. **Verify buzzers**: Play alarm patterns in DANGER mode
3. **Check sensor readings**: Monitor serial output for raw values
4. **Firebase sync**: Confirm data appears in console

---

## 📈 Performance Specifications

| Specification | Value |
|--------------|-------|
| Sensor Poll Interval | 100 ms |
| LCD Update Interval | 3 seconds |
| Firebase Upload (Normal) | 5 seconds |
| Firebase Upload (Critical) | Immediate |
| History Log Interval | 5 minutes |
| WiFi Retry Timeout | 10 seconds |
| Time Zone | UTC+7 (Indonesia/WIB) |

---

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| WiFi connection fails | Verify SSID/password, check signal strength |
| Firebase not syncing | Check API key, database URL, internet connection |
| LCD not displaying | Verify I2C address (use I2C scanner), check power |
| Sensors reading 0/max | Check pin connections, verify analog pins active |
| Ultrasonic always shows 999cm | Check TRIG/ECHO pins, verify sensor power |
| Buzzers not working | Check pin connections, test tone() function |
| Time stamp shows "N/A" | Wait for NTP sync (may take 30-60 seconds) |

---

## 📝 Code Architecture

### Main Components

**Setup()**
- GPIO initialization
- WiFi connection
- Time synchronization
- Firebase authentication

**Loop()**
1. Sensor reading (100ms interval)
2. State determination (hybrid logic)
3. Smart Firebase upload
4. Output control (LEDs/Buzzers)
5. LCD display update

**Helper Functions**
- `getDistance()`: Ultrasonic measurement
- `sendToFirebase()`: Data upload & history logging
- `playPJLAlarm()`: Alarm pattern generation
- `silenceBuzzers()`: Alarm deactivation
- `getFormattedTime()`: Timestamp formatting

---

## 📄 License

This project is provided for educational and research purposes.

---

## 👨‍💻 Author

**AQUA System Development Team**  
Microprocessor & IoT Course - Semester 5

---

## 📞 Support & Feedback

For issues, questions, or improvements, please refer to the project repository or contact the development team.

---

**Last Updated**: February 1, 2026  
**Version**: 1.0.0  
**Status**: Active Development