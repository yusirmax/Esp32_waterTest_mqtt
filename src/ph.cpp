#include <Arduino.h>
#include "ph.h"
#include "pin.h"

unsigned long int avgValue; // Store the average value of the sensor feedback
void Get_Ph_Value()
{
    int buf[10];                 // buffer for read analog
    for (int i = 0; i < 10; i++) // Get 10 sample value from the sensor for smooth the value
    {
        buf[i] = analogRead(PhSensorPin);
        delay(10);
    }
    for (int i = 0; i < 9; i++) // sort the analog from small to large
    {
        for (int j = i + 1; j < 10; j++)
        {
            if (buf[i] > buf[j])
            {
                int temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }
    avgValue = 0;
    for (int i = 2; i < 8; i++) // take the average value of 6 center sample
        avgValue += buf[i];
    float phValue = (float)avgValue * 5.0 / 1024 / 6; // convert the analog into millivolt
    phValue = 3.5 * phValue + Offset;                 // convert the millivolt into pH value
    Serial.print("    pH:");
    Serial.print(phValue, 2);
    Serial.println(" ");
}