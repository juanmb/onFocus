// Moonlite-compatible stepper controller
//
// Uses AccelStepper (http://www.airspayce.com/mikem/arduino/AccelStepper/)
//
// Inspired by (http://orlygoingthirty.blogspot.co.nz/2014/04/arduino-based-motor-focuser-controller.html)
// orly.andico@gmail.com, 13 April 2014
//
// Modified for indilib, easydriver by Cees Lensink
// Added sleep function by Daniel Franzén
// Rewritten by Juan Menendez


#include <AccelStepper.h>
#include "serial_command.h"

#define MS1_PIN 2
#define MS2_PIN 3
#define ENABLE_PIN 4
#define RESET_PIN 5
#define DIR_PIN 6
#define STEP_PIN 7
#define SWITCH_PIN 10
#define LED_PIN 13

#define USE_SLEEP true  // true: use sleep pin, false: use enable pin
#define SPEED 50        // initial speed (pulses/second)
#define ACCEL 2000      // acceleration (pulse/second2)

// stepping modes
#define FULL_STEP 0
#define HALF_STEP 1
#define MS_4STEPS 2
#define MS_8STEPS 3

// period to wait before turning power off (in milliseconds)
#define ACTIVE_TIME 4000

SerialCommand sCmd;
AccelStepper stepper(1, STEP_PIN, DIR_PIN);

bool isRunning = false;
bool isPowerOn = false;
long startTime = 0;
int steppingDelay = 1; // 500 steps/second


long hexstr2long(char *line)
{
    return strtol(line, NULL, 16);
}

void turnOn()
{
    if (!isRunning) {
        digitalWrite(ENABLE_PIN, !USE_SLEEP);
        digitalWrite(LED_PIN, HIGH);
        isRunning = true;
        isPowerOn = true;
    }
}

void turnOff()
{
    digitalWrite(ENABLE_PIN, USE_SLEEP);
    digitalWrite(LED_PIN, LOW);
    isRunning = false;
    isPowerOn = false;
}

void setStepMode(uint8_t mode)
{
    digitalWrite(MS1_PIN, mode & 0x01);
    digitalWrite(MS2_PIN, mode & 0x02);
}

uint8_t getStepMode()
{
    uint8_t mode = 2*digitalRead(MS2_PIN);
    mode += digitalRead(MS1_PIN);
    return mode;
}

//Immediately stop any focus motor movement. returns nothing
//code from Quickstop example. This is blocking
void cmdCancelFocus(char *param, uint8_t len)
{
    turnOn();
    stepper.stop(); // Stop as fast as possible: sets new target
    stepper.runToPosition();
}


// Go to the new position as set by the ":SNYYYY#" command. returns nothing
// turn stepper on and flag it is running
// is this the only command that should actually make the stepper run ?
void cmdGotoPos(char *param, uint8_t len)
{
    turnOn();
}

// Returns the temperature coefficient where XX is a two-digit signed (2’s
// complement) hex number.  hardcoded
void cmdGetTempCoef(char *param, uint8_t len)
{
    Serial.print("02#");
}

// Returns the current stepping delay where XX is a two-digit unsigned hex
// number. See the :SD# command for a list of possible return values.
void cmdGetSteppingDelay(char *param, uint8_t len)
{
    char tmp[4];
    sprintf(tmp, "%02X#", steppingDelay);
    Serial.print(tmp);
}

// Returns "FF#" if the focus motor is half-stepped otherwise return "00#".
void cmdGetHalfStep(char *param, uint8_t len)
{
    if (getStepMode() == HALF_STEP)
        Serial.print("FF#");
    else
        Serial.print("00#");
}

// Returns "00#" if the focus motor is not moving, otherwise return "01#",
// AccelStepper returns Positive as clockwise
void cmdGetFocusMoving(char *param, uint8_t len)
{
    if (stepper.distanceToGo() == 0) {
        Serial.print("00#");
    } else {
        Serial.print("01#");
    }
}

// Returns the new position previously set by a ":SNYYYY" command where
// YYYY is a four-digit unsigned hex number.
void cmdGetNewPos(char *param, uint8_t len)
{
    char tmp[10];
    long pos = stepper.targetPosition();
    sprintf(tmp, "%04lX#", pos);
    Serial.print(tmp);
}

