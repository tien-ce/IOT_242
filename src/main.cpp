#include "RTOS_Task.h"
#ifdef THINGSBOARD
#include "Wire.h"
#include "OTA_Update_Callback.h"
#include "OTA_Handler.h"
#include <Espressif_Updater.h>
#define LED_PIN 48
#define VALUE_LED_PIN 47
#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12
#define PWM_RESOLUTION 8 // Độ phân giải (8-bit: giá trị từ 0-255)
#define DHT_Pin 10
#define PUMP_PIN 21 //D10
// Nếu muốn sử dụng các task định nghĩa sẵn , vui lòng define tương ứng ở config.h,
// Nếu muốn tự tạo task, có thể thêm struct cho các hàm tùy chỉnh, delay và port nếu không sử dụng có thể gán tùy ý

// constexpr char WIFI_SSID[] = "Trung Ha";
// constexpr char WIFI_PASSWORD[] = "trung2004";
// constexpr char WIFI_SSID[] = "OXY 24H 5G";
// constexpr char WIFI_PASSWORD[] = "oxy24hcoffee";

// constexpr char WIFI_SSID[] = "Thuy Nguyen";
// constexpr char WIFI_PASSWORD[] = "Hoangduong0701";
// constexpr char WIFI_SSID[] = "ACLAB";
// constexpr char WIFI_PASSWORD[] = "ACLAB2023";
// constexpr char WIFI_SSID[] = "HYPERION";
// constexpr char WIFI_PASSWORD[] = "trung2004";

constexpr char WIFI_SSID[] = "vantien";
constexpr char WIFI_PASSWORD[] = "12341234";
// constexpr char TOKEN[] = "oB2UtY3K9tEz6uBXg0rP";   // SensorT3
// constexpr char TOKEN[] = "zsx5bm8e26lcesjhqi0g";   // SensorT2
constexpr char TOKEN[] = "llzsme7rtcrpvm1bkt9i";        // SensorT1
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io"; // Địa chỉ của server ThingsBoard
constexpr uint16_t THINGSBOARD_PORT = 1883U;            // Cổng kết nối MQTT (1883 là cổng mặc định cho MQTT)
constexpr char PUMP_ATTR[] = "fanControl";
volatile bool pump_state = false;
// constexpr uint32_t SERIAL_DEBUG_BAUD = 9600U;   // Tốc độ baud rate cho giao tiếp Serial (dùng để debug)
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;               // Tốc độ baud rate cho giao tiếp Serial (dùng để debug)
constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval"; // Thuộc tính điều chỉnh tần suất nhấp nháy của LED
constexpr char LED_MODE_ATTR[] = "ledMode";                   // Thuộc tính điều chỉnh chế độ LED (ON, OFF, Blink)
constexpr char LED_STATE_ATTR[] = "ledState";                 // Thuộc tính hiển thị trạng thái hiện tại của LED (bật/tắt)

volatile bool attributesChanged = false; // Biến đánh dấu nếu thuộc tính bị thay đổi từ ThingsBoard
volatile int ledMode = 0;                // Chế độ LED: 0 (OFF), 1 (Blink), 2 (ON)
volatile bool ledState = false;          // Trạng thái hiện tại của LED (true: bật, false: tắt)

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;    // Khoảng thời gian nhấp nháy LED tối thiểu (10ms)
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U; // Khoảng thời gian nhấp nháy LED tối đa (60 giây)
volatile uint16_t blinkingInterval = 1000U;           // Mặc định LED nhấp nháy mỗi 1000ms (1 giây)

uint32_t previousStateChange; // Thời gian lưu trạng thái lần cuối để điều khiển nhấp nháy LED

uint32_t previousDataSend; // Thời gian gửi dữ liệu cảm biến lần cuối

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
    // Cấu hình các thuộc tính chia sẻ với ThingsBoard
    LED_STATE_ATTR,
    BLINKING_INTERVAL_ATTR};
/*------------------------Declaration-------------------------------------*/
DHT_VAL re_val = {DHT11}; // For Non-Basic Task
// DHT_VAL re_val = {Dht20};// For Non-Basic Task
// Uart_VAL uart_val = {&re_val,NULL};
LIGHT_VAL light_val;
SOIL_VAL soil_val;
ThingsBoard_VAL things_val = {&re_val, &light_val, &soil_val, WIFI_SSID, WIFI_PASSWORD, THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT};
/*----------------------------------------------------------------*/
/*-----------------------OTA----------------------------------------------------------------*/
constexpr const char CURRENT_FIRMWARE_TITLE[] = "ESP32_S3"; // Title của firmware
constexpr const char CURRENT_FIRMWARE_VERSION[] = "1.1.0";  // Version của firmware
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;         // Số lần thử lại tải firmware
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;            // Kích thước mỗi gói dữ liệu

