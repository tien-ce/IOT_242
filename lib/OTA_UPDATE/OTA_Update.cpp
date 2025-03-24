#include <Espressif_Updater.h>
#include "OTA_Update.h"
#include "ThingsBoard.h"
/********************LOG************************/
void Logger::log(char *mess)
{
    Serial.println(mess);
}
/***********************************************/



// void updatedCallback(const bool &success)
// {
//   if (success)
//   {
//     Serial.println("Done, Reboot now");
//     esp_restart();
//     return;
//   }
//   Serial.println("Downloading firmware failed");
// }

// void progressCallback(const size_t &currentChunk, const size_t &totalChuncks)
// {
//   Serial.printf("Progress %.2f%%\n", static_cast<float>(currentChunk * 100U) / totalChuncks);
// }

// void SubcribeFirmwareUpdate(ThingsBoard *tb)
// {
//     if (!currentFWSent)
//     {
//       currentFWSent = tb->Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION) && tb->Firmware_Send_State(FW_STATE_UPDATED);
//     }
  
//     if (!updateRequestSent)
//     {
//       Serial.println("Firwmare Update Subscription...");
//       const OTA_Update_Callback ota_callback(&progressCallback, &updatedCallback, CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
//       updateRequestSent = tb->Subscribe_Firmware_Update(ota_callback);
//     }
// }