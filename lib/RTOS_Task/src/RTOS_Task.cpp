#include "RTOS_Task.h"
/*------------------------Begin-------------------------------------*/
#ifdef TASK_LCD
LiquidCrystal_I2C lcd(0x21,16,2);

#endif
/*------------------------------------------------------------------------*/
/*------------------------Declaration-------------------------------------*/
constexpr uint32_t MAX_MESSAGE_SIZE = 4096U;     // Kích thước tối đa của thông điệp gửi qua MQTT (4096 bytes)
constexpr int16_t telemetrySendInterval = 5000U; // Khoảng thời gian gửi dữ liệu cảm biến lên ThingsBoard (10 giây)
WiFiClient wifiClient;                           // Đối tượng WiFi để kết nối mạng
Arduino_MQTT_Client mqttClient(wifiClient);      // Đối tượng MQTT để giao tiếp với broker (ThingsBoard)
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);    // Đối tượng ThingsBoard để gửi và nhận dữ liệu qua MQTT
/*-------------------------------------------------------------------------*/

/*-----------------------------Call Back-----------------------------------*/

/*-------------------------------------------------------------------------*/
CallBack callback;

// Constructor
CallBack::CallBack(std::vector<const char *> shared_attributes, std::vector<RPC_Callback> rpc_list)
    : SHARED_ATTRIBUTES_LIST(shared_attributes), RPC_LIST(rpc_list) {}

// Thêm thuộc tính chia sẻ
void CallBack::Add_Shared_Attribute(const char *shared_attribute)
{
    SHARED_ATTRIBUTES_LIST.push_back(shared_attribute);
}

// Thêm RPC Callback
void CallBack::Add_RPC(const char *rpc_name, std::function<RPC_Response(const RPC_Data &)> rpc_response)
{
    RPC_LIST.push_back(RPC_Callback(rpc_name, rpc_response));
}
// Khởi tạo CallBack cho các thuộc tính chia sẻ
void CallBack::Shared_Attribute_Begin(std::function<bool(const Shared_Attribute_Data &)> Shared_callback)
{
    this->Shared_callback = Shared_callback;
}
// Đăng ký RPC
void SubscribeRPC(CallBack &callback)
{
    #ifdef USE_CALLBACK
    if(callback.RPC_LIST.empty()){
        Serial.println("EMPTY");
    }
    for(auto it : callback.RPC_LIST){
        Serial.print("Call back: ");
        Serial.println( it.Get_Name());
    }
    if (!tb.RPC_Subscribe(callback.RPC_LIST.cbegin(), callback.RPC_LIST.cend()))
    { // Đăng ký nhận lệnh điều khiển từ RPC_LISThingsBoard (RPC)
        Serial.println("Failed to subscribe for RPC");
        return;
    }
    Serial.println("Subcribe to RPC");
    #endif
    #ifdef USE_SHARED_ATTRIBUTE
    for(const char* key:callback.SHARED_ATTRIBUTES_LIST){
        tb.AddUserAttribute(key);
    }
    tb.AddAttributeCallBack(callback.Shared_callback);
    Serial.println("Subcribe to Attribute");
    #endif
}
// In danh sách thuộc tính và RPC
void CallBack::Print_List()
{
    Serial.println("----------- SHARED_ATTRIBUTES_LIST -----------");
    for (size_t i = 0; i < SHARED_ATTRIBUTES_LIST.size(); i++)
    {
        Serial.print(i);
        Serial.print(" : ");
        Serial.println(SHARED_ATTRIBUTES_LIST[i]);
    }

    Serial.println("------------- RPC_LIST -------------");
    for (size_t i = 0; i < RPC_LIST.size(); i++)
    {
        Serial.print(i);
        Serial.print(" : ");
        Serial.println(RPC_LIST[i].Get_Name());
    }
}
/*-------------------------------------------------------------------------*/

/*--------------------------------Task-------------------------------------*/

/*-------------------------------------------------------------------------*/
void ManagerTask::printTaskList()
{
    for (auto it : TaskList)
    {
        Serial.println(it.nameTask);
    }
}
void ManagerTask::addTask(void (*func)(void *), const char *nameTask, uint32_t Stack, uint8_t Pin, uint32_t Delay, void *other )
{
    Parameter pm(Pin, Delay, other);
    Task temp(func, nameTask, Stack, pm);
    this->TaskList.push_back(temp);
}

void ManagerTask::beginTask()
{
    for (Task &it : TaskList)
    {
        xTaskCreate(it.func, it.nameTask, it.Stack, (void *)&(it.pm), 2, NULL);
    }
}
ManagerTask task;
/*-------------------------------------------------------------------------*/

/*----------------------------Task Function--------------------------------*/

/*-------------------------------------------------------------------------*/
#ifdef TASK_BLINKY
void TaskBlinky(void *pvParameters)
{
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();
    pinMode(Pin, OUTPUT);
    for (;;)
    {
        // Serial.print("Pin: ");
        // Serial.println(Pin);
        digitalWrite(Pin, HIGH);
        delay(Delay);
        digitalWrite(Pin, LOW);
        delay(Delay);
    }
}
#endif

#ifdef fIGHT
void TaskLight(void *pvParameters)
{
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();

    LIGHT_VAL *light_val = (LIGHT_VAL *)pm.other;
    for (;;)
    {
        int light = analogRead(pm.get_Pin());
        light_val->light = map(light, 0, 4095, 0, 100);
        delay(Delay);
    }
}
#endif

