#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// === LCD setup ===
LiquidCrystal_I2C LCD(0x27, 16, 2); // 16x2 LCD

// === Servo motors ===
Servo Entrance_servo;
Servo Exit_servo;
Servo radarServo;

// === Buzzer ===
const int buzzerPin = 25;

// === Ultrasonic pins - Car Park ===
const int trigEntrance = 16;
const int echoEntrance = 17;

const int trigSlot1 = 0;
const int echoSlot1 = 4;

const int trigSlot2 = 12;
const int echoSlot2 = 14;

const int trigSlot3 = 27;
const int echoSlot3 = 26;

const int trigExit = 15;
const int echoExit = 2;

const int cmThreshold = 7;

// === Ultrasonic pins - Radar ===
const int trigRadar = 32;
const int echoRadar = 34;

volatile int radarDistance = 0;

// Task handles
TaskHandle_t TaskCarParkHandle;
TaskHandle_t TaskRadarHandle;

void setup() {
  Serial.begin(115200);
  LCD.init();
  LCD.backlight();

  pinMode(buzzerPin, OUTPUT);

  // Car park sensor pins
  pinMode(trigEntrance, OUTPUT);
  pinMode(echoEntrance, INPUT);

  pinMode(trigSlot1, OUTPUT);
  pinMode(echoSlot1, INPUT);

  pinMode(trigSlot2, OUTPUT);
  pinMode(echoSlot2, INPUT);

  pinMode(trigSlot3, OUTPUT);
  pinMode(echoSlot3, INPUT);

  pinMode(trigExit, OUTPUT);
  pinMode(echoExit, INPUT);

  // Radar pins
  pinMode(trigRadar, OUTPUT);
  pinMode(echoRadar, INPUT);

  // Attach servos
  Entrance_servo.attach(18);
  Exit_servo.attach(19);
  radarServo.attach(13);

  Entrance_servo.write(90);
  Exit_servo.write(90);

  // Create tasks pinned to different cores
  xTaskCreatePinnedToCore(
    carParkTask,
    "CarPark Task",
    8192,
    NULL,
    1,
    &TaskCarParkHandle,
    0);

  xTaskCreatePinnedToCore(
    radarTask,
    "Radar Task",
    4096,
    NULL,
    1,
    &TaskRadarHandle,
    1);
}

void loop() {
  // Nothing needed here; tasks run independently
  delay(1000);
}

// === Car park task ===
void carParkTask(void * parameter) {
  while (1) {
    int distanceEntrance = getDistance(trigEntrance, echoEntrance);
    int distanceSlot1 = getDistance(trigSlot1, echoSlot1);
    int distanceSlot2 = getDistance(trigSlot2, echoSlot2);
    int distanceSlot3 = getDistance(trigSlot3, echoSlot3);
    int distanceExit = getDistance(trigExit, echoExit);

    bool slot1Free = (distanceSlot1 > cmThreshold || distanceSlot1 <= 0);
    bool slot2Free = (distanceSlot2 > cmThreshold || distanceSlot2 <= 0);
    bool slot3Free = (distanceSlot3 > cmThreshold || distanceSlot3 <= 0);

    int available = slot1Free + slot2Free + slot3Free;

    Serial.println("Available Slot(s): " + String(available));
    Serial.println("Entrance: " + String(distanceEntrance) + " cm");
    Serial.println("Slot1: " + String(distanceSlot1) + " cm");
    Serial.println("Slot2: " + String(distanceSlot2) + " cm");
    Serial.println("Slot3: " + String(distanceSlot3) + " cm");
    Serial.println("Exit: " + String(distanceExit) + " cm");

    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("Available Slot: ");
    LCD.setCursor(0, 1);
    LCD.print(available);

    // Entrance logic
    if (distanceEntrance > 0 && distanceEntrance < cmThreshold && available > 0) {
      Entrance_servo.write(0);
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("Welcome :)");
      playWelcome();  // Play welcome melody
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      Entrance_servo.write(90);
    }

    // Exit logic
    if (distanceExit > 0 && distanceExit < cmThreshold) {
      Exit_servo.write(90);
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("Goodbye :(");
      playGoodbye();  // Play goodbye melody
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      Exit_servo.write(0);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay between checks
  }
}

// === Radar task ===
void radarTask(void * parameter) {
  while (1) {
    for (int angle = 15; angle <= 165; angle++) {
      radarServo.write(angle);
      vTaskDelay(25 / portTICK_PERIOD_MS);
      radarDistance = getDistance(trigRadar, echoRadar);

      Serial.print("Radar ");
      Serial.print(angle);
      Serial.print(", ");
      Serial.println(radarDistance);
    }
    for (int angle = 165; angle >= 15; angle--) {
      radarServo.write(angle);
      vTaskDelay(25 / portTICK_PERIOD_MS);
      radarDistance = getDistance(trigRadar, echoRadar);

      Serial.print("Radar ");
      Serial.print(angle);
      Serial.print(", ");
      Serial.println(radarDistance);
    }
  }
}

// === Distance calculation ===
int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

// === Play welcome melody ===
void playWelcome() {
  int melody[] = {523, 659, 784};  // C5, E5, G5 notes
  int noteDurations[] = {200, 200, 400};

  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, melody[i]);
    delay(noteDurations[i]);
    noTone(buzzerPin);
    delay(50);
  }
}

// === Play goodbye melody ===
void playGoodbye() {
  int melody[] = {784, 659, 523};  // G5, E5, C5 notes (reverse)
  int noteDurations[] = {400, 200, 200};

  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, melody[i]);
    delay(noteDurations[i]);
    noTone(buzzerPin);
    delay(50);
  }
}

