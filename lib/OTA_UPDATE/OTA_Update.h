#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H
#include "ThingsBoard.h"
#include "OTA_Update_Callback.h"
#include "OTA_Handler.h"

#include "Arduino.h"

class Logger
{
public:
    void log(char *mess);
};
// void SubcribeFirmwareUpdate(ThingsBoard *tb);
#endif