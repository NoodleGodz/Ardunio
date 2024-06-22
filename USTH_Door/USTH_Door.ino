#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include "SafeState.h"
#include "icons.h"

/* Locking mechanism definitions */
#define SERVO_PIN 10
#define SERVO_LOCK_POS   0
#define SERVO_UNLOCK_POS 90
Servo lockServo;
 
/*************************************************
 * Public Music Note
 *************************************************/
 
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define melodyPin 12

class KalmanFilter {
public:
    KalmanFilter(float process_noise, float sensor_noise, float estimated_error, float initial_value) {
        Q = process_noise;
        R = sensor_noise;
        P = estimated_error;
        X = initial_value;
    }

    float updateEstimate(float measurement) {
        P = P + Q;
        K = P / (P + R);
        X = X + K * (measurement - X);
        P = (1 - K) * P;

        return X;
    }

private:
    float Q; // Process noise covariance
    float R; // Measurement noise covariance
    float P; // Estimation error covariance
    float K; // Kalman gain
    float X; // Value
};

const int trig = 8;     // chân trig của HC-SR04
const int echo = 7;     // chân echo của HC-SR04

KalmanFilter kalmanFilter(0.125, 0.6, 0.1, 0);  

/* Display */
LiquidCrystal_I2C lcd(0x27,16,2);

/* Keypad setup */
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
byte rowPins[KEYPAD_ROWS] = {2, 3, 4, 5};
byte colPins[KEYPAD_COLS] = {A3, A2, A1, A0};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

/* SafeState stores the secret code in EEPROM */
SafeState safeState;

void lock() {
  lockServo.write(SERVO_LOCK_POS);
  safeState.lock();
}

void unlock() {
  lockServo.write(SERVO_UNLOCK_POS);
}

void showStartupMessage() {
  lcd.setCursor(4, 0);
  lcd.print("Hello!");
  delay(1000);

  lcd.setCursor(0, 1);
  String message = "  USTHDoor v2.0";
  for (byte i = 0; i < message.length(); i++) {
    lcd.print(message[i]);
    delay(100);
  }
  delay(500);
}

String inputSecretCode() {
  lcd.setCursor(5, 1);
  lcd.print("[____]");
  lcd.print(" B=");
  lcd.write(ICON_BELL);
  lcd.setCursor(6, 1);
  String result = "";
  while (result.length() < 4) {
    char key = keypad.getKey();
    if (key >= '0' && key <= '9') {
      lcd.print('*');
      result += key;
    }
    if (key == 'B')
    {
      Serial.println("5 : Door ring !! ");
      sing(2);
    } 
  }
  return result;
}

void showWaitScreen(int delayMillis) {
  lcd.setCursor(2, 1);
  lcd.print("[..........]");
  lcd.setCursor(3, 1);
  for (byte i = 0; i < 10; i++) {
    delay(delayMillis);
    lcd.print("=");
  }
}

bool setNewCode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter new code:");
  String newCode = inputSecretCode();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Confirm new code");
  String confirmCode = inputSecretCode();

  if (newCode.equals(confirmCode)) {
    safeState.setCode(newCode);
    Serial.println("0 : Create new Code "+ newCode);
    return true;
  } else {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Code mismatch");
    lcd.setCursor(0, 1);
    lcd.print("Safe not locked!");
    delay(2000);
    return false;
  }
}

bool wait = true;

void showUnlockMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(ICON_UNLOCKED_CHAR);
  lcd.setCursor(4, 0);
  lcd.print("Unlocked!");
  lcd.setCursor(15, 0);
  lcd.write(ICON_UNLOCKED_CHAR);
  delay(1000);
}

void safeUnlockedLogic() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.write(ICON_UNLOCKED_CHAR);
  lcd.setCursor(2, 0);
  lcd.print(" # to lock");
  lcd.setCursor(15, 0);
  lcd.write(ICON_UNLOCKED_CHAR);

  bool newCodeNeeded = true;

  if (safeState.hasCode()) {
    lcd.setCursor(0, 1);
    lcd.print("  A = new code");
    newCodeNeeded = false;
  }

  auto key = keypad.getKey();
  while (key != 'A' && key != '#') {
    key = keypad.getKey();
  }

  bool readyToLock = true;
  if (key == 'A' || newCodeNeeded) {
    readyToLock = setNewCode();
  }

  if (readyToLock) {
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.write(ICON_UNLOCKED_CHAR);
    lcd.print(" ");
    lcd.write(ICON_RIGHT_ARROW);
    lcd.print(" ");
    lcd.write(ICON_LOCKED_CHAR);
    safeState.lock();
    Serial.println("3 : Door locked.");
    lock();
    showWaitScreen(100);
  }
}

