/*
Страница определений
*/

//Включение вывода в Serial
//#define Serial_On

// Имя модуля
#define NAME "Signal System"

//Библиотека для отключения brownout detector
#include "soc/rtc_cntl_reg.h"

//Определения для WiFi, OTA, UDP, MQTT
#include <rom/rtc.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

//Определения для UDP рассылки
#include "AsyncUDP.h"
AsyncUDP udp;

//Определения для MQTT vadim.kononov.net@gmail.com
#include <PubSubClient.h>
WiFiClient mqttClient;
PubSubClient client(mqttClient);

#include "Account.h"
String       mqtt_device = "ESP32Client-";

//Определение сервера для telnet, порт 59990
WiFiServer TelnetServer(59990);
//Определение клиента для telnet, не более одного клиента
WiFiClient TelnetClient;

//Определения для Bluetoot
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

//Определения для записи-чтения EEPROM
#include <Preferences.h>
Preferences memory;

//Определения для NTP Time
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;
//Определение функции получение времени GetTime()
void GetTime()
{
if (WiFi.status() != WL_CONNECTED) return;
configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
getLocalTime(&timeinfo);
}

//Определения для WDT
#include "esp_system.h"
//Таймаут WDT, мс
const int wdtTimeout = 60000;
hw_timer_t *timer = NULL;
//Прграмма обработки прерывания WDT
void IRAM_ATTR ResetModule()
{
ESP.restart();
}
//Получение причины последней перезагрузки
String print_reset_reason(RESET_REASON reason)
{
String value;
switch (reason)
{
case 1 	: value = "POWERON_RESET";          break;/**<1,  Vbat power on reset*/
case 3 	: value = "SW_RESET";               break;/**<3,  Software reset digital core*/
case 4 	: value = "OWDT_RESET";             break;/**<4,  Legacy watch dog reset digital core*/
case 5 	: value = "DEEPSLEEP_RESET";        break;/**<5,  Deep Sleep reset digital core*/
case 6 	: value = "SDIO_RESET";             break;/**<6,  Reset by SLC module, reset digital core*/
case 7 	: value = "TG0WDT_SYS_RESET";       break;/**<7,  Timer Group0 Watch dog reset digital core*/
case 8 	: value = "TG1WDT_SYS_RESET";       break;/**<8,  Timer Group1 Watch dog reset digital core*/
case 9 	: value = "RTCWDT_SYS_RESET";       break;/**<9,  RTC Watch dog Reset digital core*/
case 10 : value = "INTRUSION_RESET";        break;/**<10, Instrusion tested to reset CPU*/
case 11 : value = "TGWDT_CPU_RESET";        break;/**<11, Time Group reset CPU*/
case 12 : value = "SW_CPU_RESET";           break;/**<12, Software reset CPU*/
case 13 : value = "RTCWDT_CPU_RESET";       break;/**<13, RTC Watch dog Reset CPU*/
case 14 : value = "EXT_CPU_RESET";          break;/**<14, for APP CPU, reseted by PRO CPU*/
case 15 : value = "RTCWDT_BROWN_OUT_RESET"; break;/**<15, Reset when the vdd voltage is not stable*/
case 16 : value = "RTCWDT_RTC_RESET";       break;/**<16, RTC Watch dog reset digital core and rtc module*/
default : value = "NO_MEAN";
}
return value;
}

//Определения для INA219
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219;

//Символьные массивы для логина и пароля WiFi
char ssid [30];
char password [30];

//Определения для IFTTT
#define KEY "nxDul7UGF8ulaxG3tXgKU2iQSCYkRlJvZ_5pZdC2UX0"
#define EVENT_NAME1 "ESP32_Reboot"
#define EVENT_NAME2 "ESP32_WiFi"

//Количество попыток подключения к WiFi за одну сессию и встроенный светодиод
#define Connection_Count_WiFi 10
#define Onboard_Led 2

//Таймеры на 61, 5, 0,25 сек.
long count_ms_Timer60, time_ms_Timer60, count_ms_Timer5, time_ms_Timer5, count_ms_Timer05, time_ms_Timer05;

//Флаги ввода пароля для SerialBT, рекконекта WiFi, отключения WiFiSerial при OTA загрузке
bool key_flag = false, res_flag = false, wt_ota_flag = true;



//____________________________________SIM800_______________________________________________________________________
//Определения GPIO для SIM800
#define RXD2 25
#define TXD2 27
#define SIM800_PWR 5
//Глобальная строковая переменная для хранения ответа SIM800 и строка подтверждения включения сирены
String respond;
//Номера телефонов
String my_number = "+79198897600", boris_number = "+79896134008", gleb_number = "+79896134009";
//____________________________________SIM800_______________________________________________________________________



//____________________________________Signal System_______________________________________________________________________
//Определения для IFTTT
#define EVENT_NAME3 "Signal_System"

//Таймер приема кода
long count_ms_Code, time_ms_Code;
//Таймер отключения сирены
long count_ms_TimerAlarm, time_ms_TimerAlarm;

//Структура для датчиков сигнализации номер, тип, код, описание
struct detector {uint8_t number; uint8_t type; uint32_t code; char title [50];};
//Структура для хранения в EEPROM
detector detect_mem [35];
//Структура для хранения в ОЗУ
detector detect_buf [35];
//Соответствующие глобальные переменные
uint8_t number, type;
uint32_t code;

String title, check_string;
//Флаги изменения кода, разрешения записи в EEPROM из эфира, флаг включение сигнализации, флаг питания от сети, флаг длительного включения сирены, флаг срабатывания сигнализации
bool code_flag = false, dev_flag = false, on_off_flag = false, on_off_perimeter_flag = false, power_flag, siren_on_flag = false, alarm_operation_flag = false;
byte situation = 0;

//Определения для приемника 433 Мгц
#define RECEIVER_PIN 18
#include <RCSwitch.h>
RCSwitch receiver = RCSwitch();

//Определение пина питания от сети, пинов реле тревоги
#define POWER_PIN 26
#define ALARM_RELAY_PIN_1 19
#define ALARM_RELAY_PIN_2 23
//____________________________________Signal System_______________________________________________________________________
