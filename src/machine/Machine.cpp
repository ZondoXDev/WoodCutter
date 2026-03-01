#include "Machine.h"

void Machine::init() {

    motorL.init(18,19,21,22);
    motorR.init(25,26,27,14);

    sawServo.init(12,20,75);

    state = IDLE;
}

void Machine::startCut() {
    state = CUT_DOWN;
    sawServo.moveTo(75);
}

void Machine::update() {

    motorL.update();
    motorR.update();
    sawServo.update();

    switch(state) {

        case CUT_DOWN:
            if(/*serwo osiągnęło pozycję*/ true) {
                state = CUT_UP;
                sawServo.moveTo(20);
            }
            break;

        case CUT_UP:
            if(/*wróciło*/ true) {
                state = IDLE;
            }
            break;

        default:
            break;
    }
}