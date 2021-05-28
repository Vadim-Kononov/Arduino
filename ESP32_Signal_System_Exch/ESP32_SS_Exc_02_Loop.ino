void loop()
{
/* ---------- OTA Telnet Bluetooth  ---------- */

if (flag_activation_OTA) ArduinoOTA.handle();

/*Проверка подключения клиентов Telnet*/
if (TelnetServer.hasClient())
{
  if (!TelnetClient || !TelnetClient.connected())
  {
     if(TelnetClient) TelnetClient.stop();
     TelnetClient = TelnetServer.available();
     TelnetClient.println("My IP: " + WiFi.localIP().toString() + "   |   Your IP: " + TelnetClient.remoteIP().toString());
     flag_activation_OTA = false;
  }
  else TelnetServer.available().stop();
}

/*Получение строки из Telnet, при ее наличии. Отправка строки в Terminal для обработки*/
if (TelnetClient && TelnetClient.connected() && TelnetClient.available()) {while(TelnetClient.available()) Terminal(TelnetClient.readStringUntil('\n'), B010);}

/*Получение строки из Bluetooth, при ее наличии. Отправка строки в Terminal для обработки*/
if (SerialBT.available()) Terminal(SerialBT.readStringUntil('\n'), B100);



/* ---------- Sygnal System ---------- */


//Код может быть получен в одном экземляре после его изменения, следующий прием неизменного кода через timerResetCode
if (receiver.available())
{
if (code != receiver.getReceivedValue()){code = receiver.getReceivedValue(); flag_code_receive = true; xTimerStart(timerResetCode,0);}
receiver.resetAvailable();
}


//Запись принятого кода в NVS, если в Terminal установлен flag_code_save и код пришел новый
if (flag_code_receive && flag_code_save)
{
//Запрет дальнейшей записи кода, до следующей установки flag_code_save
flag_code_receive = false; flag_code_save = false;
//Запись элементов структуры
detector_ram[number].number = number; detector_ram[number].type = type; detector_ram[number].code = code; title.toCharArray(detector_ram[number].title, 50);
//Запись в NVS
memory.putBytes("mem", (detector*)detector_ram, sizeof(detector_ram));
//Вывод в терминалы таблицы кодов
print_Detector();

//Отправка сообщения о записи кода датчика
if (flag_WiFi_connected)
IFTTTSend (String(ifttt_event), "There was a sensor code entry", "", "");
}

//При изменении кода и flag_code_save = false отправить FTTT сообщение и SMS сообщение с кодом и наименованием сработавшего датчика при включенной охране
if (flag_code_receive && !flag_code_save)
{
flag_code_receive = false;
int detector_type; 
//Определение наименования сработавшего датчика и вывод информации в терминал
for (int i=0; i<35; i++) {if (detector_ram[i].number == i && detector_ram[i].code == code) {detector_type = detector_ram[i].type; detector_title = detector_ram[i].title;}}
Writeln (">> " + String(code, HEX) + "  " + detector_title, B110);
// detector_type содержит номер группы сработавшего датчика
switch (detector_type) 
{
  case 1: 
    if        (detector_title.endsWith("#1")) OFF();
    else if   (detector_title.endsWith("#2")) ON();
    else if   (detector_title.endsWith("#3")) ON_Perimeter();
    else if   (detector_title.endsWith("#4")) struct_Send.situation = 20;
    break;
  case 2:
    if        (detector_title.endsWith("#10")) {AlarmON();}
    else if   (detector_title.endsWith("#20")) {AlarmOFF();}
    break;
  case 10:
  case 20:
  case 30:
  if (flag_activation_alarm || (flag_perimeter_alarm && detector_type!=10)) 
  {
    struct_Send.situation = 10;
    if (!IFTTTSend(String(ifttt_event), detector_title, "", "")) {sendSMS(my_number, detector_title); sendSMS(boris_number, detector_title); sendSMS(gleb_number, detector_title);}
    AlarmRelayOn();
    xTimerChangePeriod(timerRelayOff_1, pdMS_TO_TICKS(250), 0);
    /*Установка флага срабатывания сигнализации*/
    flag_alarm_triggered = true;
  }
}
}



//Проверка питания от сети и отправка SMS при при его появлении-исчезновении
if (flag_power_220 != digitalRead(POWER_PIN))
{
  flag_power_220 = digitalRead(POWER_PIN);
  String str;
  if (flag_power_220) str = "Power 220V OFF"; else str = "Power 220V ON";
  sendSMS(my_number, str); sendSMS(boris_number, str); sendSMS(gleb_number, str);
}



//Чтение с SIM800
if (Serial2.available())
{
//Получаем ответ
respond_sim800 = waitRespond();
respond_sim800.trim();
//Если пришло SMS
if (respond_sim800.startsWith("+CMTI:"))
    {
    int index = respond_sim800.lastIndexOf(",");                                    //Находим последнюю запятую, перед индексом
    String result = respond_sim800.substring(index + 1, respond_sim800.length());   //Получаем индекс
    result.trim();                                                                  //Убираем пробелы в начале/конце
    respond_sim800=sendATCommand("AT+CMGR="+result, true);                          //Содержимое SMS
    analysisSMS(respond_sim800);                                                    //Парсинг SMS на элементы
    }
}



//Операции с частотой BigCycleFunction
if(flag_BigCycleFunction)
{
flag_BigCycleFunction = false;
/*Переподключение к WiFi*/
WiFiReconnect ();
/*Опрос SIM800, при отсутсвии ответа - сигнал сброса*/
if (sendATCommand("AT", true).equalsIgnoreCase("Timeout")) {digitalWrite(SIM800_PWR, LOW); xTimerStart(timerCheckeSim800,0); DEBUG_PRINTLN("Time:\t" + String(xTaskGetTickCount()/1000.0) + "\t" + "CheckeSim800 = LOW");}
}



}

/*Обнуление принятого кода, вызывается таймером timerResetCode*/
void ResetCode ()
{
/*Переменная с кодом обнуляется каждые две секунды если не было срабатываний, для приема повтрных срабатываний с повторяющимся кодом*/
if (!flag_code_receive) code = 0;
}
/**/

/*Установка в 1 обнуленного сигнала сброса Sim800*/
void CheckeSim800 ()
{
digitalWrite(SIM800_PWR, HIGH);
}
/**/
