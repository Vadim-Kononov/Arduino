/*Шаблон, основанный на задачах FreeRTOS, с поддержкой сервисов*/
/*ESP-NOW, OTA, BluetoothSerial, TelnetSerial, MQTT, IFTTT*/
/*MAIN плата инициирует ESP-NOW обмен с подтверждением получения от SLAVE*/
/*SLAVE возвращает полученный, случайный хэш*/
/*SLAVE не поддерживает MQTT, в остальном аналогична MAIN*/

// #define MAIN																		//Закомментировать для SLAVE платы
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

#ifdef MAIN //----------------------------------------------------------------------------------------------------------------------------------
/*MAC AC:67:B2:F8:E:A8 Адрес главной платы*/
const char *board_name = "Number One";
uint8_t mac_Peer_One[] = {0xCC, 0x50, 0xE3, 0xB5, 0x8F, 0xF0};						//Адрес SLAVE платы

#else //----------------------------------------------------------------------------------------------------------------------------------------
/*MAC CC:50:E3:B5:8F:F0 Адрес подчиненной платы*/
const char *board_name = "Number Two";
uint8_t mac_Peer_One[] =  {0xAC, 0x67, 0xB2, 0xF8, 0xEE, 0xA8}; 					//Адрес MAIN платы
#endif //---------------------------------------------------------------------------------------------------------------------------------------

/*Символьные массивы для хранения логина и пароля WiFi в NVS*/
char ssid [30];
char pass [30];

/*Символьные константы для IFTTT vadim.kononov.net@gmail.com*/
const char * ifttt_event	PROGMEM = "another";
const char * ifttt_api_key	PROGMEM = "nxDul7UGF8ulaxG3tXgKU2iQSCYkRlJvZ_5pZdC2UX0";

/*Определение и константы для MQTT && Аккаунт vadim@kononov.xyz WQTT.RU*/
AsyncMqttClient mqttClient;
const char * mqtt_host		 = "m5.wqtt.ru";
const char * mqtt_username	 = "u_6TLZKS";
const char * mqtt_pass		 = "J5kW8jTy";
const int	 mqtt_port		 = 2836;

/*Определения для Telnet*/
WiFiServer TelnetServer(59990);
WiFiClient TelnetClient;

/*Определение для Bluetooth*/
BluetoothSerial SerialBT;

/*Определение для NVS памяти*/
Preferences memory;

/*Структура для передачи по ESP-NOW*/
struct now_Data
{
char		board_name [20];
bool		flag;
uint16_t	hash;
};
now_Data now_get;
now_Data now_put;

/*Очереди*/
QueueHandle_t queueReceiving;

/*Семафоры*/
xSemaphoreHandle xBinSemaphore_Get_End;												//Окончание приема ESP-NOW
xSemaphoreHandle xBinSemaphore_Put_Start;											//Запуск передачи ESP-NOW

/*Таймеры*/
TimerHandle_t timerClock 		= xTimerCreate("timerClock", 	pdMS_TO_TICKS(10000), 		pdTRUE,   (void*)0, reinterpret_cast<TimerCallbackFunction_t>(Clock));		//Произвольная, периодическая функция Clock
TimerHandle_t timerReset  		= xTimerCreate("timerReset",  	pdMS_TO_TICKS(500),         pdFALSE,  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(Reset));		//Задержка для вызова Reset
TimerHandle_t timerRecon  		= xTimerCreate("timerRecon",  	pdMS_TO_TICKS(500),         pdFALSE,  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(Recon));		//Задержка для вызова Reconnect
TimerHandle_t timerIFTTTTime  	= xTimerCreate("timerIFTTTTime",pdMS_TO_TICKS(500),         pdFALSE,  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(IFTTTTime));	//Задержка ожидания ответа сервера

