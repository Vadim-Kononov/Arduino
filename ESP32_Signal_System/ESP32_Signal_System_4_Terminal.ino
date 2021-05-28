//Обработка принятых строк, параметры строка и строковое имя терминала от которого пришел запрос
void Terminal(String string, String terminal)
{
String string_0, string_1 = "", string_2 = "", string_3  = "" , string_4 = "", string_5 = "";

//Парсинг входой строки на 5 элементов по "/"
string_0 = string;
if (string.indexOf("/") >=0) {string_1 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_1 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_2 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_2 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_3 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_3 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_4 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_4 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_5 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_5 = string; string.remove(0, string.length());}


//Если был введен пароль
if (key_flag)
{

//"Пароль". Отключение пароля, если он включен. Режим триггера
if (string_1.equalsIgnoreCase("cbhLtpjr")) {key_flag = false; Out ("< ", terminal); return;}

//"Login". Запись в память логина и пароля WiFi
else if (string_1.equalsIgnoreCase("Login")) {memory.putString("login", string_2); memory.putString("pass", string_3); Out("> " + memory.getString("login") + " " + memory.getString("pass"), terminal);}

//"Time". Вывод текущего времени, обращение к элементам структуры: timeinfo.tm_sec|timeinfo.tm_min|timeinfo.tm_hour и т.д., тип - int
else if (string_1.equalsIgnoreCase("Time")) {char time_str[50]; GetTime(); strftime(time_str, 50, "%A %d %b %Y %H:%M:%S", &timeinfo); Out("> " + String(time_str), terminal);}

//"Reset". Перезагрузка модуля
else if (string_1.equalsIgnoreCase("Reset")) {Out("> Reset", terminal); delay (250); ESP.restart();}

//"Count". Вывод счетчиков перезагрузок и реконнектов
else if (string_1.equalsIgnoreCase("Count")) Out("> Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi")), terminal);

//"CountR". Сброс счетчиков перезагрузок и реконнектов
else if (string_1.equalsIgnoreCase("CountR")) {memory.putInt("countReset", 0); memory.putInt("countWifi", 0); Out("> Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi")), terminal);}

//"Save". Вывод логина и пароля WiFi из памяти
else if (string_1.equalsIgnoreCase("Save")) {Out("> Login: " + memory.getString("login") + " | Pass: " + memory.getString("pass"), terminal);}

//"Mem". Вывод количества свободной памяти
else if (string_1.equalsIgnoreCase("Mem")) {Out ("> Free memory: " + String(ESP.getFreeHeap()), terminal);}

//"Reconnect". Переподключение WiFi по логину и паролю хранящимися в памяти
else if (string_1.equalsIgnoreCase("Reconnect"))
{
memory.getString("login").toCharArray(ssid, memory.getString("login").length() + 1);
memory.getString("pass").toCharArray(password, memory.getString("pass").length() + 1);
WiFi.disconnect(); WiFi_reconnect(); Out("> Reconnect to " + WiFi.SSID(), terminal);
}

//"WiFi". Вывод текущих параметров WiFi
else if (string_1.equalsIgnoreCase("WiFi")) {Out("> " + String(WiFi.SSID()) + " " + WiFi.localIP().toString()+ " " + String(WiFi.RSSI()), terminal);}

//"Scan". Сканирование WiFi
else if (string_1.equalsIgnoreCase("Scan")) {
Out("> Scan: ", terminal);
WiFiScan ();
}

//"OTA1". Разрешение, запрет работы WiFiSerial терминала для обеспечения работы ArduinoOTA
else if (string_1.equalsIgnoreCase("OTA0")) {wt_ota_flag = true; Out("> WiFiSerial On | OТА Off", terminal);}
else if (string_1.equalsIgnoreCase("OTA1")) {wt_ota_flag = false; Out("> WiFiSerial Off | OТА On", terminal);}

