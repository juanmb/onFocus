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
#define RESET_PIN 7
#define DIR_PIN 5
#define STEP_PIN 6
#define SWITCH_PIN 9
#define LED_PIN 13


#define USE_SLEEP true  // true: use sleep pin, false: use enable pin
#define HOMING1_SPEED 800
#define HOMING2_SPEED 400
#define HOMING3_SPEED 200
#define HOMING_DIST 400
#define ACCEL 4000      // acceleration (pulse/second2)

// stepping modes
#define FULL_STEP 0
#define HALF_STEP 1
#define MS_4STEPS 2
#define MS_8STEPS 3

// period to wait before turning power off (in milliseconds)
#define ACTIVE_TIME 1000

enum {CMD_NONE, CMD_GOTO, CMD_STOP, CMD_HOME};
enum {ST_IDLE, ST_RUNNING, ST_HOMING1, ST_HOMING2, ST_HOMING3};

SerialCommand sCmd;
AccelStepper stepper(1, STEP_PIN, DIR_PIN);

long startTime = 0;
int command = CMD_NONE;
int state = ST_IDLE;
bool isPowerOn = false;
int speed = 32; // motor speed (from 1 to 32)


long hexstr2long(char *line)
{
    return strtol(line, NULL, 16);
}

void turnOn()
{
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(ENABLE_PIN, !USE_SLEEP);
    isPowerOn = true;
}

void turnOff()
{
    digitalWrite(LED_PIN, LOW);
    digitalWrite(ENABLE_PIN, USE_SLEEP);
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
    command = CMD_STOP;
}


// Go to the new position as set by the ":SNYYYY#" command. returns nothing
// turn stepper on and flag it is running
// is this the only command that should actually make the stepper run ?
void cmdGotoPos(char *param, uint8_t len)
{
    command = CMD_GOTO;
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
    sprintf(tmp, "%02X#", speed);
    Serial.print(tmp);
}

// Returns "FF#" if the focus motor is half-stepped otherwise return "00#".
void cmdGetHalfStep(char *param, uint8_t len)
{
    if (getStepMode() == MS_8STEPS)
        Serial.print("FF#");
    else
        Serial.print("00#");
}

// Returns "00#" if the focus motor is not moving, otherwise return "01#",
// AccelStepper returns Positive as clockwise
void cmdGetFocusMoving(char *param, uint8_t len)
{
    if (stepper.distanceToGo() == 0)
        Serial.print("00#");
    else
        Serial.print("01#");
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
    speed = (int)hexstr2long(param);
}

//Set full-step mode.
void cmdSetFullStep(char *param, uint8_t len)
{
    //setStepMode(FULL_STEP);
    setStepMode(MS_4STEPS);
}

//Set half-step mode.
void cmdSetHalfStep(char *param, uint8_t len)
{
    //setStepMode(HALF_STEP);
    setStepMode(MS_8STEPS);
}

//Set the new position where YYYY is a four-digit
void cmdSetNewPos(char *param, uint8_t len)
{
    long pos = hexstr2long(param);

    if (pos == 0) {
        command = CMD_HOME;
    } else {
        stepper.moveTo(pos);
        command = CMD_GOTO;
    }
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

    setStepMode(MS_4STEPS);

    stepper.setAcceleration(ACCEL);
    digitalWrite(RESET_PIN, HIGH);
    turnOff();

    Serial.begin(9600);
}


void loop()
{
    command = CMD_NONE;
    sCmd.readSerial();  // process serial commands

    // update motor status
    switch(state) {
        case ST_IDLE:
            if (command == CMD_GOTO) {
                turnOn();
                stepper.setMaxSpeed(80.0*(float)speed);
                state = ST_RUNNING;
            } else if (command == CMD_HOME) {
                turnOn();
                stepper.setMaxSpeed(HOMING1_SPEED);
                stepper.setCurrentPosition(20000);
                stepper.moveTo(0);
                state = ST_HOMING1;
            } else {
                // turn power off if active time period has elapsed
                if (isPowerOn && (millis() - startTime > ACTIVE_TIME))
                    turnOff();
            }
            break;

        case ST_RUNNING:
            if (command == CMD_STOP) {
                stepper.stop();
                state = ST_IDLE;
            }

            if (stepper.distanceToGo() == 0) {
                //start timer to decide when to power off the board
                startTime = millis();
                state = ST_IDLE;
            }
            break;

        case ST_HOMING1:
            if (command == CMD_STOP) {
                stepper.stop();
                stepper.setCurrentPosition(0);
                state = ST_IDLE;
            } else if (digitalRead(SWITCH_PIN) == 0) {
                delay(20);
                stepper.setCurrentPosition(0);
                stepper.moveTo(HOMING_DIST);
                stepper.setMaxSpeed(HOMING2_SPEED);
                state = ST_HOMING2;
            }
            break;

        case ST_HOMING2:
            if (command == CMD_STOP) {
                stepper.stop();
                stepper.setCurrentPosition(0);
                state = ST_IDLE;
	    } else if (stepper.currentPosition() == HOMING_DIST) {
                stepper.stop();
                stepper.moveTo(-1000);
                stepper.setMaxSpeed(HOMING3_SPEED);
                state = ST_HOMING3;
            }
            break;

        case ST_HOMING3:
            if (command == CMD_STOP) {
                stepper.stop();
                stepper.setCurrentPosition(0);
                state = ST_IDLE;
	    } else if (digitalRead(SWITCH_PIN) == 0) {
                stepper.stop();
                stepper.setCurrentPosition(0);
                state = ST_IDLE;
            }
            break;
    };

    stepper.run();
}
