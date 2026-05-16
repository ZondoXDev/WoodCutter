#include "Machine.h"

void Machine::init() {
    
    web.begin();
    pusherMotor.init(25,26,27,14);
    limiterMotor.init(18,19,21,22);

    sawServo.init(12,20,75);

    state = IDLE;
}

void Machine::startCut() {
    state = CUT_DOWN;
    sawServo.moveTo(75);
}

void Machine::update() {

    web.update();

    if(web.startRequested()) {
        startCut();
    }

    if(web.stopRequested()) {
        stop();
    }

    pusherMotor.update();
    limiterMotor.update();
    sawServo.update();
}

void Machine::stop() {
    // zatrzymujemy silniki i serwa
}