//"Write". Запись в память данных датчика
else if (string_1.equalsIgnoreCase("Write"))
{
number = string_2.toInt(); type = string_3.toInt(); title = string_4;
if ((number>=0 && number<=255) && (type>=0 && type<=255) && (title.length() >=1 && title.length() <=48)) {dev_flag = true; Out ("!!! Wait !!!", terminal);}
}

//"Read". Чтение записанных датчиков
else if (string_1.equalsIgnoreCase("Read")) print_sensor(terminal);

//"Clear". Стирание записанного датчика
else if (string_1.equalsIgnoreCase("Clear"))
{
number = string_2.toInt();
if (number>=0 && number<=255)
{
detect_buf[number].number = 0; detect_buf[number].type = 0; detect_buf[number].code = 0; title = "0"; title.toCharArray(detect_buf[number].title, 50);
memory.putBytes("mem", (detector*)detect_buf, sizeof(detect_buf));
print_sensor(terminal);
}
}

//"On". Включение сигнализации
else if (string_1.equalsIgnoreCase("On")) {ON(); Out (">On", terminal);}

//"Off". Отключение сигнализации
else if (string_1.equalsIgnoreCase("Off")) {OFF(); Out (">Off", terminal);}

//"Alarm On". Включение сирены
else if (string_1.equalsIgnoreCase("Alarm On")) {AlarmON(); Out (">Alarm On", terminal);}

//"Alarm Off". Отключение сирены
else if (string_1.equalsIgnoreCase("Alarm Off")) {AlarmOFF(); Out (">Alarm Off", terminal);}

//"INA". Показания INA219
else if (string_1.equalsIgnoreCase("INA")) {Out (">" + String(ina219.getBusVoltage_V()) + " V   |   " + String(ina219.getShuntVoltage_mV()) + " mV   |   " + String(ina219.getCurrent_mA()) + " mA", "WiFi");}


//"Help".
else if (string_1.equalsIgnoreCase("?") || string_1.equalsIgnoreCase("Help"))
{Out (">\n| Login/<SSID>/<Password> |\n| Write/<Номер>/<Тип>/<Описание> |\n| Read | Clear/<Номер> | Save |\n| Mem | WiFi | Scan | Reconnect |\n| Count | CountR | Time | Reset |\n| OTA0 | ОТА1 | SIM/<Команда> |\n| On | Off | Alarm On | Alarm Off | INA |\n| ? | Help |\n>", terminal);}

//"SIM". Команды SIM800 вида SIM/<Команда>
else if (string_1.equalsIgnoreCase("SIM")) {sendATCommand(string_2, true);}

//Все остальное
else
{
Out ("1>" + string_1 + "<", terminal);
Out ("2>" + string_2 + "<", terminal);
Out ("3>" + string_3 + "<", terminal);
Out ("4>" + string_4 + "<", terminal);
Out ("5>" + string_5 + "<", terminal);
Out ("> ? " + string_0 + " ?", terminal);
}
}



//Обработка пароля при первом после перезагрузке входе
if (!key_flag) {if (!string_1.compareTo("cbhLtpjr")) {key_flag = true; Out ("> ", terminal);} else Out ("Пароль > ", terminal);}
}



//Вывод записанных в память датчиков
void print_sensor(String terminal)
{
memory.getBytes("mem", (detector*)detect_buf, sizeof(detect_buf));
Out("", terminal);
for (int i=0; i<35; i++) {Out(String(detect_buf[i].number) + "  " + String(detect_buf[i].type) + "  " + String(detect_buf[i].code, HEX) + "  " +  String(detect_buf[i].title), terminal); Serial.flush(); TelnetClient.flush(); SerialBT.flush();}
Out ("> ", terminal);
}



// Вывод в терминалы Serial, WiFiSerial, BTSerial
void Out(String string, String terminal)
{
if      (terminal.equalsIgnoreCase("WiFi"))   {if (TelnetClient && TelnetClient.connected()) TelnetClient.println (string);}
else if (terminal.equalsIgnoreCase("BT"))     {if (SerialBT.hasClient()) SerialBT.println (string);}
}
