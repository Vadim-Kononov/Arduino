void setup()
{
//Отключение brownout detector
WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

#ifdef Serial_On
Serial.begin(115200);
Serial.println("Booting");
#endif

//Настройка встроенного светодиода
pinMode(Onboard_Led, OUTPUT);

//Инициализация EEPROM
memory.begin("memory", false);

//Подключение к WiFi
WiFi_reconnect();

//Подключение к Telnet WiFiSerial
TelnetServer.begin();
TelnetServer.setNoDelay(true);

//UDP Multicast рассылка для группы 239.0.0.0, порт 59900
udp.listenMulticast(IPAddress(239,0,0,0), 59900);

//Подключение к BTSerial
SerialBT.begin(NAME);

//Подключение к MQTT
client.setServer(mqtt_server, mqtt_port);
client.setCallback(callback);

//Формирование случайного числа для логина
randomSeed(micros());

//Подключение к OTA
ArduinoOTA.setHostname(NAME);
ArduinoOTA.begin();

//Настройка WDT
timer = timerBegin(0, 80, true);                  //Timer 0, div 80
timerAttachInterrupt(timer, &ResetModule, true);  //Attach callback
timerAlarmWrite(timer, wdtTimeout * 1000, false); //Set time in us
timerAlarmEnable(timer);                          //Enable interrupt

//Получение времени с NTP сервера
digitalWrite(Onboard_Led, HIGH);
GetTime();
digitalWrite(Onboard_Led, LOW);

//Увеличение счетчика перезагрузок на 1
memory.putInt("countReset", memory.getInt("countReset") + 1);

//Отправка сообщения о перезагрузке
SendIFTTT(KEY, EVENT_NAME1, NAME + String(" >") + String (memory.getInt("countReset")) + String("<"), print_reset_reason(rtc_get_reset_reason(0)), print_reset_reason(rtc_get_reset_reason(1)));

//Установка таймеров 60, 5 и 0.25 сек
count_ms_Timer60 = millis(); time_ms_Timer60 = 60000;
count_ms_Timer5 = millis(); time_ms_Timer5 = 5000;
count_ms_Timer05 = millis(); time_ms_Timer05 = 500;

//------------------------------------------SIM800------------------------------------------//
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
check_string = String(random(100, 1000));
//------------------------------------------SIM800------------------------------------------//

//------------------------------------------Signal System------------------------------------------//
//Инициализаця INA219
ina219.begin();

//Установка таймера сирены
count_ms_TimerAlarm = millis(); time_ms_TimerAlarm = 1800000;

//Установка 3 сек. задержки на получение кода
time_ms_Code = 3000;
//Загрузка таблицы из EEPROM
memory.getBytes("mem", (detector*)detect_buf, sizeof(detect_buf));
//Включение приемника 433 МГц
receiver.enableReceive(RECEIVER_PIN);
//Установка флага питания от сети, установки пинов тревоги
pinMode(POWER_PIN, INPUT_PULLUP);
power_flag = digitalRead(POWER_PIN);
pinMode(ALARM_RELAY_PIN_1, OUTPUT);
pinMode(ALARM_RELAY_PIN_2, OUTPUT);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);
//------------------------------------------Signal System------------------------------------------//
}
