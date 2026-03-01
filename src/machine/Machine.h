#pragma once
#include <motion/StepperMotor.h>
#include <motion/ServoAxis.h>
#include <web/WebServer.h>


enum MachineState {
    IDLE,
    CUT_DOWN,
    CUT_UP
};

class Machine {
public:
    void init();
    void update();
    void startCut();
    void stop(); 

private:
    MachineState state;

    StepperMotor motorL;
    StepperMotor motorR;
    ServoAxis sawServo;
    WebInterface web;
};