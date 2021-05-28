void loop()
{
//Разрешение подсчетов реконнектов WiFI (без учета перезагрузок)
res_flag = true;

//Запуск OTA
ArduinoOTA.handle();

//Запуск MQTT
client.loop();

//Запуск WiFiSerial, если позволяет флаг
if (wt_ota_flag)
{
//WiFiSerial проверка подключения клиентов
    if (TelnetServer.hasClient())
    {
        if (!TelnetClient || !TelnetClient.connected())
        {
        if(TelnetClient) TelnetClient.stop();
        TelnetClient = TelnetServer.available();
        TelnetClient.println("> Your IP: " + TelnetClient.remoteIP().toString());
        }
        else TelnetServer.available().stop();      
    }

//WiFiSerial получение строки, если она есть. Вызов Terminal для обработки с параметром "WiFi"
    if (TelnetClient && TelnetClient.connected() && TelnetClient.available()) {while(TelnetClient.available()) Terminal(TelnetClient.readStringUntil('\n'), "WiFi");}
}

//BTSerial получение строки, если она есть. Вызов Terminal для обработки с параметром "BT"
if (SerialBT.available()) Terminal(SerialBT.readStringUntil('\n'), "BT");

//------------------------------------------Signal System------------------------------------------//
//Код может быть получен в одном экземляре после его изменения
if (receiver.available())
{
if (code != receiver.getReceivedValue()){code = receiver.getReceivedValue(); code_flag = true; count_ms_Code = millis();}
receiver.resetAvailable();
}
//Переменная с кодом обнуляется каждые две секунды если не было срабатываний, для приема повтрных срабатываний с одним кодом
if (millis() - count_ms_Code > time_ms_Code && !code_flag) {count_ms_Code = millis(); code = 0;}

//Запись принятого кода, если в Terminal установлен dev_flag и код пришел новый после таймаута count_ms_Code
if (code_flag && dev_flag)
{
//Запрет дальнейшей записи кода, до следующей установки dev_flag
code_flag = false; dev_flag = false;
//Запись элементов структуры
detect_buf[number].number = number; detect_buf[number].type = type; detect_buf[number].code = code; title.toCharArray(detect_buf[number].title, 50);
//Запись в EEPROM
memory.putBytes("mem", (detector*)detect_buf, sizeof(detect_buf));
//Вывод в терминалы таблицы кодов
print_sensor("WiFi"); print_sensor("BT");

//Отправка сообщения о записи кода датчика
SendIFTTT(KEY, EVENT_NAME3, "", "There was a sensor code entry", "");
}

//При изменении кода отправить FTTT сообщение и SMS сообщение с кодом и наименованием сработавшего датчика при включенной охране
if (code_flag && !dev_flag)
{
code_flag = false;
int k; String str;
//Определение наименования сработавшего датчика
for (int i=0; i<35; i++) {if (detect_buf[i].number == i && detect_buf[i].code == code) {k = detect_buf[i].type; str = detect_buf[i].title;}}
Out (">> " + String(code, HEX) + "  " + str, "WiFi"); Out (">> " + String(code, HEX) + "  " + str, "BT"); 

switch (k) 
{
  case 1: 
    if        (str.endsWith("#1")) OFF();
    else if   (str.endsWith("#2")) ON();
    else if   (str.endsWith("#3")) ON_Perimeter();
    else if   (str.endsWith("#4")) bitWrite(situation, 2, !bitRead(situation, 2));
    break;
  case 2:
    if        (str.endsWith("#10")) {AlarmON();}
    else if   (str.endsWith("#20")) {AlarmOFF();}
    break;
  case 10:
  case 20:
  case 30:
  if (on_off_flag || (on_off_perimeter_flag && k!=10)) 
  {
    bitWrite(situation, 3, !bitRead(situation, 3));
    if (!SendIFTTT(KEY, EVENT_NAME3, "", str, "")) {sendSMS(my_number, str); sendSMS(boris_number, str); sendSMS(gleb_number, str);}
    digitalWrite(ALARM_RELAY_PIN_1, LOW);
    digitalWrite(ALARM_RELAY_PIN_2, LOW);
    delay (250);
    digitalWrite(ALARM_RELAY_PIN_1, HIGH);
    digitalWrite(ALARM_RELAY_PIN_2, HIGH);
    alarm_operation_flag = true;
  }
}
}

//Проверка питания от сети и отправка SMS при при его появлении-исчезновении
if (power_flag != digitalRead(POWER_PIN))
{
  power_flag = digitalRead(POWER_PIN);
  String str;
  if (power_flag) str = "Voltage 220V OFF"; else str = "Voltage 220V ON";
  sendSMS(my_number, str); sendSMS(boris_number, str); sendSMS(gleb_number, str);
}

//Таймер сирены
if (millis() - count_ms_TimerAlarm > time_ms_TimerAlarm && siren_on_flag)
{
siren_on_flag = false;
digitalWrite(ALARM_RELAY_PIN_1, HIGH); digitalWrite(ALARM_RELAY_PIN_2, HIGH); sendSMS(my_number, "The Siren is OFF"); sendSMS(boris_number, "The Siren is OFF"); sendSMS(gleb_number, "The Siren is OFF");
}
//------------------------------------------Signal System------------------------------------------//

//Таймер 05 сек
if (millis() - count_ms_Timer05 > time_ms_Timer05)
{
count_ms_Timer05 = millis();
udp.write (situation);
}

//Таймер 5 сек
if (millis() - count_ms_Timer5 > time_ms_Timer5)
{
count_ms_Timer5 = millis();
//Переподключение при обрыве связи MQTT
if (!client.connected() && WiFi.status() == WL_CONNECTED && !power_flag) MQTT_reconnect();

//------------------------------------------SIM800------------------------------------------//
//Каждые 5 секунд отключение возможного сброса SIM800
digitalWrite(SIM800_PWR, HIGH);
//------------------------------------------SIM800------------------------------------------//
}

//Таймер 60 сек
if (millis() - count_ms_Timer60 > time_ms_Timer60)
{
count_ms_Timer60 = millis();
//Переподключение при обрыве связи WiFi
if ((WiFi.status() != WL_CONNECTED) && !power_flag) WiFi_reconnect();

//Обновление времени с NTP
GetTime();
//Опрос SIM800, при отсутсвии ответа - сигнал сброса
if (sendATCommand("AT", true).equalsIgnoreCase("Timeout")) digitalWrite(SIM800_PWR, LOW);
}

//------------------------------------------SIM800------------------------------------------//
//Чтение с SIM800
if (Serial2.available())
{
//Получаем ответ
respond = waitRespond();
respond.trim();
//Если пришло SMS
if (respond.startsWith("+CMTI:"))
    {
    int index = respond.lastIndexOf(",");                           //Находим последнюю запятую, перед индексом
    String result = respond.substring(index + 1, respond.length()); //Получаем индекс
    result.trim();                                                  //Убираем пробелы в начале/конце
    respond=sendATCommand("AT+CMGR="+result, true);                 //Содержимое SMS
    analysisSMS(respond);                                           //Парсинг SMS на элементы
    }
  }
//------------------------------------------SIM800------------------------------------------//
//Сброс сторожевого таймера
timerWrite(timer, 0);
}



