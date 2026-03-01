#pragma once
#include <Arduino.h>

class StepperMotor {
public:
    void init(int p1,int p2,int p3,int p4);
    void moveTo(long target);
    void update();

    long getPosition();

private:
    int pins[4];

    int stepIndex = 0;
    long position = 0;
    long targetPos = 0;

    bool moving = false;
    int dir = 0;
};