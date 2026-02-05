#include <SPI.h>
#include <EEPROM.h>
#include <SD.h>
//#include <RFID.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define RST_PIN  9
#define RFID_CS   10 //note this is labelled 'SDA' on the RFID module

const int RECORD_SIZE = 5;
const int MAX_RECORDS = 20;

// Data management bytes
const int INDEX_ADDR = 200;      // where the next write goes
const int COUNT_ADDR = 201;      // how many records stored

// RFID.h object (SDA = RFID_CS, RST = RST_PIN)
//RFID rfid(RFID_CS, RST_PIN);
MFRC522 rfid(RFID_CS, RST_PIN);
//unsigned char str[MAX_LEN];   // Buffer for card data

// Master card UID 
String MasterTag = "3DEF0201";  
String UIDCard = "";

// PIN
const int pin = 3207;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo = door lock
Servo servo;
const int ServoPin           = 6;
const int servoLockedAngle   = 10;    // closed position
const int servoUnlockedAngle = 100;   // open position

// buzzer pin
#define Buzzer 5

//failed attempts
int failedAttempts = 0;

//SD card pins
/*
SD card chip select pin
Disable when not in use because MOSI/MISO/SCK is shared with RFID reader
*/
#define SD_CS 4
Sd2Card card;
SdVolume volume;
SdFile root;

//KEYBOARD RELATED
//DIAGRAM: https://www.amazon.ca/Matrix-Membrane-Keyboard-Arduino-Microcontroller/dp/B086Z1ZXNJ?source=ps-sl-shoppingads-lpcontext&ref_=fplfs&psc=1&smid=A2RJ79XBQX6W3M
#define kr1 A3
#define kr2 A2
#define kr3 A1
#define kr4 A0
#define kc1 2
#define kc2 3
#define kc3 7
#define kc4 8

char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

const int rows[4] = {kr1, kr2, kr3, kr4};
const int cols[4] = {kc1, kc2, kc3, kc4};

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 4; i++) {
    pinMode(rows[i], INPUT_PULLUP);
    digitalWrite(rows[i], HIGH); // Idle state HIGH
  }

  for (int i = 0; i < 4; i++) {
    pinMode(cols[i], OUTPUT);
    digitalWrite(cols[i], LOW); // Idle state LOW
  }

  //Enable RFID reader only when needed
  pinMode(RFID_CS, OUTPUT);
  digitalWrite(RFID_CS, HIGH);

  SPI.begin();

  digitalWrite(RFID_CS, LOW);
  //rfid.init();
  rfid.PCD_Init();
  digitalWrite(RFID_CS, HIGH);
  //delay(500);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  servo.attach(ServoPin);
  servo.write(servoLockedAngle);   // start locked

  pinMode(Buzzer, OUTPUT);
  noTone(Buzzer);

  lcd.clear();
  lcd.print(" Access Control ");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card>>");
}

void loop() {

  char key = readKeypad();

  if (key != ' ') {  // if a key was pressed
    Serial.print("Key Pressed: ");
    Serial.println(key);

    if(key == '#'){
      readAllAttempts();
    }

    if(key == 'C'){
      resetLogIndex();
      Serial.println("Reset log indexes");
    }
  }
 
  delay(200); // slow down a bit

  digitalWrite(RFID_CS, LOW);

  // Check if a card is present and read it
  if (getUID()) {

    Serial.print("UID: ");
    Serial.println(UIDCard);

    // lcd.clear();
    // lcd.setCursor(2, 0);
    // lcd.print("Permission");
    // lcd.setCursor(0, 1);

    // CARD ACCEPTED
    if (UIDCard == MasterTag) {

      // 1 buzz
      tone(Buzzer, 2000);
      delay(300);
      noTone(Buzzer);

      delay(1000);

      int attempts = 0;
      int pinSuccess = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   Enter PIN");
      delay(100);
      lcd.setCursor(0, 1);
      lcd.print("Press D to enter");

      int inputPin = 0;
      int factor = 1000;
      for(int i = 0; i < 4; i++){
        char c = readKeypadBlocking();
        int n = charToInt(c);
        if(n != -1){
          inputPin += factor * n;
          factor /= 10;
        }
        Serial.println(inputPin);
        Serial.println(pin);
      }

      if(inputPin == pin){
        pinSuccess = 1;
      } else {
        // 2 buzzes for failed unlock attempt
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   Wrong PIN ");
        lcd.setCursor(0, 1);
        lcd.print("   Try Again ");

        for (int i = 0; i < 2; i++) {
          tone(Buzzer, 2000);
          delay(300);
          noTone(Buzzer);
          delay(200);
        }

        delay(2000);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   Enter PIN");
        lcd.setCursor(0, 1);
        lcd.print("Press D to enter");
      }

      if(pinSuccess){
        logAttempt(true);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Access Granted!");
        lcd.setCursor(0, 1);
        Serial.println("Door unlocked");
      
        // Reset failed attempts counter
        failedAttempts = 0;

        // Unlock the door
        servo.write(servoUnlockedAngle);

        // 1 buzz for successful unlock
        tone(Buzzer, 2000);
        delay(300);
        noTone(Buzzer);

        // Keep door unlocked for a few seconds
        delay(5000);

        // Lock the door again
        servo.write(servoLockedAngle);
      } else {
        logAttempt(false);
        Serial.println("Door locked");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Access denied. ");
        lcd.setCursor(0, 1);
        lcd.print("Scan card again");
      }
    } else { // ACCESS DENIED
      logAttempt(false);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(" Access Denied!");

      // make sure door stays locked
      servo.write(servoLockedAngle);

      failedAttempts++;

      // 2 buzzes for failed unlock attempt
      for (int i = 0; i < 2; i++) {
        tone(Buzzer, 2000);
        delay(300);
        noTone(Buzzer);
        delay(200);
      }
      delay(2000);

      // On 3 failed attempts: 10-second continuous buzz
      if (failedAttempts >= 3) {
        Serial.println("3 failed attempts! Alarm!");
        tone(Buzzer, 2000);
        delay(10000);   // 10 seconds
        noTone(Buzzer);

        failedAttempts = 0;
      }

    }
    
    // After handling the card, go back to "scan" screen
    lcd.clear();
    lcd.print(" Access Control ");
    lcd.setCursor(0, 1);
    lcd.print("Scan Your Card>>");
  }

  // If no card, loop just keeps idling, door remains in last position
}

