/******************************************************************
    Author:     Juan Menendez Blanco    <juanmb@gmail.com>

    This code is part of the MoonliteFocuser project:
        https://github.com/juanmb/MoonliteFocuser

*******************************************************************************/

#include <Arduino.h>
#include "serial_command.h"

#define START_CHAR ':'
#define END_CHAR '#'
#define HDR_SIZE 2

SerialCommand::SerialCommand()
{
    nCommands = 0;
    bufPos = 0;
}

// Add a new command callback function
int SerialCommand::addCommand(const char *header, cbFunction function)
{
    if (nCommands >= MAX_COMMANDS) {
        return 1;
    }
    strncpy(commandList[nCommands].header, header, 2);
    commandList[nCommands].function = function;
    nCommands++;
    return 0;
}

// Process available data in the input serial buffer
void SerialCommand::readSerial()
{
    while (Serial.available()) {
        char c = Serial.read();
        buffer[bufPos++] = c;

        if (bufPos >= MAX_CMD_SIZE) {
            bufPos = MAX_CMD_SIZE - 1;
        }

        if (c == END_CHAR) {
            for (int i = 0; i < nCommands; i++) {
                if (strncmp(commandList[i].header, buffer + 1, HDR_SIZE) == 0) {
                    uint8_t param_len = bufPos - HDR_SIZE + 1;
                    commandList[i].function(buffer + HDR_SIZE + 1, param_len);
                    break;
                }
            }
            bufPos = 0;
        }
    }
}
