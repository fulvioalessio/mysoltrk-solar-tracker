#ifndef __AVR__
    #error Tested for AVR only
#endif

// set it to obtain more debug
#define MORE_DEBUG

// Arduino nano pins ( https://github.com/fulvioalessio/mysoltrk-solar-tracker/blob/main/assets/images/mysoltrk_bb.jpg )
#define ACTUATOR_R_PIN1             4       // R+
#define ACTUATOR_R_PIN2             7       // R-
#define ACTUATOR_L_PIN1             6       // L+
#define ACTUATOR_L_PIN2             5       // L-
#define SHUNT_PIN                   18
#define VREF_PIN                    21
#define PHOTORESISTOR_DRIVER        12
#define PHOTORESISTOR_PIN1          14
#define PHOTORESISTOR_PIN2          15
#define PHOTORESISTOR_PIN3          16
#define PHOTORESISTOR_PIN4          17
#define MOSFET_PIN                  8

// Program params
#define MAX_SAMPLES                 10      // to improve the quality of analog readings
#define PHOTORESISTOR_THREESHOLD    2       // difference between opposite photoresistor to force move calculation
#define IGNORE_SHUNT_VREF_FOR_      500L    // in milliseconds
#define MAX_MILLISECONDS_MOVEMENT   800L    // max milliseconds to move actuator. must be greater than IGNORE_SHUNT_VREF_FOR_
#define MAX_MILLISECONDS_WORK       60000L  // max seconds of activity per session
#define SLEEP_DELAY                 60000L  // sleep time

// External hardware params
#define MIN_VREF_VALUE              80      // below this value the external power supply (eg: solar panel) supplies too little energy
#define LIGHT_THREESHOLD            700     // threeshold to enable movements and to turn on or off mosfet pin (load)
#define MAX_SHUNT_VALUE_R           30      // threshold for right motor
#define MAX_SHUNT_VALUE_L           35      // threshold for left motor. Left motor seem to do higher strain compared to right one
