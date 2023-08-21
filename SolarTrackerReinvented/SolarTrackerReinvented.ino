/* .+

.context    : mysoltrk - a solar tracker, reinvented
.title      : Solar Tracker application
.kind       : C Source
.author     : Fulvio Alessio <mysoltrk@gmail.com>
.creation   : 2023 July
.copyright  : (C) 2023 Fulvio Alessio
.license    : CC-BY-NC-4.0

.description

  This is a working example of solar tracker based on mysoltrk hardware disposition

.- */

#include "config.h"
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>

unsigned int shuntValues[MAX_SAMPLES];
unsigned int shuntPointer;
unsigned int VRefValues[MAX_SAMPLES];
unsigned int VRefPointer;
bool workIsCompleted = false;

/*
 * Variables used for photoresistors
 */
unsigned int phUp = 0, phDown = 0, phLeft = 0, phRight = 0, phMedium = 0;

/*
 * Variables used for movements
 * end of stroke flags. Useful for move calculations
 * t/b = top/bottom l/r = left/right
 */
bool moveUp = false, moveDown = false, moveEast = false, moveWest = false;
bool eos_bl = false, eos_br = false, eos_tl = false, eos_tr = false;

/*
 * functions definition
 */
bool move(int pin, unsigned long milliseconds, unsigned int shuntMaxValue);
void resetShuntValues();
void resetShuntVRef();
unsigned int getShunt();
unsigned int getVRef();
void sleepForMilliseconds(long value);
bool isLowVREF();
bool isShunt(unsigned int shuntMaxValue);
void readPhotoresistorValuesAndEvaluateMovements();
void movePanel();
void processMosfet();
void debugMovements(String s);

/*
 * SETUP
 */
void setup()
{
    delay(2000);
    randomSeed(analogRead(0));

    Serial.begin(9600);
    Serial.println();
    Serial.println("mysoltrk - a solar tracker, reinvented - (C) 2023 Fulvio Alessio <mysoltrk@gmail.com>");
    Serial.println("Solar Tracker Application");

    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(ACTUATOR_R_PIN1, OUTPUT);
    pinMode(ACTUATOR_R_PIN2, OUTPUT);
    pinMode(ACTUATOR_L_PIN1, OUTPUT);
    pinMode(ACTUATOR_L_PIN2, OUTPUT);

    pinMode(SHUNT_PIN, INPUT);
    pinMode(VREF_PIN, INPUT);
    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW);

    pinMode(PHOTORESISTOR_DRIVER, OUTPUT);
    digitalWrite(PHOTORESISTOR_DRIVER, LOW);

    pinMode(PHOTORESISTOR_PIN1, INPUT);
    pinMode(PHOTORESISTOR_PIN2, INPUT);
    pinMode(PHOTORESISTOR_PIN3, INPUT);
    pinMode(PHOTORESISTOR_PIN4, INPUT);

    digitalWrite(ACTUATOR_R_PIN1, LOW);
    digitalWrite(ACTUATOR_R_PIN2, LOW);
    digitalWrite(ACTUATOR_L_PIN1, LOW);
    digitalWrite(ACTUATOR_L_PIN2, LOW);

    Serial.println("Pin setup done");
    delay(1000);
}

/*
 * LOOP
 */
void loop()
{
    unsigned long startedAt = millis();

    workIsCompleted = false;
    readPhotoresistorValuesAndEvaluateMovements();

    Serial.println("Tracker start");
    while (!workIsCompleted && !isLowVREF())
    {
        movePanel();
        if (millis() - startedAt > MAX_MILLISECONDS_WORK)
        {
            Serial.println("Exceeded work time limit");
            workIsCompleted = true;
        }
        else
        {
            readPhotoresistorValuesAndEvaluateMovements();
        }
        delay(10);
    }
    Serial.println("Tracker complete");
    processMosfet();
    sleepForMilliseconds(SLEEP_DELAY);
}

/*
 *  Check if we are in low voltage condition
 */
