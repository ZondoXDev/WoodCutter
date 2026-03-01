#include <ESP32Servo.h>

class ServoAxis {
public:
    void init(int pin,int minA,int maxA);
    void moveTo(int angle);
    void update();

private:
    Servo servo;
    int current;
    int target;
    int minAngle;
    int maxAngle;
};