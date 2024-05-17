#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define TRIG_PIN 2
#define ECHO_PIN 3
#define LED_PIN 13 // Optional: Use an LED to indicate object detection
#define SERVO_PIN 8 // Servo motor pin
#define SS_PIN 10
#define RST_PIN 9

#define IR_SENSOR_1 4
#define IR_SENSOR_2 5
#define IR_SENSOR_3 6
#define IR_SENSOR_4 7

String UID = "77 B6 0E 17"; // Replace with your RFID card UID
byte lock = 0;

long duration;
int distance;
int thresholdDistance = 20; // Distance threshold in centimeters
int gateOpenDistance = 10;  // Distance threshold to open gate in centimeters
unsigned long openTime = 0;
unsigned long closeDelay = 5000; // Close the gate after 5 seconds of no object detected (adjust as needed)

unsigned long previousMillis = 0;
const long interval = 2000; // Interval to refresh parking slot status

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT); // Optional: Set LED pin as output
  pinMode(IR_SENSOR_1, INPUT);
  pinMode(IR_SENSOR_2, INPUT);
  pinMode(IR_SENSOR_3, INPUT);
  pinMode(IR_SENSOR_4, INPUT);

  myServo.attach(SERVO_PIN); // Attach the servo to the pin
  myServo.write(0); // Initial position of the servo (closed gate)

  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight

  SPI.begin();
  rfid.PCD_Init();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Clear the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Set the trigPin HIGH for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance
  distance = duration * 0.034 / 2;

  // Check if the distance is within the threshold distance
  if (distance <= thresholdDistance) {
    digitalWrite(LED_PIN, HIGH); // Turn on LED
    openTime = millis(); // Reset the timer when object detected
    
    // Check if the distance is within the gate open distance
    if (distance <= gateOpenDistance && distance > 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Welcome!!! ");
      lcd.setCursor(0, 1);
      lcd.print(" Tap Your Card  ");

      // Check if an RFID card is present
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String ID = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          ID.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
          ID.concat(String(rfid.uid.uidByte[i], HEX));
        }
        ID.toUpperCase();

        if (ID.substring(1) == UID && lock == 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(" Thank You!!! ");
          servoOpenGate();
          delay(2000); // Display "Thank You!!!" for 2 seconds
          lock = 1;
        } else if (ID.substring(1) == UID && lock == 1) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(" Thank You!!! ");
          servoCloseGate();
          delay(2000); // Display "Thank You!!!" for 2 seconds
          lock = 0;
        } else {
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("Invalid Card     ");
          delay(2000);
        }
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
    }
  } else {
    digitalWrite(LED_PIN, LOW); // Turn off LED
    
    // Close the gate after a certain delay
    if (millis() - openTime > closeDelay && openTime != 0) {
      servoCloseGate();
      openTime = 0; // Reset the openTime to prevent continuous closing
    }
  }

  // Refresh the parking slot status every 2 seconds
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    displayParkingSlots();
  }

  // Small delay before the next measurement
  delay(500);
}

void servoOpenGate() {
  myServo.write(90); // Open gate (adjust the angle as needed)
  delay(5000); // Keep the gate open for 5 seconds (adjust as needed)
}

void servoCloseGate() {
  myServo.write(0); // Close gate
}

void displayParkingSlots() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("S1:");
  lcd.print(digitalRead(IR_SENSOR_1) ? "NO.P" : "P.AV");
  lcd.setCursor(8, 0);
  lcd.print("S2:");
  lcd.print(digitalRead(IR_SENSOR_2) ? "NO.P" : "P.AV");

  lcd.setCursor(0, 1);
  lcd.print("S3:");
  lcd.print(digitalRead(IR_SENSOR_3) ? "NO.P" : "P.AV");
  lcd.setCursor(8, 1);
  lcd.print("S4:");
  lcd.print(digitalRead(IR_SENSOR_4) ? "NO.P" : "P.AV");
}