bool isLowVREF()
{
    unsigned int vref = getVRef();

    if (vref < MIN_VREF_VALUE)
    {
        Serial.print("VREF ALARM: ");
        Serial.print(vref);
        Serial.print("<");
        Serial.println(MIN_VREF_VALUE);
        return true;
    }
    else
    {
        return false;
    }
}

/*
 *  Check if we are in shunt condition
 */
bool isShunt(unsigned int shuntMaxValue)
{
    unsigned int shunt = getShunt();

    if (shunt > shuntMaxValue)
    {
        Serial.print("SHUNT ALARM: ");
        Serial.print(shunt);
        Serial.print(">");
        Serial.print(shuntMaxValue);
        return true;
    }
    else
    {
        return false;
    }
}

/*
 *  Sleep device with a (very draft) low power implementation
 *  @TODO: use LowPower.h
 */
void sleepForMilliseconds(long value)
{

    Serial.print("Going to sleep for ~");
    Serial.print(value);
    Serial.println("ms");
    Serial.flush();

    // shutdown all
    digitalWrite(PHOTORESISTOR_DRIVER, LOW);
    digitalWrite(ACTUATOR_R_PIN1, LOW);
    digitalWrite(ACTUATOR_R_PIN2, LOW);
    digitalWrite(ACTUATOR_L_PIN1, LOW);
    digitalWrite(ACTUATOR_L_PIN2, LOW);

    // enter to low power mode (DRAFT)
    cli();
    power_adc_disable();
    power_spi_disable();
    sei();

    delay(value);

    // exit to low power mode (DRAFT)
    cli();
    power_all_enable();
    sei();

    Serial.println("Wake up!");
}

/*
 *  Do the move, aka: enable a specific pin for n seconds checking shunt & vref
 */

