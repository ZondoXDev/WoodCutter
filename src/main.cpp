#include <Arduino.h>
#include "machine/Machine.h"

Machine machine;

void setup() {

    Serial.begin(115200);
    machine.init();
}

void loop() {

    machine.update();
}