boolean getUID() {
  // Look for a card
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  UIDCard = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    UIDCard += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    UIDCard += String(rfid.uid.uidByte[i], HEX);
  }
  UIDCard.toUpperCase();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}

/*
Rows are set to INPUT_PULLUP
Columns are set to OUTPUT
*/
char readKeypad() {
  for (int r = 0; r < 4; r++) {
    digitalWrite(rows[r], HIGH); //ensure row is HIGH
    for (int c = 0; c < 4; c++) {
      digitalWrite(cols[c], LOW);
      if(digitalRead(rows[r]) == LOW){
        delay(50); //debounce
        while(digitalRead(rows[r]) == LOW); //wait until released
        return keys[r][c];
      }
      digitalWrite(cols[c], HIGH);
      delay(10); //just in case there is a delay before it's HIGH again
    }
  }
  return ' '; // No key
}

char readKeypadBlocking(){
  char c = ' ';
  while(c == ' '){
    //Serial.println("Key pressed (Blocking): " + c);
    c = readKeypad();
  }
  return c;
}

int charToInt(char c){
  int intValue = c - '0';
  if(intValue >= 0 && intValue <= 9){
    return intValue;
  }
  return -1;
}

void logAttempt(bool success) {
    unsigned long ts = millis();

    int index = EEPROM.read(INDEX_ADDR);
    int count = EEPROM.read(COUNT_ADDR);

    if (index >= MAX_RECORDS) index = 0;
    if (count < MAX_RECORDS) count++;

    int base = index * RECORD_SIZE;

    EEPROM.update(base, success ? 1 : 0);

    EEPROM.update(base + 1, (byte)(ts & 0xFF));
    EEPROM.update(base + 2, (byte)((ts >> 8) & 0xFF));
    EEPROM.update(base + 3, (byte)((ts >> 16) & 0xFF));
    EEPROM.update(base + 4, (byte)((ts >> 24) & 0xFF));

    index = (index + 1) % MAX_RECORDS;

    EEPROM.update(INDEX_ADDR, index);
    EEPROM.update(COUNT_ADDR, count);
}

void readAttempt(int idx, bool &success, unsigned long &ts) {
    int base = idx * RECORD_SIZE;

    success = EEPROM.read(base);

    ts = 0;
    ts |= (unsigned long)EEPROM.read(base + 1);
    ts |= (unsigned long)EEPROM.read(base + 2) << 8;
    ts |= (unsigned long)EEPROM.read(base + 3) << 16;
    ts |= (unsigned long)EEPROM.read(base + 4) << 24;
}

void readAllAttempts() {
    int index = EEPROM.read(INDEX_ADDR); // next write location
    int count = EEPROM.read(COUNT_ADDR); // number of valid records

    if (count == 0) {
        Serial.println("No attempts logged yet.");
        return;
    }

    Serial.println("=== Attempt Log (oldest → newest) ===");

    // oldest record = (index - count + MAX_RECORDS) % MAX_RECORDS
    int start = (index - count + MAX_RECORDS) % MAX_RECORDS;

    for (int i = 0; i < count; i++) {
        int r = (start + i) % MAX_RECORDS;

        bool success;
        unsigned long ts;
        readAttempt(r, success, ts);

        Serial.print("#");
        Serial.print(i + 1);
        Serial.print("  ");
        Serial.print(success ? "SUCCESS" : "FAILURE");
        Serial.print("  @ millis ");
        Serial.println(ts);
    }
}

// void eraseAttemptLog() {
//     // erase all records used by the circular buffer
//     for (int i = 0; i < MAX_RECORDS * RECORD_SIZE; i++) {
//         EEPROM.update(i, 0xFF); // use 0xFF as erased state
//     }

//     // reset control bytes
//     EEPROM.update(INDEX_ADDR, 0);
//     EEPROM.update(COUNT_ADDR, 0);

//     Serial.println("Attempt log erased.");
// }

void resetLogIndex(){
  EEPROM.update(INDEX_ADDR, 0);
  EEPROM.update(COUNT_ADDR, 0);
}
