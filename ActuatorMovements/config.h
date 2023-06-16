#ifndef __AVR__
    #error Code in here tested only for AVR
#endif

// Arduino nano pins
#define ACTUATOR_R_PIN1             4
#define ACTUATOR_R_PIN2             5
#define ACTUATOR_L_PIN1             6
#define ACTUATOR_L_PIN2             7
#define SHUNT_PIN                   18
#define VREF_PIN                    21

// Program params
#define MAX_SAMPLES                 10
#define IGNORE_SHUNT_VREF_FOR_      1000L   // milliseconds
#define MAX_SHUNT_VALUE             20      // threshold
#define MIN_VREF_VALUE              100     // below this value the external power supply (eg: solar panel) supplies too little energy
#define MAX_SECONDS_MOVEMENT        180     // max seconds to move actuator
