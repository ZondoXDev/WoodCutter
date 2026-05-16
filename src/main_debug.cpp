#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "web/secrets.h"
#include <LittleFS.h>

WebServer server(80);

/* ---------------- STEPPER ---------------- */

const int steps[4][4] = {
  {1,0,0,1},
  {1,1,0,0},
  {0,1,1,0},
  {0,0,1,1}
};

struct Stepper {
  int pins[4];
  int stepIndex;
  long position;
  long target;
  bool moving;
  int dir;
  
  // Homing vars
  bool isHoming;
  int homingDir;
};

Stepper pusher;
Stepper limiter;

/* ---------------- SERVO ---------------- */

Servo servo;

const int pinServo = 33; 
int minAngle = 20;
int maxAngle = 75;

/* ---------------- ENDSTOP SENSORS ---------------- */

const int IR_Pusher_Home = 34;
const int IR_Pusher_Ext = 35;
const int IR_Limiter_Home = 36;
const int IR_Limiter_Ext = 39;

/* ---------------- STEPPER PINS ---------------- */

const int PINS_PUSHER[4] = {14, 27, 26, 25};
const int PINS_LIMITER[4] = {22, 21, 19, 18};

const int HOME_SENSOR_ACTIVE = HIGH;
const int HOME_DIRECTION = -1; // Standard: w stronę 0

int servoPos = minAngle;
const int servoStep = 1;

/* ---------------- LIMITS ---------------- */

const long AXIS_MIN = 0;
const long PUSHER_MAX = 4200;
const long LIMITER_MAX = 4200;

/* ---------------- SYSTEM ---------------- */

unsigned long lastStep = 0;
const int stepDelay = 3;

/* ---------------- HARDWARE FUNCTIONS ---------------- */

void initStepper(Stepper &m, int p1,int p2,int p3,int p4){
  m.pins[0] = p1; m.pins[1] = p2; m.pins[2] = p3; m.pins[3] = p4;
  m.stepIndex = 0;
  m.position = 0;
  m.target = 0;
  m.moving = false;
  m.isHoming = false;
  m.dir = 0;

  for(int i=0;i<4;i++) pinMode(m.pins[i], OUTPUT);
}

void stopMotor(Stepper &m){
  for(int i=0;i<4;i++) digitalWrite(m.pins[i], LOW);
}

bool isSensorActive(int pin) {
  return digitalRead(pin) == HOME_SENSOR_ACTIVE;
}

void stepOnce(Stepper &m){ 
  m.stepIndex += m.dir;
  if(m.stepIndex > 3) m.stepIndex = 0;
  if(m.stepIndex < 0) m.stepIndex = 3;

  for(int i=0;i<4;i++)
    digitalWrite(m.pins[i], steps[m.stepIndex][i]);

  m.position += m.dir;
}

/* ---------------- MOTION LOGIC ---------------- */

void moveTo(Stepper &m, long target){
  if(target == m.position) return;
  m.target = target;
  // Czysta matematyka: pozytywne to do przodu
  m.dir = (target > m.position) ? 1 : -1; 
  m.moving = true;
  m.isHoming = false; // Ręczny ruch przerywa homing
}

void startHoming(Stepper &m) {
  m.isHoming = true;
  m.homingDir = HOME_DIRECTION;
  m.dir = m.homingDir;
  m.moving = true;
}

// Główna funkcja wykonująca ruch i pilnująca bezpieczeństwa
void handleMotion(Stepper &m, int homePin, int extPin) {
  if (!m.moving) return;

  if (m.isHoming) {
    // Tryb bazowania (szukania zera)
    if (isSensorActive(homePin)) {
      m.position = 0;
      m.target = 0;
      m.isHoming = false;
      m.moving = false;
      stopMotor(m);
      return;
    }
    stepOnce(m);
  } else {
    // Normalny tryb ze sprawdzaniem krańcówek (Hard Limits)
    if (m.dir == -1 && isSensorActive(homePin)) {
      m.position = 0; // Auto-korekta zera jeśli uderzono w HOME
      m.target = 0;
      m.moving = false;
      stopMotor(m);
      return;
    }
    if (m.dir == 1 && isSensorActive(extPin)) {
      m.target = m.position; // Twardy stop
      m.moving = false;
      stopMotor(m);
      return;
    }

    // Dojechanie do celu
    if (m.position == m.target) {
      m.moving = false;
      stopMotor(m);
      return;
    }
    stepOnce(m);
  }
}

/* ---------------- AXIS WRAPPERS ---------------- */

void movePusher(long relativeSteps){
  long basePos = pusher.moving ? pusher.target : pusher.position;
  long newTarget = basePos + relativeSteps;
  if(newTarget < AXIS_MIN) newTarget = AXIS_MIN;
  if(newTarget > PUSHER_MAX) newTarget = PUSHER_MAX;
  moveTo(pusher, newTarget);
}