// Returns the current position where YYYY is a four-digit unsigned hex number.
void cmdGetCurrentPos(char *param, uint8_t len)
{
    char tmp[10];
    long pos = stepper.currentPosition();
    sprintf(tmp, "%04lX#", pos);
    Serial.print(tmp);
}

// Returns the current temperature where YYYY is a four-digit signed (2’s
// complement) hex number.
void cmdGetCurrentTemp(char *param, uint8_t len)
{
    Serial.print("0020#");
}

// Get the version of the firmware as a two-digit decimal number where the
// first digit is the major version number, and the second digit is the
// minor version number.  hardcoded
void cmdGetVersion(char *param, uint8_t len)
{
    Serial.print("10#");
}

// Set the new temperature coefficient where XX is a two-digit, signed (2’s
// complement) hex number.
void cmdSetTempCoef(char *param, uint8_t len)
{
    //TODO
}

//Set the new stepping delay where XX is a two-digit,unsigned hex number.
void cmdSetSteppingDelay(char *param, uint8_t len)
{
    steppingDelay = (int)hexstr2long(param);
    stepper.setMaxSpeed(500.0/(float)steppingDelay);
}

//Set full-step mode.
void cmdSetFullStep(char *param, uint8_t len)
{
    setStepMode(FULL_STEP);
}

//Set half-step mode.
void cmdSetHalfStep(char *param, uint8_t len)
{
    setStepMode(HALF_STEP);
}

//Set the new position where YYYY is a four-digit
void cmdSetNewPos(char *param, uint8_t len)
{
    turnOn();
    long pos = hexstr2long(param);
    stepper.moveTo(pos);
}

//Set the current position where YYYY is a four-digit unsigned hex number.
void cmdSetCurrentPos(char *param, uint8_t len)
{
    long pos = hexstr2long(param);
    stepper.setCurrentPosition(pos);
}


void setup()
{
    sCmd.addCommand("FQ", cmdCancelFocus);
    sCmd.addCommand("FG", cmdGotoPos);
    sCmd.addCommand("GC", cmdGetTempCoef);
    sCmd.addCommand("GD", cmdGetSteppingDelay);
    sCmd.addCommand("GH", cmdGetHalfStep);
    sCmd.addCommand("GI", cmdGetFocusMoving);
    sCmd.addCommand("GN", cmdGetNewPos);
    sCmd.addCommand("GP", cmdGetCurrentPos);
    sCmd.addCommand("GT", cmdGetCurrentTemp);
    sCmd.addCommand("GV", cmdGetVersion);
    sCmd.addCommand("SC", cmdSetTempCoef);
    sCmd.addCommand("SD", cmdSetSteppingDelay);
    sCmd.addCommand("SF", cmdSetFullStep);
    sCmd.addCommand("SH", cmdSetHalfStep);
    sCmd.addCommand("SN", cmdSetNewPos);
    sCmd.addCommand("SP", cmdSetCurrentPos);

    pinMode(ENABLE_PIN, OUTPUT);
    pinMode(RESET_PIN, OUTPUT);
    pinMode(MS1_PIN, OUTPUT);
    pinMode(MS2_PIN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLUP);

    setStepMode(FULL_STEP);

    stepper.setSpeed(SPEED);
    stepper.setMaxSpeed(500.0/(float)steppingDelay);
    stepper.setAcceleration(ACCEL);
    digitalWrite(RESET_PIN, HIGH);
    turnOff();

    Serial.begin(9600);
}


void loop()
{
    sCmd.readSerial();  // process serial commands

    // update motor status
    if (isRunning) {
        stepper.run();
        if (digitalRead(SWITCH_PIN) == 0) {
            isRunning = false;
            stepper.stop(); // Stop as fast as possible: sets new target
            stepper.runToPosition();
        }
        if (stepper.distanceToGo() == 0) {
            //start timer to decide when to power off the board
            startTime = millis();
            isRunning = false;
        }
    } else if(isPowerOn) {
        // turn power off if active time period has passed
        if(millis() - startTime > ACTIVE_TIME) {
            turnOff();
        }
    }
}
