void setup()
{
#if   (DEBUG == 1)
Serial.begin(115200);
#endif

/*Инициализация NVS памяти*/
memory.begin("memory", false);

/*Инкримент счетчика перезагрузок на 1*/
memory.putInt("countReset", memory.getInt("countReset") + 1);

/*Первоначальное (первое для платы) сохранение логина и пароля WiFi закомментировано*/
/*memory.putString("login", "One"); memory.putString("pass", "73737373");*/

/*Назначение функции WiFiEvent*/
WiFi.onEvent(WiFiEvent);

/*Подключение к WiFi*/
WiFiConnect();

/*Инициализация  ESPNOW*/
if (esp_now_init() != ESP_OK) {DEBUG_PRINTLN("Error initializing ESP-NOW"); return;}

/*Регистрация функции обратного вызова при отправке данных ESPNOW*/
esp_now_register_send_cb(Now_Sending_Complete);

/*Регистрация функции обратного вызова при приеме данных ESPNOW*/
esp_now_register_recv_cb(Now_Receiving_Complete);

/*Запись информации о пире*/
esp_now_peer_info_t peerInfo;
memcpy(peerInfo.peer_addr, broadcastAddress, 6);
peerInfo.channel = 0;
peerInfo.encrypt = false;

/*Добавление пира в список*/
if (esp_now_add_peer(&peerInfo) != ESP_OK) {DEBUG_PRINTLN("Failed to add peer"); return;}

/*Иниализация передаваемых по ESPNOW значений*/
struct_Send.cod = 1;
struct_Send.situation = 0;

/*Инициализация Bluetooth*/
SerialBT.begin(board_name);

/*Инициализация Telnet*/
TelnetServer.begin();
TelnetServer.setNoDelay(true);

/*Назначение функций и параметров OTA*/
ArduinoOTA.setHostname(board_name);
ArduinoOTA.onStart(TimerStop);

/*Назначение функций и параметров MQTT*/
mqttClient.onConnect(mqtt_Connected_Complete);
mqttClient.onDisconnect(mqtt_Disconnect_Complete);
mqttClient.onSubscribe(mqtt_Subscribe_Complete);
mqttClient.onUnsubscribe(mqtt_Unsubscribe_Complete);
mqttClient.onMessage(mqtt_Receiving_Complete);
mqttClient.onPublish(mqtt_Publishe_Complete);
mqttClient.setServer(mqtt_host, mqtt_port);
mqttClient.setCredentials(mqtt_username, mqtt_pass);
mqttClient.setClientId(board_name);

/*Получение времени с NTP сервера*/
GetTime();

/*Инициализация WDT*/
timerWDT = timerBegin(0, 80, true);
timerAttachInterrupt(timerWDT, &ResetModule, true);
timerAlarmWrite(timerWDT, 30000 * 1000, false);
timerAlarmEnable(timerWDT);

/*Отправка сообщения о перезагрузке*/

IFTTTSend (String(ifttt_event), String(board_name), String("Res:") + String (memory.getInt("countReset")), String("   WiFi:") + String (memory.getInt("countWifi")));

/* ---------- Sygnal System ---------- */

/*Настройка встроенного светодиода*/
pinMode(2, OUTPUT);

/*SIM800*/
Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
pinMode(SIM800_PWR, OUTPUT);
//Высокий уровень на RST SIM800
digitalWrite(SIM800_PWR, HIGH);
//AT для настройки скорости обмена данными
sendATCommand("AT", true);
//Отключить эхо, включить текстовый формат ответов, включить Text mode SMS, сохранить настройки (AT&W)
sendATCommand("ATE1V1+CMGF=1;&W", true);
//Удалить все сообщения, чтобы не забивали память модуля
sendATCommand("AT+CMGDA=\"DEL ALL\"", true);

//Случайное число в строку проверки
string_check_random = String(random(100, 1000));

//Инициализаця INA219
ina219.begin();

//Загрузка таблицы из NVS
memory.getBytes("mem", (detector*)detector_ram, sizeof(detector_ram));

//Включение приемника 433 МГц
receiver.enableReceive(RECEIVER_PIN);

//Установка флага питания от сети, установки пинов тревоги
pinMode(POWER_PIN, INPUT);
flag_power_220 = digitalRead(POWER_PIN);

pinMode(ALARM_RELAY_PIN_1, OUTPUT);
pinMode(ALARM_RELAY_PIN_2, OUTPUT);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);

/* ---------- Sygnal System ---------- */


/*Запуск таймеров*/
xTimerStart(timerBigCycleFunction, 0);

xTimerStart(timerESPNowSend, 0);

xTimerStart(timerMqttSend, 0);
}