//------------------------------------------Signal System------------------------------------------//

//Процедура включения сигнализации
void ON()
{
//Отправка UDP сообщения
bitSet(situation, 1);
on_off_flag = true; on_off_perimeter_flag = false;
alarm_operation_flag = false;
digitalWrite(ALARM_RELAY_PIN_1, LOW);
digitalWrite(ALARM_RELAY_PIN_2, LOW);
delay (75);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);
if (!SendIFTTT(KEY, EVENT_NAME3, "", "Signal System ON", "")) {sendSMS(my_number, "Signal System ON"); sendSMS(boris_number, "Signal System ON"); sendSMS(gleb_number, "Signal System ON");}
}



//Процедура включения сигнализации по периметру
void ON_Perimeter()
{
//Отправка UDP сообщения
bitSet(situation, 1);
on_off_flag = false; on_off_perimeter_flag = true;
alarm_operation_flag = false;
digitalWrite(ALARM_RELAY_PIN_1, LOW);
digitalWrite(ALARM_RELAY_PIN_2, LOW);
delay (35);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);
if (!SendIFTTT(KEY, EVENT_NAME3, "", "Signal System Perimeter ON", "")) {sendSMS(my_number, "Signal System Perimeter ON"); sendSMS(boris_number, "Signal System Perimeter ON"); sendSMS(gleb_number, "Signal System Perimeter ON");}
}



//Процедура отключения сигнализации
void OFF()
{
//Отправка UDP сообщения
bitClear(situation, 1);
on_off_flag = false; on_off_perimeter_flag = false;
digitalWrite(ALARM_RELAY_PIN_1, LOW);
delay (75);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
delay (150);
digitalWrite(ALARM_RELAY_PIN_2, LOW);
delay (75);
digitalWrite(ALARM_RELAY_PIN_2, HIGH); 
if (alarm_operation_flag)
{
delay (150);
digitalWrite(ALARM_RELAY_PIN_1, LOW);
digitalWrite(ALARM_RELAY_PIN_2, LOW);
delay (75);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);
}
if (!SendIFTTT(KEY, EVENT_NAME3, "", "Signal System OFF", "")) {sendSMS(my_number, "Signal System OFF"); sendSMS(boris_number, "Signal System OFF"); sendSMS(gleb_number, "Signal System OFF");}
}



//Процедура включения сирены
void AlarmON()
{
bitSet(situation, 0);
digitalWrite(ALARM_RELAY_PIN_1, LOW);
digitalWrite(ALARM_RELAY_PIN_2, LOW);
siren_on_flag = true;
count_ms_TimerAlarm = millis();
}



//Процедура отключения сирены
void AlarmOFF()
{
bitClear(situation, 0);
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);
siren_on_flag = false;
}
//------------------------------------------Signal System------------------------------------------//