/*Глобальные переменные*/
bool
flag_WiFi_connected = false,														//Флаг подключение к WiFi
flag_MQTT_connected = false,														//Флаг подключение к MQTT
flag_After_Boot		= false,														//Флаг перезагрузки
flag_IFTTT_time 	= false,														//Флаг ожидания ответа сервера
flag_Terminal_pass 	= false;														//Флаг ввода пароля



void setup()
{
Serial.begin(112500);
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

memcpy(now_put.board_name, board_name, 20);											//Запись имени платы в структуру

/*Удаление ключей || Инициализация NVS памяти */
//memory.remove("name");
//memory.clear();
memory.begin("memory", false);
/*Настройка ESP-NOW*/
if (esp_now_init() != ESP_OK) {log_i("! Error initializing ESP-NOW !"); return;}
esp_now_peer_info_t peerInfo; 
memcpy(peerInfo.peer_addr, mac_Peer_One, 6);
peerInfo.channel = 0;
peerInfo.encrypt = false;
if (esp_now_add_peer(&peerInfo) != ESP_OK){log_i("! Failed to add peer !"); return;}
/*Назначение функции обработки приема ESP-NOW*/
esp_now_register_recv_cb(OnReceiving);

/*Очереди*/
queueReceiving = xQueueCreate(1, sizeof(now_Data));									//Передача принятых данных ESP-NOW

/*Семафоры*/
vSemaphoreCreateBinary(xBinSemaphore_Get_End);										//Окончание приема ESP-NOW
vSemaphoreCreateBinary(xBinSemaphore_Put_Start);									//Запуск передачи ESP-NOW

/*Задачи*/
xTaskCreate(WiFiConnect,	"WiFiConnect"		, 2048,  NULL, 1, NULL);			//Подключение WiFi при потере связи
xTaskCreate(Main_Function, 	"Main_Function"		, 2048,  NULL, 0, NULL);			//Основная задача
xTaskCreate(Slave_Function, "Slave_Function"	, 2048,  NULL, 0, NULL);			//Вспомогательная задача
xTaskCreate(Receiving, 		"Receiving"			, 2048,  NULL, 0, NULL); 			//Обработка принятых ESP-NOW данных
xTaskCreate(Sending, 		"Sending"			, 2048,  NULL, 0, NULL); 			//Отправка ESP-NOW данных
xTaskCreate(Bluetooth, 		"Bluetooth"			, 4096,  NULL, 0, NULL); 			//Bluetooth терминал
xTaskCreate(Telnet, 		"Telnet"			, 4096,  NULL, 0, NULL); 			//Telnet терминал
xTaskCreate(OTA, 			"OTA"				, 2048,  NULL, 0, NULL); 			//OTA загрузка скетча

/*Запуск таймера*/
xTimerStart(timerClock, 10);														//Произвольная, периодическая функция Clock

/*Инкремент счетчика перезагрузок*/
memory.putInt("countReset", memory.getInt("countReset") + 1);

#ifdef MAIN //----------------------------------------------------------------------------------------------------------------------------------
/*Назначение функций и параметров MQTT*/
xTaskCreate(MQTTConnect,	"MQTTConnect"		, 2048,  NULL, 1, NULL); 			//Подключение MQTT при потере связи
xTaskCreate(MQTTSend,		"MQTTSend"			, 2048,  NULL, 1, NULL); 			//Отправка MQTT

mqttClient.onConnect(mqtt_Connected_Complete);										//Назначение функции обработки события подключения MQTT
mqttClient.onDisconnect(mqtt_Disconnect_Complete);									//Назначение функции обработки события отключения MQTT
mqttClient.onMessage(mqtt_Receiving_Complete);										//Назначение функции обработки события приема данных MQTT
/*Настройка MQTT*/
mqttClient.setServer(mqtt_host, mqtt_port);										
mqttClient.setCredentials(mqtt_username, mqtt_pass);												
mqttClient.setClientId(board_name);
#endif //---------------------------------------------------------------------------------------------------------------------------------------
}

/*Отключение loop()*/
void loop() {vTaskSuspend(NULL);}