Espressif_Updater updater;
bool currentFWSent = false;
bool updateRequestSent = false;
void updatedCallback(const bool &success)
{
  if (success)
  {
    Serial.println("Done, Reboot now");
    esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progressCallback(const size_t &currentChunk, const size_t &totalChuncks)
{
  Serial.printf("Progress %.2f%%\n", static_cast<float>(currentChunk * 100U) / totalChuncks);
}
/*--------------------------------------------------------------------------------*/
RPC_Response getLedState(const RPC_Data &data)
{
  bool currentState = digitalRead(LED_PIN);
  Serial.println("Return for things board");
  Serial.println(currentState);
  return RPC_Response("getLedState", currentState); // Gửi trạng thái hiện tại về ThingsBoard
}
RPC_Response setLedSwitchState(const RPC_Data &data)
{                                          // Hàm xử lý lệnh RPC từ ThingsBoard (Bật/Tắt LED từ xa)
  Serial.println("Received Switch state"); // Hiển thị thông tin nhận được lệnh điều khiển
  bool newState = data;                    // Đọc trạng thái mới từ dữ liệu nhận được (true: bật, false: tắt)
  Serial.print("Switch state change: ");
  Serial.println(newState);
  digitalWrite(LED_PIN, newState);              // Thay đổi trạng thái của LED
  attributesChanged = true;                     // Đánh dấu đã có thay đổi để cập nhật lại ThingsBoard
  return RPC_Response("setStateLED", newState); // Trả về kết quả để cập nhật trạng thái trên Dashboard
}
RPC_Response setValueLed(const RPC_Data &data)
{                                       // Hàm xử lý lệnh RPC từ ThingsBoard (Bật/Tắt LED từ xa)
  Serial.println("Received Value LED"); // Hiển thị thông tin nhận được lệnh điều khiển
  uint32_t value = data;                // Đọc trạng thái mới từ dữ liệu nhận được (true: bật, false: tắt)
  Serial.print("Value Led Change: ");
  Serial.println(value);
  int pwmValue = map(value, 0, 100, 0, 225);
  pinMode(VALUE_LED_PIN, OUTPUT);
  analogWrite(VALUE_LED_PIN, pwmValue); // Thay đổi trạng thái của LED

  attributesChanged = true;                  // Đánh dấu đã có thay đổi để cập nhật lại ThingsBoard
  return RPC_Response("setValueLed", value); // Trả về kết quả để cập nhật trạng thái trên Dashboard
}

String new_version;
// Đăng ký Callback để nhận thuộc tính từ ThingsBoard
const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributesRequest, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
void processSharedAttributes(const Shared_Attribute_Data &data)
{ // Xử lý các thuộc tính từ ThingsBoard
  for (auto it = data.begin(); it != data.end(); ++it)
  { // Duyệt qua tất cả các thuộc tính được gửi từ ThingsBoard
    if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0)
    {                                                           // Kiểm tra nếu thuộc tính là "blinkingInterval"
      const uint16_t new_interval = it->value().as<uint16_t>(); // Đọc giá trị khoảng thời gian nhấp nháy mới
      if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX)
      {
        blinkingInterval = new_interval; // Cập nhật khoảng nhấp nháy LED nếu nằm trong giới hạn cho phép
        Serial.print("Blinking interval is set to: ");
        Serial.println(new_interval); // In ra khoảng thời gian mới để kiểm tra
      }
    }
    else if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0)
    {                                    // Kiểm tra nếu thuộc tính là "ledState"
      ledState = it->value().as<bool>(); // Đọc trạng thái mới của LED (bật/tắt)
      digitalWrite(LED_PIN, ledState);   // Thay đổi trạng thái LED theo lệnh từ ThingsBoard
      Serial.print("LED state is set to: ");
      Serial.println(ledState); // In trạng thái LED để kiểm tra
    }
    else if (strcmp(it->key().c_str(), PUMP_ATTR) == 0)
    {
      pump_state = it->value().as<bool>();
      digitalWrite(PUMP_PIN, pump_state);
      Serial.print("Pump state is set to: ");
      Serial.println(pump_state);
    }
    // Nếu có thay đổi về title hoặc version 
    else if(strcmp(it->key().c_str(),"fw_title") == 0 
          || strcmp(it->key().c_str(),"fw_version") == 0) 
    {
      // Kiểm tra title
     
    }
  }
  attributesChanged = true; // Đánh dấu rằng thuộc tính đã được thay đổi
}
void processSharedAttributesRequest(const Shared_Attribute_Data &data){
  String fw_title;
  String fw_version; 
  for(auto it = data.begin(); it!=data.end();++it){
    if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0)
    {                                                           // Kiểm tra nếu thuộc tính là "blinkingInterval"
      const uint16_t new_interval = it->value().as<uint16_t>(); // Đọc giá trị khoảng thời gian nhấp nháy mới
      if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX)
      {
        blinkingInterval = new_interval; // Cập nhật khoảng nhấp nháy LED nếu nằm trong giới hạn cho phép
        Serial.print("Blinking interval is set to: ");
        Serial.println(new_interval); // In ra khoảng thời gian mới để kiểm tra
      }
    }
    else if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0)
    {                                    // Kiểm tra nếu thuộc tính là "ledState"
      ledState = it->value().as<bool>(); // Đọc trạng thái mới của LED (bật/tắt)
      digitalWrite(LED_PIN, ledState);   // Thay đổi trạng thái LED theo lệnh từ ThingsBoard
      Serial.print("LED state is set to: ");
      Serial.println(ledState); // In trạng thái LED để kiểm tra
    }
    else if (strcmp(it->key().c_str(), PUMP_ATTR) == 0)
    {
      pump_state = it->value().as<bool>();
      digitalWrite(PUMP_PIN, pump_state);
      Serial.print("Pump state is set to: ");
      Serial.println(pump_state);
    }
    else if (strcmp(it->key().c_str(),"fw_title") == 0){
       fw_title = it->value().as<String>();
    }
    else if(strcmp(it->key().c_str(),"fw_version") == 0){
      fw_version = it->value().as<String>();
    }
  }
  if(strcmp(CURRENT_FIRMWARE_TITLE,fw_title.c_str() )!= 0){
    // Kiểm tra title
    Serial.println("TB : Firmware is not for us, title is differnet");
  }
  else{
    // Kiểm tra version
    if(strcmp(CURRENT_FIRMWARE_VERSION,fw_version.c_str() ) == 0){
      Serial.println("No thing to Update, same version");
    }
    else{
      if(!currentFWSent){
        currentFWSent = tb.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION) && tb.Firmware_Send_State(FW_STATE_UPDATED);
      }
      if(!u)
    }
  }
}


