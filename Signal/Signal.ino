/*Сигнализация, вариант на задачах FreeRTOS, с поддержкой сервисов*/
/*ESP-NOW, OTA, BluetoothSerial, TelnetSerial, MQTT, IFTTT*/

#define TICKCOUNT xTaskGetTickCount()/1000.0										//Время от начала загрузки
#define CONFIG_ARDUHAL_LOG_COLORS 1													//Включение цветных логов

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ESPmDNS.h>																//Для ОТА
#include <WiFiUdp.h>																//Для ОТА
#include "BluetoothSerial.h"
#include <AsyncMqttClient.h>
#include <Preferences.h>

#include <Wire.h>																	//Для INA219
#include <Adafruit_INA219.h>

/*Определения GPIO для SIM800*/
#define RXD2 25
#define TXD2 27
#define SIM800_PWR 32

/*Определение пина питания от сети, пинов реле тревоги*/
#define POWER_PIN 18
#define ALARM_RELAY_PIN_1 26
#define ALARM_RELAY_PIN_2 33

/*Определения пина прерывания от приемника 433 Мгц*/
#define RECEIVER_PIN 23
#include <RCSwitch.h>

/*Определения для Telnet*/
WiFiServer TelnetServer(59990);
WiFiClient TelnetClient;

/*Определение для Bluetooth*/
BluetoothSerial SerialBT;

/*Определение для NVS памяти*/
Preferences memory;

/*Определениe для приемника 433 Мгц*/
RCSwitch receiver = RCSwitch();

/*Определениe для INA219*/
Adafruit_INA219 ina219;

/*Имя данной платы и адрес SLAVE платы*/
const char *board_name = "Signal";
uint8_t mac_Peer_One[] = {0xCC, 0x50, 0xE3, 0xB5, 0x8F, 0xF0};

/*Символьные массивы для хранения логина и пароля WiFi в NVS*/
char ssid [30];
char pass [30];

/*Символьные константы для IFTTT vadim.kononov.net@gmail.com*/
const char * ifttt_event	PROGMEM = "another";
const char * ifttt_api_key	PROGMEM = "nxDul7UGF8ulaxG3tXgKU2iQSCYkRlJvZ_5pZdC2UX0";

/*Определение и константы для MQTT && Аккаунт vadim@kononov.xyz WQTT.RU*/
AsyncMqttClient mqttClient;
const char * mqtt_host		PROGMEM = "m5.wqtt.ru";
const char * mqtt_username	PROGMEM = "u_6TLZKS";
const char * mqtt_pass		PROGMEM = "J5kW8jTy";
const int	 mqtt_port		PROGMEM = 2836;

/*Структура для передачи по ESP-NOW*/
struct now_Data
{
char		board_name [20];
uint16_t	hash;
bool		exchange;
bool		protect;
bool		alarm;
bool 		light;
};
now_Data now_get;																	//Структура принимаемая
now_Data now_put;																	//Структура отправляемая

/*Структура для управления сиреной*/
struct alarm_Startup
{
uint32_t	on;
uint32_t	off;
uint16_t	count;
};
alarm_Startup alarm_start;

/*Структура для датчиков сигнализации тип, код, описание*/
struct detector
{
uint8_t     type;
uint32_t    code;
char        title [50];
};
detector detector_ram [35];

/*Глобальные переменные*/
bool
flag_WiFiConnected 	= false,														//Флаг подключение к WiFi
flag_MQTTConnected 	= false,														//Флаг подключение к MQTT
flag_AfterBoot		= false,														//Флаг перезагрузки
flag_TerminalPass 	= false,														//Флаг ввода пароля
flag_SavingCode		= false,														//Флаг ожидания сохранения кода
flag_RepeatSignal	= false,														//Флаг недавнего срабатывания
flag_ProtectOn 		= false,														//Флаг включения сигнализации
flag_AlarmEnable	= true;															//Флаг разрешения сирены

uint8_t
number,																				//Номер датчика в массиве
type;																				//Тип датчика

String
title,																				//Имя датчика
my_number_1 	= "+79198897600",
my_number_2 	= "+79094008600",
boris_number 	= "+79896134008",
gleb_number 	= "+79896134009";

/*Задачи*/
TaskHandle_t handleAlarmProcessing;													//Хэндл задачи управления сиреной

/*Очереди*/
QueueHandle_t queueAlarm;															//Очередь для задачи управления сиреной

/*Семафоры*/
xSemaphoreHandle xBinSemaphore_PutStart;											//Запуск передачи ESP-NOW
xSemaphoreHandle xBinSemaphore_SensorSignal;										//Запуск обработки полученного кода датчика

/*Таймеры*/
TimerHandle_t timerReset  		= xTimerCreate("timerReset",  		pdMS_TO_TICKS(500),         pdFALSE,  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(Reset));		//Задержка для вызова Reset
TimerHandle_t timerRecon  		= xTimerCreate("timerRecon",  		pdMS_TO_TICKS(500),         pdFALSE,  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(Recon));		//Задержка для вызова Reconnect
TimerHandle_t timerSavingCode  	= xTimerCreate("timerSavingCode",	pdMS_TO_TICKS(5000),       	pdFALSE,  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(SavingCode));	//Задержка ожидания записи кода
TimerHandle_t timerRepeatSignal = xTimerCreate("timerRepeatSignal",	pdMS_TO_TICKS(10000),       pdFALSE,  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(RepeatSignal));//Задержка обработки сигнала от датчика, для исключения повторного срабатывания



