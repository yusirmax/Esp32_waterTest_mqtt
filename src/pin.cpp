#include "pin.h"
#include <Arduino.h>

void pin_init()
{
    pinMode(TdsSensorPin, INPUT);
    pinMode(PhSensorPin, OUTPUT);
    Serial.println("引脚初始化完成");
}