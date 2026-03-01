#include "ServoAxis.h"
void ServoAxis::init(int pin,int minA,int maxA) {
    minAngle = minA;
    maxAngle = maxA;

    servo.setPeriodHertz(50);
    servo.attach(pin,500,2400);

    current = minAngle;
    target  = minAngle;

    servo.write(current);
}

void ServoAxis::moveTo(int angle) {
    target = constrain(angle,minAngle,maxAngle);
}

void ServoAxis::update() {
    if(current == target) return;

    if(current < target) current++;
    else current--;

    servo.write(current);
}