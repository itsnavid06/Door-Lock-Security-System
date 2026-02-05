# Door-Lock-Security-System

## 🎯 Project Objectives

- Implement RFID + PIN multi-factor authentication
- Control a servo-based locking mechanism using PWM
- Log access attempts to EEPROM (persistent across power loss)
- Provide real-time user feedback through LCD display and buzzer
- Apply embedded systems concepts learned in coursework

All proposed objectives were successfully achieved.

---

## 🛠 Hardware Components Used

- Freenove Board (Arduino Uno R3 compatible)
- RC522 RFID module
- RFID tag and card
- 4x4 Keypad
- Servo motor
- Passive buzzer
- LCD screen
- Breadboard
- Jumper wires

---

## ⚙️ System Overview

### Authentication Flow

1. User scans registered RFID card
2. System verifies card ID
3. User enters correct PIN on keypad
4. If both factors are valid:
   - Servo motor unlocks door
   - LCD displays access granted
   - Buzzer provides confirmation tone
   - Access attempt is logged in EEPROM
5. If authentication fails:
   - LCD displays access denied
   - Buzzer provides alert tone
   - Failed attempt is logged in EEPROM

---

## 🧠 Embedded Concepts Demonstrated

- PWM (Pulse Width Modulation) for servo control
- SPI communication with RC522 RFID module
- Interrupts and timers for input handling
- EEPROM usage for persistent access logging
- Real-time feedback systems integration

---

## 🚀 How to Set Up and Run

### 1️⃣ Software Requirements

- Arduino IDE
- Required Libraries:
  - MFRC522
  - Servo
  - LiquidCrystal
  - Keypad

Install libraries via:
Arduino IDE → Sketch → Include Library → Manage Libraries

---

### 2️⃣ Hardware Connections

Connect components according to your wiring diagram:

- RC522 RFID → SPI pins
- Servo → PWM digital pin
- Keypad → Digital I/O pins
- LCD → Digital pins (or I2C if applicable)
- Buzzer → Digital output pin

(Include circuit diagram image here if available)

---

### 3️⃣ Upload the Code

1. Connect Freenove / Arduino board via USB
2. Open the `.ino` file in Arduino IDE
3. Select:
   - Board: Arduino Uno
   - Correct COM Port
4. Click **Upload**

---