bool move(int pin, unsigned long milliseconds, int shuntMaxValue)
{
    unsigned long until = millis() + milliseconds;
    unsigned long ignoreSensorsUntil = millis() + IGNORE_SHUNT_VREF_FOR_;

    resetShuntValues();
    resetShuntVRef();

    Serial.println();
    Serial.print("Activating pin ");
    Serial.print(pin);
    Serial.print(" for ");
    Serial.print(milliseconds);
    Serial.println("ms");

    // engine on
    digitalWrite(pin, HIGH);

    while (millis() <= until)
    {
        if (millis() > ignoreSensorsUntil)
        {
            /*
             * We check voltage reference sensor values after ignoring the initial start engine
             */
            if (isLowVREF())
            {
                // engine off
                digitalWrite(pin, LOW);
                Serial.println();
                Serial.println("Stop now");
                delay(1000);
                return true;
            }
            /*
             * We check shunt value
             */
            if (isShunt(shuntMaxValue))
            {
                // engine off
                digitalWrite(pin, LOW);
                Serial.println();
                Serial.println("Stop now");
                delay(1000);
                return false;
            }
        }
        else {
            // load data into buffer, ignore return values
            getShunt();
            getVRef();
        }
        delay(10);
    }

    // All given time has passed, engine off
    digitalWrite(pin, LOW);

    Serial.println("Move done");
    return true;
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

/*
 *  get shunt value
 */
unsigned int getShunt()
{
unsigned int value,result,tmp=0;
int i;

    for (i = 1; i < MAX_SAMPLES; i++)
    {
        tmp += analogRead(SHUNT_PIN);
        delay(1);
    }
    // get medium value
    value = tmp / MAX_SAMPLES;

    if (++shuntPointer > MAX_SAMPLES)
    {
        // queue is full. Shift values
        for (i = 1; i < MAX_SAMPLES; i++)
        {
            shuntValues[i - 1] = shuntValues[i];
        }
        shuntValues[MAX_SAMPLES - 1] = value;
        shuntPointer = MAX_SAMPLES;
    }
    else
    {
        shuntValues[shuntPointer - 1] = value;
    }

    // calculate medium value
    value = 0;
    for (unsigned int i = 0; i < shuntPointer; i++)
    {
        value += shuntValues[i];
    }

    result = value / shuntPointer;

#ifdef MORE_DEBUG
    Serial.print("SHUNT=");
    Serial.print(result);
    Serial.print(" shuntPointer ");
    Serial.print(shuntPointer);
    Serial.println();
#endif
    return result;
}

/*
 *  get VRef value
 */
unsigned int getVRef()
{
unsigned int value,result,tmp=0;
int i;

    for (i = 1; i < MAX_SAMPLES; i++)
    {
        tmp += analogRead(VREF_PIN);
        delay(1);
    }
    // get medium value
    value = tmp / MAX_SAMPLES;

    if (++VRefPointer > MAX_SAMPLES)
    {
        // queue is full. Shift values
        for (i = 1; i < MAX_SAMPLES; i++)
        {
            VRefValues[i - 1] = VRefValues[i];
        }
        VRefValues[MAX_SAMPLES - 1] = value;
        VRefPointer = MAX_SAMPLES;
    }
    else
    {
        VRefValues[VRefPointer - 1] = value;
    }

    // calculate medium value
    value = 0;
    for (unsigned int i = 0; i < VRefPointer; i++)
    {
        value += VRefValues[i];
    }

    result = value / VRefPointer;
 #ifdef MORE_DEBUG
    Serial.print(" VREF=");
    Serial.print(result);
    Serial.print(" VRefPointer ");
    Serial.print(VRefPointer);
    Serial.println();
#endif
    return result;
}

/*
 *  read photoresistor values and calculate movements
 */
void readPhotoresistorValuesAndEvaluateMovements()
{
    int diffVertical, diffHorizontal;

    moveUp   = false;
    moveDown = false;
    moveEast = false;
    moveWest = false;

    digitalWrite(PHOTORESISTOR_DRIVER, HIGH);

    delay(5);
    phUp    = analogRead(PHOTORESISTOR_PIN1);
    phRight = analogRead(PHOTORESISTOR_PIN2);
    phLeft  = analogRead(PHOTORESISTOR_PIN3);
    phDown  = analogRead(PHOTORESISTOR_PIN4);

    digitalWrite(PHOTORESISTOR_DRIVER, LOW);

#ifdef MORE_DEBUG
    Serial.print("readPhotoresistorValues **");
    Serial.print(" up: ");
    Serial.print(phUp);
    Serial.print(" down: ");
    Serial.print(phDown);
    Serial.print(" right: ");
    Serial.print(phRight);
    Serial.print(" left: ");
    Serial.print(phLeft);
    Serial.println();
#endif

    phMedium = (phUp + phDown + phLeft + phRight) / 4;

    if (phMedium < LIGHT_THREESHOLD)
    {
        Serial.println("Low light");
        workIsCompleted = true;
        return;
    }

    // calculate differences
    diffVertical = phUp - phDown;
    diffHorizontal = phRight - phLeft;

#ifdef MORE_DEBUG
    Serial.print("Differences **");
    Serial.print(" Horizontal: ");
    Serial.print(diffHorizontal);
    Serial.print(" Vertical: ");
    Serial.print(diffVertical);
    Serial.println();
#endif

    if (abs(diffHorizontal) > PHOTORESISTOR_THREESHOLD)
    {
        Serial.println("Evaluate RIGHT/LEFT");
        if (phRight < phLeft)
        {
            moveWest = true;
        }
        else
        {
            moveEast = true;
        }
    }

    debugMovements("Evaluate 1st");

    // check end of stroke behaviors
    if (moveWest && eos_tr && eos_bl)
    {
        moveWest = false;
    }
    if (moveEast && eos_br && eos_tl)
    {
        moveEast = false;
    }

    debugMovements("Evaluate 2nd");

    if (!moveWest && !moveEast)
    {
        // evaluate vertical movements only when panel is correctly positioned horizontally 
        Serial.println("Evaluate UP/DOWN");
        if (abs(diffVertical) > PHOTORESISTOR_THREESHOLD)
        {
            if (phUp < phDown)
            {
                moveDown = true;
            }
            else
            {
                moveUp = true;
            }
        }
    }

    debugMovements("Evaluate 3rd");

    // check end of stroke behaviors
    if (moveDown && (eos_bl || eos_br))
    {
        moveDown = false;
    }
    if (moveUp && (eos_tl || eos_tr))
    {
        moveUp = false;
    }

    debugMovements("Evaluate 4th");

    // check other contrasting behaviors
    if (moveUp && moveDown)
    {
        moveUp = false;
        moveDown = false;
    }
    if (moveEast && moveWest)
    {
        moveEast = false;
        moveWest = false;
    }

    debugMovements("Evaluate 5th");

    // the work is complete when there are no further movements
    workIsCompleted = !moveUp && !moveDown && !moveWest && !moveEast;
}

void debugMovements(String s)
{
#ifdef MORE_DEBUG
    Serial.print(s);
    Serial.print(" ");
    Serial.print(">>> MOVES:");
    Serial.print(" moveUp: ");
    Serial.print(moveUp);
    Serial.print(" moveDown: ");
    Serial.print(moveDown);
    Serial.print(" moveWest: ");
    Serial.print(moveWest);
    Serial.print(" moveEast: ");
    Serial.print(moveEast);

    Serial.print(" EOF STROKE:");
    Serial.print(" topRight: ");
    Serial.print(eos_tr);
    Serial.print(" topLeft: ");
    Serial.print(eos_tl);
    Serial.print(" bottomRight: ");
    Serial.print(eos_br);
    Serial.print(" bottomLeft: ");
    Serial.print(eos_bl);
    Serial.println();
#endif
}

/*
 *  transform calculated movements to motor movements, check and apply end of stroke behaviors
 */
void movePanel()
{
bool done=false;
    Serial.print("movePanel: ");

    if (moveUp)
    {
        Serial.println("move UP ^^");
        eos_br = false;
        if (!move(ACTUATOR_R_PIN1, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_R))
        {
            eos_tr = true;
        }
        eos_bl = false;
        if (!move(ACTUATOR_L_PIN1, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_L))
        {
            eos_tl = true;
        }
    }
    if (moveDown)
    {
        Serial.println("move DOWN vv");
        eos_tr = false;
        if (!move(ACTUATOR_R_PIN2, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_R))
        {
            eos_br = true;
        }
        eos_tl = false;
        if (!move(ACTUATOR_L_PIN2, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_L))
        {
            eos_bl = true;
        }
    }
    if (moveWest)
    {
        Serial.println("move to WEST >>");
        if (random(0,3) && !eos_tr)
        {
            eos_br = false;
            if (!move(ACTUATOR_R_PIN1, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_R))
            {
                eos_tr = true;
            }
            else
            {
                done=true;
            }
        }
        if (!done && !eos_bl)
        {
            eos_tl = false;
            if (!move(ACTUATOR_L_PIN2, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_L))
            {
                eos_bl = true;
            }
        }
    }
    if (moveEast)
    {
        Serial.println("move to EAST <<");
        if (random(0,3) && !eos_tl)
        {
            eos_bl = false;
            if (!move(ACTUATOR_L_PIN1, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_L))
            {
                eos_tl = true;
            }
            else
            {
                done=true;
            }
        }
        if (!done && !eos_br)
        {
            eos_br = false;
            if (!move(ACTUATOR_R_PIN2, MAX_MILLISECONDS_MOVEMENT, MAX_SHUNT_VALUE_R))
            {
                eos_br = true;
            }
        }
    }
}

/*
 *  evaluate if we can turn on mosfet (load output)
 */
void processMosfet()
{

    Serial.print("processMosfet: ");
    if (phMedium >= LIGHT_THREESHOLD && !isLowVREF())
    {
        // light on
        digitalWrite(MOSFET_PIN, HIGH);
    }
    else
    {
        // light off
        digitalWrite(MOSFET_PIN, LOW);
    }
    Serial.print(digitalRead(MOSFET_PIN) ? "ON" : "OFF");
    Serial.println();
}