void safeLockedLogic() {
  lcd.clear();
  wait = true;
  lcd.setCursor(0, 0);
  lcd.write(ICON_LOCKED_CHAR);
  lcd.print(" Safe Locked! ");
  lcd.write(ICON_LOCKED_CHAR);

  String userCode = inputSecretCode();
  
  bool unlockedSuccessfully = safeState.unlock(userCode);
  showWaitScreen(200);

  if (unlockedSuccessfully) {
    showUnlockMessage();
    Serial.println("1 : Unlock Successfully");
    Serial.println("4 : Door unlocked.");
    unlock();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied!");
    Serial.println("2 : Unlock Failed with code " + userCode);
    showWaitScreen(1000);
  }
}

void setup() {
  lcd.init();
  lcd.backlight(); 
  pinMode(trig, OUTPUT);  // chân trig sẽ phát tín hiệu
  pinMode(echo, INPUT);   // chân echo sẽ nhận tín hiệu
  init_icons(lcd);
  pinMode(melodyPin, OUTPUT);
  lockServo.attach(SERVO_PIN);

  /* Make sure the physical lock is sync with the EEPROM state */
  Serial.begin(9600);
  if (safeState.locked()) {
    lock();
  } else {
    unlock();
  }

  showStartupMessage();
}



void loop() {
  if (safeState.locked()) {
    if (checkDistance()) 
    {
    safeLockedLogic();
    }
  } else {
    safeUnlockedLogic();
  }
}


int underworld_melody[] = {
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0, 0, 0
};
//Underwolrd tempo
int underworld_tempo[] = {
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3, 3, 3
};

 
void sing(int s) {
  int song = s;
  if (song == 2) {
    int size = sizeof(underworld_melody) / sizeof(int);
    for (int thisNote = 0; thisNote < size; thisNote++) {
      int noteDuration = 1000 / underworld_tempo[thisNote];
 
      buzz(melodyPin, underworld_melody[thisNote], noteDuration);
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      buzz(melodyPin, 0, noteDuration);
    }
  } 
}
 
void buzz(int targetPin, long frequency, long length) {
  long delayValue = 1000000 / frequency / 2; 
  long numCycles = frequency * length / 1000; 
  for (long i = 0; i < numCycles; i++) { 
    digitalWrite(targetPin, HIGH); 
    delayMicroseconds(delayValue); 
    digitalWrite(targetPin, LOW); 
    delayMicroseconds(delayValue); }
}


bool checkDistance() {
    if (wait)
    {
      lcd.clear();
      lcd.print("Stand near 5 sec."); 
    }
    wait = false;
    const unsigned long durationThreshold = 5000; // 5 seconds
    const int distanceThreshold = 30;             // 30 cm
    const int interval = 200;      
    int count = 0;               // Measurement interval in ms
    unsigned long startTime = millis();

    while (millis() - startTime < durationThreshold) {
        count += interval;
        if (count % 1000==0)
        { lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Stand near "); 
          lcd.print(5-(count / 1000));
          lcd.print(" sec."); 
        }
        int distance = measureDistance();
        float filteredDistance = kalmanFilter.updateEstimate(distance);

        /* In kết quả ra Serial Monitor */
        //Serial.print("Distance: ");
        //Serial.print(distance);
        //Serial.print(" cm, Filtered: ");
        //Serial.print(filteredDistance);
        //Serial.println(" cm");

        if (filteredDistance >= distanceThreshold) {
            return false;
        }

        delay(interval);
    }
    return true;
}

int measureDistance() {
    unsigned long duration;
    int distance;

    /* Phát xung từ chân trig */
    digitalWrite(trig, LOW);  // tắt chân trig
    delayMicroseconds(2);
    digitalWrite(trig, HIGH); // phát xung từ chân trig
    delayMicroseconds(5);     // xung có độ dài 5 microSeconds
    digitalWrite(trig, LOW);  // tắt chân trig

    /* Tính toán thời gian */
    // Đo độ rộng xung HIGH ở chân echo. 
    duration = pulseIn(echo, HIGH);  
    // Tính khoảng cách đến vật.
    distance = int(duration / 2 / 29.412);

    return distance;
}