void moveLimiter(long relativeSteps){
  long basePos = limiter.moving ? limiter.target : limiter.position;
  long newTarget = basePos + relativeSteps;
  if(newTarget < AXIS_MIN) newTarget = AXIS_MIN;
  if(newTarget > LIMITER_MAX) newTarget = LIMITER_MAX;
  moveTo(limiter, newTarget);
}

/* ---------------- SERVO ---------------- */

void moveServoSmooth(int target){
  if(target > servoPos){
    for(int p=servoPos; p<=target; p+=servoStep){
      servo.write(p);
      server.handleClient(); // Nie blokuj serwera!
      delay(5);
    }
  } else {
    for(int p=servoPos; p>=target; p-=servoStep){
      servo.write(p);
      server.handleClient();
      delay(5);
    }
  }
  servoPos = target;
}

/* ---------------- TEST CYCLE ---------------- */

void testCycle(){
  moveServoSmooth(maxAngle);
  delay(300);
  moveServoSmooth(minAngle);
  delay(300);

  moveTo(pusher, PUSHER_MAX);
  while(pusher.moving){
    server.handleClient(); // Podtrzymanie WWW
    if(millis() - lastStep >= stepDelay){
      lastStep = millis();
      handleMotion(pusher, IR_Pusher_Home, IR_Pusher_Ext);
    }
  }

  moveTo(pusher, AXIS_MIN);
  while(pusher.moving){
    server.handleClient();
    if(millis() - lastStep >= stepDelay){
      lastStep = millis();
      handleMotion(pusher, IR_Pusher_Home, IR_Pusher_Ext);
    }
  }
}

/* ---------------- SETUP ---------------- */

void setup(){
  Serial.begin(115200);

  initStepper(pusher, PINS_PUSHER[0], PINS_PUSHER[1], PINS_PUSHER[2], PINS_PUSHER[3]);
  initStepper(limiter, PINS_LIMITER[0], PINS_LIMITER[1], PINS_LIMITER[2], PINS_LIMITER[3]);

  servo.setPeriodHertz(50);
  servo.attach(pinServo,500,2400);
  servo.write(minAngle);
  servoPos = minAngle;

  pinMode(IR_Pusher_Home, INPUT);
  pinMode(IR_Pusher_Ext, INPUT);
  pinMode(IR_Limiter_Home, INPUT);
  pinMode(IR_Limiter_Ext, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n" + WiFi.localIP().toString());

/* ---------------- WEB ENDPOINTS ---------------- */

  if(!LittleFS.begin(true)){
    Serial.println("LittleFS Mount Failed");
  } else {
    Serial.println("LittleFS mounted");
  }

  server.on("/", [](){
    File f = LittleFS.open("/debug_index.html","r");
    if(!f){ server.send(500, "text/plain", "Index not found"); return; }
    server.streamFile(f, "text/html");
    f.close();
  });

  server.serveStatic("/debug_style.css", LittleFS, "/debug_style.css");
  server.serveStatic("/debug_script.js", LittleFS, "/debug_script.js");

  server.on("/pusher/move",[](){
    movePusher(server.arg("steps").toInt());
    server.send(200);
  });
  server.on("/pusher/full",[](){
    moveTo(pusher, PUSHER_MAX);
    server.send(200);
  });
  server.on("/pusher/home",[](){
    startHoming(pusher);
    server.send(200);
  });

  server.on("/limiter/move",[](){
    moveLimiter(server.arg("steps").toInt());
    server.send(200);
  });
  server.on("/limiter/full",[](){
    moveTo(limiter, LIMITER_MAX);
    server.send(200);
  });
  server.on("/limiter/home",[](){
    startHoming(limiter);
    server.send(200);
  });

  server.on("/mag/open",[](){ moveServoSmooth(maxAngle); server.send(200); });
  server.on("/mag/close",[](){ moveServoSmooth(minAngle); server.send(200); });
  server.on("/cycle",[](){ testCycle(); server.send(200); });

  server.on("/sensors",[](){
    String json = "{";
    json += "\"pusherHome\":" + String(digitalRead(IR_Pusher_Home)) + ",";
    json += "\"pusherExt\":" + String(digitalRead(IR_Pusher_Ext)) + ",";
    json += "\"pusherMoving\":" + String(pusher.moving ? 1 : 0) + ",";
    json += "\"limiterHome\":" + String(digitalRead(IR_Limiter_Home)) + ",";
    json += "\"limiterExt\":" + String(digitalRead(IR_Limiter_Ext)) + ",";
    json += "\"limiterMoving\":" + String(limiter.moving ? 1 : 0);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

/* ---------------- LOOP ---------------- */

void loop(){
  server.handleClient();

  if(millis() - lastStep >= stepDelay){
    lastStep = millis();
    handleMotion(pusher, IR_Pusher_Home, IR_Pusher_Ext);
    handleMotion(limiter, IR_Limiter_Home, IR_Limiter_Ext);
  }
}