void setup()
{
  Serial.begin(SERIAL_DEBUG_BAUD); // Khởi động giao tiếp Serial với tốc độ 115200 để debug
  //   Serial.println("Image displayed!");
  pinMode(LED_PIN, OUTPUT);
  callback.Add_RPC("setValueLED", setValueLed);
  callback.Add_RPC("setStateLED", setLedSwitchState);
  callback.Add_Shared_Attribute(BLINKING_INTERVAL_ATTR);
  callback.Add_Shared_Attribute(LED_STATE_ATTR);
  callback.Shared_Attribute_Begin(processSharedAttributes);
  callback.Print_List();
  // delay(1000);  // Chờ 1 giây trước khi thực hiện các tác vụ tiếp theo

  #ifdef TASK_BLINKY
    task.addTask(TaskBlinky, "Blinky", 1024, 6, 1000, NULL);
  #endif

  #ifdef TASK_DHT
    Wire.begin(11, 12);  // Khởi tạo giao tiếp I2C với cảm biến DHT20
    task.addTask(TaskDht, "dht", 2048, DHT_Pin, 1000, &re_val);
  #endif

  #ifdef TASK_SOIL
    task.addTask(TaskSoil, "Soil", 2048, 1, 1000, &soid_val);
  #endif
  task.addTask(TaskPublishDataToThingsboard, "Thingsboard", 16384, 225, 1000, &things_val);
  task.beginTask();
}

void loop()
{
  // if (!currentFWSent)
  // {
  //   currentFWSent = tb.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION) && tb.Firmware_Send_State(FW_STATE_UPDATED);
  // }
  // if (!updateRequestSent)
  // {
  //   Serial.println("Firwmare Update Subscription...");
  // const OTA_Update_Callback callback(&progressCallback, &updatedCallback, CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
  //   updateRequestSent = tb.Subscribe_Firmware_Update(callback);
  // }
  // delay(1000);
}
#endif
