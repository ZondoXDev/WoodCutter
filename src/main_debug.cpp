#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "web/secrets.h"

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
};

Stepper pusher;
Stepper limiter;

/* ---------------- SERVO ---------------- */

Servo servo;

const int pinServo = 12;
int minAngle = 20;
int maxAngle = 75;

int servoPos = minAngle;
const int servoStep = 1;

/* ---------------- LIMITS ---------------- */

const long PUSHER_MIN = 0;
const long PUSHER_MAX = 4200;

const long LIMITER_MIN = 0;
const long LIMITER_MAX = 4200;

/* ---------------- SYSTEM ---------------- */

unsigned long lastStep = 0;
const int stepDelay = 3;

/* ---------------- STEPPER INIT ---------------- */

void initStepper(Stepper &m, int p1,int p2,int p3,int p4){

  m.pins[0] = p1;
  m.pins[1] = p2;
  m.pins[2] = p3;
  m.pins[3] = p4;

  m.stepIndex = 0;
  m.position = 0;
  m.target = 0;
  m.moving = false;
  m.dir = 0;

  for(int i=0;i<4;i++)
    pinMode(m.pins[i], OUTPUT);
}

/* ---------------- MOTOR CONTROL ---------------- */

void stopMotor(Stepper &m){

  for(int i=0;i<4;i++)
    digitalWrite(m.pins[i], LOW);

}

void stepOnce(Stepper &m){

  m.stepIndex += m.dir;

  if(m.stepIndex > 3) m.stepIndex = 0;
  if(m.stepIndex < 0) m.stepIndex = 3;

  for(int i=0;i<4;i++)
    digitalWrite(m.pins[i], steps[m.stepIndex][i]);

  m.position += m.dir;

}

void moveTo(Stepper &m,long target){

  if(target == m.position) return;

  m.target = target;
  m.dir = (target > m.position) ? 1 : -1;
  m.moving = true;

  Serial.print("MOVE ");
  Serial.print(m.position);
  Serial.print(" -> ");
  Serial.println(target);
}

void handleMotion(Stepper &m){ 

  if(!m.moving) return;

  if(m.position == m.target){
    m.moving = false;
    stopMotor(m);
    return;
  }

  stepOnce(m);

}

/* ---------------- AXIS WRAPPERS ---------------- */

void movePusher(long target){

  if(target < PUSHER_MIN) target = PUSHER_MIN;
  if(target > PUSHER_MAX) target = PUSHER_MAX;

  moveTo(pusher,target);

}

void moveLimiter(long target){

  if(target < LIMITER_MIN) target = LIMITER_MIN;
  if(target > LIMITER_MAX) target = LIMITER_MAX;

  moveTo(limiter,target);

}

/* ---------------- SERVO ---------------- */

void moveServoSmooth(int target){

  if(target > servoPos){

    for(int p=servoPos; p<=target; p+=servoStep){
      servo.write(p);
      delay(5);
    }

  }else{

    for(int p=servoPos; p>=target; p-=servoStep){
      servo.write(p);
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

  movePusher(PUSHER_MAX);

  while(pusher.moving){
    handleMotion(pusher);
    delay(stepDelay);
  }

  movePusher(PUSHER_MIN);

  while(pusher.moving){
    handleMotion(pusher);
    delay(stepDelay);
  }

}

/* ---------------- WEB PAGE ---------------- */

const char PAGE[] PROGMEM = R"rawliteral(

<html>
<head>
<meta name="viewport" content="width=device-width">
<style>

button,input{
width:180px;
height:45px;
font-size:16px;
margin:6px;
}

</style>
</head>

<body>

<h2>WOOD CUTTER TEST</h2>

<h3>Pusher</h3>

<button onclick="fetch('/pusher/move?steps=50')">+50</button>
<button onclick="fetch('/pusher/move?steps=-50')">-50</button>

<br>

<button onclick="fetch('/pusher/full')">FULL</button>
<button onclick="fetch('/pusher/home')">HOME</button>

<hr>

<h3>Limiter</h3>

<button onclick="fetch('/limiter/move?steps=50')">+50</button>
<button onclick="fetch('/limiter/move?steps=-50')">-50</button>

<br>

<button onclick="fetch('/limiter/full')">FULL</button>
<button onclick="fetch('/limiter/home')">HOME</button>

<hr>

<h3>Magazine</h3>

<button onclick="fetch('/mag/open')">OPEN</button>
<button onclick="fetch('/mag/close')">CLOSE</button>

<hr>

<h3>Test</h3>

<button onclick="fetch('/cycle')">TEST CYCLE</button>

</body>
</html>

)rawliteral";

/* ---------------- SETUP ---------------- */

void setup(){

  Serial.begin(115200);

  initStepper(pusher,25,26,27,14);
  initStepper(limiter,18,19,21,22);

  servo.setPeriodHertz(50);
  servo.attach(pinServo,500,2400);

  servo.write(minAngle);
  servoPos = minAngle;

  WiFi.begin(WIFI_SSID,WIFI_PASS);

  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println(WiFi.localIP());

/* ---------------- WEB ---------------- */

  server.on("/",[](){
    server.send(200,"text/html",PAGE);
  });

/* ----- PUSHER ----- */

  server.on("/pusher/move",[](){

    int steps = server.arg("steps").toInt();
    movePusher(pusher.position + steps);

    server.send(200);

  });

  server.on("/pusher/full",[](){

    movePusher(PUSHER_MAX);
    server.send(200);

  });

  server.on("/pusher/home",[](){

    movePusher(PUSHER_MIN);
    server.send(200);

  });

/* ----- LIMITER ----- */

  server.on("/limiter/move",[](){

    int steps = server.arg("steps").toInt();
    moveLimiter(limiter.position + steps);

    server.send(200);

  });

  server.on("/limiter/full",[](){

    moveLimiter(LIMITER_MAX);
    server.send(200);

  });

  server.on("/limiter/home",[](){

    moveLimiter(LIMITER_MIN);
    server.send(200);

  });

/* ----- MAGAZINE ----- */

  server.on("/mag/open",[](){

    moveServoSmooth(maxAngle);
    server.send(200);

  });

  server.on("/mag/close",[](){

    moveServoSmooth(minAngle);
    server.send(200);

  });

/* ----- TEST ----- */

  server.on("/cycle",[](){

    testCycle();
    server.send(200);

  });

  server.begin();

}

/* ---------------- LOOP ---------------- */

void loop(){

  server.handleClient();

  if(millis() - lastStep >= stepDelay){

    lastStep = millis();

    handleMotion(pusher);
    handleMotion(limiter);

  }

}