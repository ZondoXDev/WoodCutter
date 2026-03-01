#include <Arduino.h>
#include "machine/Machine.h"

Machine woodCutter;

void setup() {

    Serial.begin(115200);
    woodCutter.init();
}

void loop() {

    woodCutter.update();
}