/* .+

.context    : mysoltrk - a solar tracker, reinvented
.title      : Actuators movements and limit switch/VRef simulation test
.kind       : C Source
.author     : Fulvio Alessio <mysoltrk@gmail.com>
.creation   : 2023 June
.copyright  : (C) 2023 Fulvio Alessio
.license    : CC-BY-NC-4.0

.description
  This application is an example to test actuators with shunt and/or VRef condition

  How to test:
  - shunt condition: leave actuator to hit top or bottom
  - VRef: reduce external power supply < ~6V
.- */

#include <Arduino.h>
#include "config.h"

unsigned int shuntValues[MAX_SAMPLES];
unsigned int shuntPointer;
unsigned int VRefValues[MAX_SAMPLES];
unsigned int VRefPointer;

void move(int pin, int seconds, int shuntMaxValue=MAX_SHUNT_VALUE);
void resetShuntValues();
void resetShuntVRef();
unsigned int getShunt();
unsigned int getVRef();

void setup()
{
    delay(2000);

    Serial.begin(9600);
    Serial.println();
    Serial.println("mysoltrk - a solar tracker, reinvented - (C) 2023 Fulvio Alessio <mysoltrk@gmail.com>");
    Serial.println("Actuators movements and limit switch simulation test");

    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(ACTUATOR_R_PIN1, OUTPUT);
    pinMode(ACTUATOR_R_PIN2, OUTPUT);
    pinMode(ACTUATOR_L_PIN1, OUTPUT);
    pinMode(ACTUATOR_L_PIN2, OUTPUT);

    pinMode(SHUNT_PIN, INPUT);
    pinMode(VREF_PIN, INPUT);

    digitalWrite(ACTUATOR_R_PIN1, LOW);
    digitalWrite(ACTUATOR_R_PIN2, LOW);
    digitalWrite(ACTUATOR_L_PIN1, LOW);
    digitalWrite(ACTUATOR_L_PIN2, LOW);

    Serial.println("Pin setup done");
    delay(1000);

    Serial.println("Move all actuators to zero position");

    move(ACTUATOR_R_PIN2, MAX_SECONDS_MOVEMENT);
    delay(2000);
    move(ACTUATOR_L_PIN2, MAX_SECONDS_MOVEMENT);

    Serial.println("Setup complete");
    delay(5000);

}

void loop()
{
    Serial.println("Extend right actuator to max");
    move(ACTUATOR_R_PIN1, MAX_SECONDS_MOVEMENT);
    delay(2000);

    Serial.println("Extend left actuator to max");
    move(ACTUATOR_L_PIN1, MAX_SECONDS_MOVEMENT);
    delay(2000);

    Serial.println("Move right actuator to zero position");
    move(ACTUATOR_R_PIN2, MAX_SECONDS_MOVEMENT);
    delay(2000);

    Serial.println("Move left actuator to zero position");
    move(ACTUATOR_L_PIN2, MAX_SECONDS_MOVEMENT, MAX_SHUNT_VALUE);
    delay(2000);

}

/*
 *  Do the move, aka: enable a specific pin for n seconds checking shunt & vref
 */

void move(int pin, int seconds, int shuntMaxValue)
{
unsigned long until = millis() + (unsigned long)(seconds * 1000L);
unsigned long ignoreShuntUntil = millis() + IGNORE_SHUNT_VREF_FOR_;
unsigned int shunt = 0, vref = 0;
bool dot = false;

    resetShuntValues();
    resetShuntVRef();

    Serial.println();
    Serial.print("Activating pin ");
    Serial.print(pin);
    Serial.print(" for ");
    Serial.print(seconds);
    Serial.println(" seconds");
    digitalWrite(pin, HIGH);
    while (millis() <= until)
    {
        shunt = getShunt();
        vref = getVRef();
        if (millis() > ignoreShuntUntil)
        {
            /*
             * We check sensor values after ignoring the initial start engine due to higher current absorption
             */
            if (vref < MIN_VREF_VALUE)
            {
                digitalWrite(pin, LOW);
                Serial.print("VREF ALARM: ");
                Serial.print(vref);
                Serial.print("<");
                Serial.print(MIN_VREF_VALUE);
                Serial.println(". Stop now");
                delay(500);
                return;
            }
            /*
             * We check shunt value
             */
            if (shunt > shuntMaxValue)
            {
                digitalWrite(pin, LOW);
                Serial.print("SHUNT ALARM: ");
                Serial.print(shunt);
                Serial.print(">");
                Serial.print(shuntMaxValue);
                Serial.println(". Stop now");
                delay(500);
                return;
            }
        }

        // Draw a dot every second
        if (millis() % 1000 == 0)
        {
            if (!dot)
            {
                Serial.print(".");
                dot = true;
            }
        }
        else
        {
            dot = false;
        }
    }
    // All given time has passed
    digitalWrite(pin, LOW);
    Serial.println(" time out.");
}

/*
 *  set shunt samples to 0, initialize pointer
 */
void resetShuntValues()
{
    for (int i = 0; i > MAX_SAMPLES; i++)
    {
        shuntValues[i] = 0;
    }
    shuntPointer = 0;
}

/*
 *  set VRef samples to 0, initialize pointer
 */
void resetShuntVRef()
{
    for (int i = 0; i > MAX_SAMPLES; i++)
    {
        VRefValues[i] = 0;
    }
    VRefPointer = 0;
}

unsigned int getShunt()
{
    unsigned int value = analogRead(SHUNT_PIN);

    if (++shuntPointer > MAX_SAMPLES)
    {
        // queue is full. Shift values
        for (int i = 1; i < MAX_SAMPLES; i++)
        {
            shuntValues[i - 1] = shuntValues[i];
        }
        shuntValues[MAX_SAMPLES - 1] = value;
        shuntPointer = MAX_SAMPLES;
    }

    // calculate medium value
    value = 0;
    for (int i = 0; i < shuntPointer; i++)
    {
        value += shuntValues[i];
    }

    return value / shuntPointer;
}

unsigned int getVRef()
{
    unsigned int value = analogRead(VREF_PIN);

    if (++VRefPointer > MAX_SAMPLES)
    {
        // queue is full. Shift values
        for (int i = 1; i < MAX_SAMPLES; i++)
        {
            VRefValues[i - 1] = VRefValues[i];
        }
        VRefValues[MAX_SAMPLES - 1] = value;
        VRefPointer = MAX_SAMPLES;
    }

    // calculate medium value
    value = 0;
    for (int i = 0; i < VRefPointer; i++)
    {
        value += VRefValues[i];
    }

    return value / VRefPointer;
}
