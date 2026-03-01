#include "StepperMotor.h"

const int steps[4][4] = {
  {1,0,0,1},
  {1,1,0,0},
  {0,1,1,0},
  {0,0,1,1}
};

void StepperMotor::init(int p1,int p2,int p3,int p4){

    pins[0]=p1;
    pins[1]=p2;
    pins[2]=p3;
    pins[3]=p4;

    for(int i=0;i<4;i++)
        pinMode(pins[i],OUTPUT);
}

void StepperMotor::moveTo(long target){

    if(target==position) return;

    targetPos=target;
    dir=(target>position)?1:-1;
    moving=true;
}

void StepperMotor::update(){

    if(!moving) return;

    if(position==targetPos){
        moving=false;
        return;
    }

    stepIndex+=dir;

    if(stepIndex>3) stepIndex=0;
    if(stepIndex<0) stepIndex=3;

    for(int i=0;i<4;i++)
        digitalWrite(pins[i],steps[stepIndex][i]);

    position+=dir;
}

long StepperMotor::getPosition(){
    return position;
}