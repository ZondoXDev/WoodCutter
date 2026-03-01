#pragma once

#include "motion/StepperMotor.h"
#include "motion/ServoAxis.h"

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

private:
    MachineState state;

    StepperMotor motorL;
    StepperMotor motorR;
    ServoAxis sawServo;
};