void setup()
{
Serial.begin(115200);
/*Последовательный порт SIM800*/
Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

/*Настройка WiFi*/
WiFi.mode(WIFI_STA);
WiFi.setHostname(board_name);
WiFi.setAutoConnect(false);
WiFi.setAutoReconnect(false);
WiFi.softAP("Now", NULL, 1, 1);
WiFi.onEvent(WiFiEvent);

TelnetServer.begin();
TelnetServer.setNoDelay(true);

SerialBT.begin(board_name);
SerialBT.register_callback(BT_Event);

ArduinoOTA.setHostname(board_name);
ArduinoOTA.begin();

//Установка флага питания от сети, установки пинов тревоги
pinMode(POWER_PIN, INPUT_PULLUP);
pinMode(ALARM_RELAY_PIN_1, OUTPUT);
pinMode(ALARM_RELAY_PIN_2, OUTPUT);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);

/*Настройка SIM800*/
pinMode(SIM800_PWR, OUTPUT);
digitalWrite(SIM800_PWR, HIGH);
sendATCommand("AT", 5000);
sendATCommand("ATE1V1+CMGF=1;&W", 5000);
sendATCommand("AT+CMGDA=\"DEL ALL\"", 5000);

/*Инициализация приемника 433 МГц*/
receiver.enableReceive(RECEIVER_PIN);
/*Инициализация измерителя тока*/
ina219.begin();
/*Запись имени платы в структуру*/
memcpy(now_put.board_name, board_name, 20);


//Инициализация NVS памяти
memory.begin("memory", false);
//Очистка NVS памяти
//memory.clear();
//Очистка ключа NVS памяти
//memory.remove("name");

/*Настройка ESP-NOW*/
if (esp_now_init() != ESP_OK) {log_i("! Error initializing ESP-NOW !"); return;}
esp_now_peer_info_t peerInfo; 
memcpy(peerInfo.peer_addr, mac_Peer_One, 6);
peerInfo.channel = 0;
peerInfo.encrypt = false;
if (esp_now_add_peer(&peerInfo) != ESP_OK){log_i("! Failed to add peer !"); return;}

esp_now_register_recv_cb(OnReceiving);												//Назначение функции обработки приема ESP-NOW

/*Очереди*/
queueAlarm 		= xQueueCreate(1, sizeof(alarm_Startup));							//Данные для включения сирены

/*Семафоры*/
vSemaphoreCreateBinary(xBinSemaphore_PutStart);										//Запуск передачи ESP-NOW
vSemaphoreCreateBinary(xBinSemaphore_SensorSignal);									//Запуск обработки кода датчика

/*Обнуление очереди и семафора*/											
xQueueReceive(queueAlarm, &alarm_start, 100);										//Для предотвращения страбатывания после загрузки
xSemaphoreTake(xBinSemaphore_SensorSignal, 	100);

/*Задачи*/
xTaskCreate(WiFiConnect,	"WiFiConnect"		, 2000,  NULL, 1, NULL);			//Подключение WiFi при потере связи
xTaskCreate(Now_Exchange, 	"Now_Exchange"		, 2000,  NULL, 0, NULL);			//ESP-NOW обмен
xTaskCreate(Bluetooth, 		"Bluetooth"			, 3000,  NULL, 0, NULL); 			//Bluetooth терминал
xTaskCreate(Telnet, 		"Telnet"			, 3000,  NULL, 0, NULL); 			//Telnet терминал
xTaskCreate(OTA, 			"OTA"				, 2000,  NULL, 0, NULL); 			//OTA загрузка скетча
xTaskCreate(MQTTConnect,	"MQTTConnect"		, 2000,  NULL, 1, NULL); 			//Подключение MQTT при потере связи
xTaskCreate(MQTTSend,		"MQTTSend"			, 2000,  NULL, 0, NULL); 			//Отправка MQTT

xTaskCreate(CodeProcessing,	"CodeProcessing"	, 2000,  NULL, 0, NULL); 			//Получение кода датчика
xTaskCreate(SIM800Processing,"SIM800Processing"	, 3000,  NULL, 0, NULL); 			//Получение SMS
xTaskCreate(PowerControl,	"PowerControl"		, 2000,  NULL, 0, NULL); 			//Контроль 220В
xTaskCreate(SensorSignal,	"SensorSignal"		, 2000,  NULL, 0, NULL);			//Обработка сигнала от датчика

xTaskCreate(AlarmProcessing, "AlarmProcessing" 	, 1000,	 NULL, 0, &handleAlarmProcessing); //Управление сиреной

//Загрузка таблицы датчиков из EEPROM
memory.getBytes("mem", (detector*)detector_ram, sizeof(detector_ram));
/*Инкремент счетчика перезагрузок*/
memory.putInt("countReset", memory.getInt("countReset") + 1);

/*Назначение функций и параметров MQTT*/
mqttClient.onConnect(mqtt_Connected_Complete);										//Назначение функции обработки события подключения MQTT
mqttClient.onDisconnect(mqtt_Disconnect_Complete);									//Назначение функции обработки события отключения MQTT
mqttClient.onMessage(mqtt_Receiving_Complete);										//Назначение функции обработки события приема данных MQTT
/*Настройка MQTT*/
mqttClient.setServer(mqtt_host, mqtt_port);										
mqttClient.setCredentials(mqtt_username, mqtt_pass);												
mqttClient.setClientId(board_name);
}

/*Отключение loop()*/
void loop() {vTaskSuspend(NULL);}