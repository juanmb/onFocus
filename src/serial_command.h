#ifndef _SerialCommand_h_
#define _SerialCommand_h_

#include <inttypes.h>

#define MAX_CMD_SIZE 16
#define MAX_COMMANDS 32


typedef void (* cbFunction)(char *param, uint8_t len);


typedef struct commandCallback {
    char header[2];
    cbFunction function;
} commandCallback;


class SerialCommand {
public:
    SerialCommand();
    void readSerial();
    int addCommand(const char *header, cbFunction function);
private:
    commandCallback commandList[MAX_COMMANDS];
    char buffer[MAX_CMD_SIZE];
    cbFunction cmdFunction;
    int nCommands;
    int bufPos;
};

#endif