#ifdef TASK_SOIL
void Tasksoil(void *pvParameters)
{
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();

    SOIL_VAL *soid_val = (SOIL_VAL *)pm.other;
    for (;;)
    {
        int soil = analogRead(pm.get_Pin());
        soid_val->soil = map(soil, 0, 4095, 0, 100);
        delay(Delay);
    }
}
#endif
#ifdef TASK_DHT
void TaskDht(void *pvParameters)
{
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();

    DHT_VAL *re_val = (DHT_VAL *)pm.other;
    uint8_t DHT_type = re_val->DHT_type;

    switch (DHT_type)
    {
    case DHT11:
    case DHT22:
    {
        DHT_Unified dht(Pin, DHT_type);
        dht.begin();
        for (;;)
        {
            sensors_event_t event;
            dht.temperature().getEvent(&event);
            re_val->temperature = event.temperature;
            // Serial.print("Temp: ");
            // Serial.println(re_val->temperature);
            dht.humidity().getEvent(&event);
            re_val->humidity = event.relative_humidity;
            delay(Delay);
        }
        break;
    }

    case Dht20:
    {
        DHT20 dht20;
        dht20.begin();
        for (;;)
        {
            dht20.read();
            re_val->temperature = dht20.getTemperature();
            re_val->humidity = dht20.getHumidity();
            Serial.println(re_val->temperature);
            Serial.println(dht20.getTemperature());
            delay(Delay);
        }
        break;
    }

    default:
        break;
    }
}
#endif
#ifdef TASK_LCD
void TaskLCD(void* pvParameters){
    lcd.begin();
    lcd.clear();
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();
    LCD_VAL lcd_val = *(LCD_VAL*) pm.other;
    for(;;){
        delay(Delay);
    }
}
#endif
#ifdef TASK_UART
void TaskTransmitUart(void *pvParameters)
{
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();

    Uart_VAL transmit = *(Uart_VAL *)pm.other;
    char trans[50];
    for (;;)
    {
        sprintf(trans, "!1:H:%.2f#!1:T:%.2f#", transmit.dht_val->humidity, transmit.dht_val->temperature);
        Serial.println(trans);
        delay(Delay);
    }
}

void TaskReceiveUart(void *pvParameters)
{
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();

    pinMode(20, OUTPUT);
    for (;;)
    {
        if (Serial.available() > 0)
        {
            int status = Serial.read() - '0';
            switch (status)
            {
            case 0:
                digitalWrite(20, LOW);
                break;
            case 1:
                digitalWrite(20, HIGH);
                break;
            default:
                break;
            }
        }
        delay(pm.get_Delay());
    }
}
#endif
void TaskPublishDataToThingsboard(void *pvParameters)
{
    Parameter pm = *((Parameter *)pvParameters);
    uint8_t Pin = pm.get_Pin();
    uint32_t Delay = pm.get_Delay();
    ThingsBoard_VAL thingsboard = *((ThingsBoard_VAL *)pm.other);
    uint8_t num = 0;
    uint32_t previousDataSend = 0;
    float longti = 106.76940000;
    float lati = 10.90682000;
    DynamicJsonDocument jsonBuffer(256);
    JsonObject telemetry = jsonBuffer.to<JsonObject>();
    WiFi.begin(thingsboard.WIFI_SSID, thingsboard.WIFI_PASSWORD); // Kết nối esp32 vào WIFI
    for (;;)
    {
        delay(1000);
        if (WiFi.status() != WL_CONNECTED)
        {
            WiFi.begin(thingsboard.WIFI_SSID, thingsboard.WIFI_PASSWORD); // Kết nối esp32 vào WIFI
            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            }
            Serial.println("Connected to AP");
        }
        if (!tb.connected())
        {
            Serial.print("Connecting to: ");
            Serial.print(thingsboard.THINGSBOARD_SERVER);
            Serial.print(" with token ");
            Serial.println(thingsboard.TOKEN);
            if (!tb.connect(thingsboard.THINGSBOARD_SERVER, thingsboard.TOKEN, thingsboard.THINGSBOARD_PORT))
            {
                Serial.println("Connected to ThingsBoard");
                return;
            }
            Serial.println("Subscribing for RPC...");
            SubscribeRPC(callback);
        }
        if (millis() - previousDataSend > telemetrySendInterval)
        {
            previousDataSend = millis();
            float temperature = thingsboard.dht->temperature;
            float humidity = thingsboard.dht->humidity;
            int soil = thingsboard.soil_val->soil;
            if (isnan(temperature) || isnan(humidity))
            {
                telemetry["temperature"] = NAN;
                telemetry["humidity"] = NAN;
            }
            else
            {
                telemetry["temperature"] = temperature;
                telemetry["humidity"] = humidity;

            }

            // tb.sendTelemetryData("soil", soil);
            if (soil == -1)
            {
                telemetry["soil"] = NULL;
            }
            else
            {
                telemetry["soil"] = thingsboard.soil_val->soil;
            }
            if (thingsboard.light_val->light == -1)
            {
                telemetry["light"] = NULL;
            }
            else
            {
                int light = thingsboard.light_val->light;
                telemetry["light"] = light;
            }
            char jsonStr[256]; // Định nghĩa bộ nhớ đệm cho chuỗi JSON
            serializeJson(telemetry, jsonStr);
            tb.sendTelemetryJson(jsonStr);
        }

        tb.loop();
    }
}
