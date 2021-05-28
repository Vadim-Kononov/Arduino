/*
ESP32_Signal_Sysytem
на шаблоне
ESP32_Exchange
Oбмен между двумя ESP32 без Wifi и роутера - ESPNOW, обмен по SerialWiFi, обмен по Serial.Bluetooth, обмен по MQTT
Вывод данных на IFTTT, получение времени по NTP
Сохранение данных в NVS, сторожевой таймер WDT
Загрузка скетча OTA

Main плата
WiFi.macAddress  C4:4F:33:11:3B:A9

DEBUG 1 Включает вывод отладочных сообщений в Serial и (или) SerialWiFi
*/

const char * board_name PROGMEM = "Signal System";

/*Включение вывода в Serial при отладке*/
#define DEBUG 1

#if   (DEBUG == 1)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_WRITELN(x, y) Writeln(x, y)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_WRITELN(x, y)
#endif

/*Символьные массивы для хранения логина и пароля WiFi в NVS*/
char ssid [30];
char password [30];

#include "Account.h"

/*Флаги ввода пароля для Terminal, наличия подключения к MQTT*/
bool flag_entry_password = false, flag_MQTT_connected = false, flag_WiFi_connected = false, flag_activation_OTA = false;

/*Cтруктура данных для передачи через ESPNOW*/
struct struct_data
{
int     cod;
int     situation;
int     var3;
int     var4;
int     var5;
int     var6;
int     var7;
};

/*Cтруктура данных для отправки*/
struct_data struct_Send;
/*Cтруктура данных для приема*/
struct_data struct_Receiv;

int receiv_Saved;

/*MAC адрес Slave платы*/
uint8_t broadcastAddress[] = {0xA4, 0xCF, 0x12, 0x24, 0xB4, 0x68};
/*MAC адрес широковещательный*/
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*Определения для ESPNOW, OTA, Ping*/
#include <esp_now.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP32Ping.h>

/*Определения для Bluetooth*/
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

/*Определения для Telnet*/
WiFiServer TelnetServer(59990);
WiFiClient TelnetClient;

/*Определения для MQTT*/
#include <AsyncMqttClient.h>
AsyncMqttClient mqttClient;

/*Определения для NTP*/
#include "time.h"
struct tm timeinfo;

/*Определения для NVS памяти*/
#include <Preferences.h>
Preferences memory;

/*Определения для WDT*/
#include "esp_system.h"
hw_timer_t *timerWDT = NULL;
void IRAM_ATTR ResetModule()
{DEBUG_PRINTLN(F("\n\n\nThe watchdog timerWDT went off\n\n\n"));ESP.restart();}

/*Таймеры*/
#define AlarmRelayPin_time    1000
#define ESPNowSend_time       3000
#define ResetCode_time        2000
#define CheckeSim800_time     2000
#define MqttSend_time         9000
#define Big_Cycle_time        60000
#define LongTime_time         3600000


TimerHandle_t timerBigCycleFunction              = xTimerCreate("timerBigCycleFunction",     pdMS_TO_TICKS(Big_Cycle_time),                   pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(BigCycleFunction));
TimerHandle_t timerESPNowSend                    = xTimerCreate("timerESPNowSend",           pdMS_TO_TICKS(ESPNowSend_time),                  pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(SmallCycleFunction));
TimerHandle_t timerMqttSend                      = xTimerCreate("timerMqttSend",             pdMS_TO_TICKS(MqttSend_time),                    pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(mqtt_Send));

TimerHandle_t timerResetCode                     = xTimerCreate("timerResetCode",            pdMS_TO_TICKS(ResetCode_time),                   pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(ResetCode));
TimerHandle_t timerCheckeSim800                  = xTimerCreate("timerCheckeSim800",         pdMS_TO_TICKS(CheckeSim800_time),                pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(CheckeSim800));
TimerHandle_t timerAlarmLongTime                 = xTimerCreate("timerAlarmLongTime",        pdMS_TO_TICKS(LongTime_time),                    pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(AlarmRelayOff));

TimerHandle_t timerRelayOff_1                    = xTimerCreate("timerRelayOff_1",           pdMS_TO_TICKS(AlarmRelayPin_time),               pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(AlarmRelayOff));
TimerHandle_t timerRelayOff_2                    = xTimerCreate("timerRelayOff_2",           pdMS_TO_TICKS(AlarmRelayPin_time),               pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(AlarmRelayOff));
TimerHandle_t timerRelayOff_3                    = xTimerCreate("timerRelayOff_3",           pdMS_TO_TICKS(AlarmRelayPin_time),               pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(AlarmRelayOff));
TimerHandle_t timerRelayOff_4                    = xTimerCreate("timerRelayOff_4",           pdMS_TO_TICKS(AlarmRelayPin_time),               pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(AlarmRelayOff));
TimerHandle_t timerRelayOn_1                     = xTimerCreate("timerRelayOn_1",            pdMS_TO_TICKS(AlarmRelayPin_time),               pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(AlarmRelayOn_1));
TimerHandle_t timerRelayOn_2                     = xTimerCreate("timerRelayOn_2",            pdMS_TO_TICKS(AlarmRelayPin_time),               pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(AlarmRelayOn_2));



/* ---------- Sygnal System ---------- */

/*Определения для INA219*/
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219;

/*Определения GPIO для SIM800*/
#define TXD2 16
#define RXD2 17
#define SIM800_PWR 5

//Переменная для ответа SIM800 и номераомера телефонов 
String respond_sim800, my_number = "+79198897600", boris_number = "+79094008600", gleb_number = "+78635294929";

//Структура для датчиков сигнализации номер, тип, код, описание
struct detector
{
uint8_t     number;
uint8_t     type;
uint32_t    code;
char        title [50];
};

//Структура для хранения в ОЗУ
detector detector_ram [35];

//Соответствующие глобальные переменные
uint8_t number, type;
uint32_t code;

String title, string_check_random, detector_title;


//Флаги изменения кода, разрешения записи в EEPROM из эфира, флаг включение сигнализации, флаг питания от сети, флаг длительного включения сирены, флаг срабатывания сигнализации
bool flag_code_receive = false, flag_code_save = false, flag_activation_alarm = false, flag_perimeter_alarm = false, flag_power_220 = false , flag_alarm_triggered = false, flag_BigCycleFunction = false ;

//Определения для приемника 433 Мгц
#define RECEIVER_PIN 18
#include <RCSwitch.h>
RCSwitch receiver = RCSwitch();

//Определение пина питания от сети, пинов реле тревоги
#define POWER_PIN 26
#define ALARM_RELAY_PIN_1 19
#define ALARM_RELAY_PIN_2 23
