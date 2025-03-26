#ifndef TASK_H
#define TASK_H
#include "config.h"
#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include "string.h"
#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include <ArduinoOTA.h>
#include <RTOS_Task.h>
#include <OTA_Update.h>
#include "LiquidCrystal_I2C.h"
/*--------------------Define--------------------------------------*/
#define Dht20 0
extern LiquidCrystal_I2C lcd;
/*------------------------Macro-----------------------------------*/
#define setVal(name_val) \
    this->name_val = name_val
#define CreateTask(Task, name_task, stack, Pin, Delay, other) \
    Parameter pm_##Task(Pin, Delay, other);                   \
    xTaskCreate(Task, #name_task, stack, (void *)&pm_##Task, 2, NULL);
/*-------------------------Struct-----------------------------------*/
struct LIGHT_VAL
{
    int light;
};

struct SOIL_VAL
{
    int soil;
};
struct DHT_VAL
{
    uint8_t DHT_type;
    float temperature;
    float humidity;
    DHT_VAL(uint8_t DHT_type)
    {
        this->DHT_type = DHT_type;
        this->temperature = 0;
        this->humidity = 0;
    }
};
struct LCD_VAL{
    DHT_VAL* dht;
};
struct ThingsBoard_VAL
{
    DHT_VAL *dht;
    LIGHT_VAL *light_val;
    SOIL_VAL *soil_val;
    const char *WIFI_SSID;
    const char *WIFI_PASSWORD;
    const char *THINGSBOARD_SERVER;
    const char *TOKEN;
    uint16_t THINGSBOARD_PORT;

    ThingsBoard_VAL(DHT_VAL *dht, LIGHT_VAL *light, SOIL_VAL *soil_val, const char *WIFI_SSID, const char *WIFI_PASSWORD,
                    const char *THINGSBOARD_SERVER, const char *TOKEN, uint16_t THINGSBOARD_PORT)
    {
        this->dht = dht;
        this->light_val = light;
        setVal(soil_val);
        this->WIFI_SSID = WIFI_SSID;
        this->WIFI_PASSWORD = WIFI_PASSWORD;
        setVal(TOKEN);
        setVal(THINGSBOARD_SERVER);
        setVal(THINGSBOARD_PORT);
    }
};
struct Uart_VAL
{
    DHT_VAL *dht_val;
    LIGHT_VAL *light_val;
    Uart_VAL(DHT_VAL *dht_val, LIGHT_VAL *light_val)
    {
        this->dht_val = dht_val;
        this->light_val = light_val;
    }
};

class Parameter
{
private:
    uint8_t Pin;
    uint32_t Task_Delay;

public:
    void *other;
    Parameter(uint8_t Pin = 255, uint32_t delay = 1000, void *other = NULL)
    {
        this->Pin = Pin;
        this->Task_Delay = delay;
        this->other = other;
    }
    uint8_t get_Pin()
    {
        return this->Pin;
    }
    uint32_t get_Delay()
    {
        return this->Task_Delay;
    }
};
/*-----------------------CallBack------------------------*/
class CallBack
{
private:
    std::vector<const char *> SHARED_ATTRIBUTES_LIST; // Danh sách thuộc tính chia sẻ
    std::vector<RPC_Callback> RPC_LIST;               // Danh sách RPC
    std::function<bool(const Shared_Attribute_Data &)> Shared_callback;
    // Đăng kí RPC
    friend void SubscribeRPC(CallBack &callback);

public:
    // Constructor
    CallBack(std::vector<const char *> shared_attributes = {}, std::vector<RPC_Callback> rpc_list = {});

    // Thêm một thuộc tính chia sẻ
    void Add_Shared_Attribute(const char *shared_attribute);
    // Khởi tạo hàm CallBack cho các thuộc tính chia sẻ
    void Shared_Attribute_Begin(std::function<bool(const Shared_Attribute_Data &)> Shared_callback);
    // Thêm một RPC mới
    void Add_RPC(const char *rpc_name, std::function<RPC_Response(const RPC_Data &)> rpc_response);
    // In danh sách thuộc tính và RPC đã đăng ký
    void Print_List();
};
/*------------------------Task-------------------------------*/
class ManagerTask
{
private:
    struct Task
    {
    private:
        void (*func)(void *);
        const char *nameTask;
        uint32_t Stack;
        Parameter pm;
        friend class ManagerTask;

    public:
        Task(void (*func)(void *), const char *nameTask, uint32_t Stack, Parameter pm)
        {
            setVal(func);
            setVal(nameTask);
            setVal(Stack);
            setVal(pm);
        }
    };
    std::vector<Task> TaskList;

public:
    void addTask(void (*func)(void *), const char *nameTask, uint32_t Stack, uint8_t Pin, uint32_t Delay, void *other = NULL);
    void printTaskList();
    void beginTask();
};
#ifdef TASK_LIGHT
void TaskLight(void *pvParameters);
#endif

#ifdef TASK_SOIL
void Tasksoil(void *pvParameters);
#endif

#ifdef TASK_BLINKY
void TaskBlinky(void *pvParameters);
#endif

#ifdef TASK_DHT
void TaskDht(void *pvParameters);
#endif

#ifdef TASK_TRANSMIT_UART
void TaskTransmitUart(void *pvParameters);
#endif

#ifdef TASK_RECEIVE_UART
void TaskReceiveUart(void *pvParameters);
#endif
#ifdef TASK_LCD
void TaskLCD(void* pvParametes);
#endif
void TaskPublishDataToThingsboard(void *pvParameters);
/*-------------------------------------Extern------------------------------------*/
extern ThingsBoard tb;
extern ManagerTask task;
extern CallBack callback;
#endif