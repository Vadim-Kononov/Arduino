/*Вывод в терминалы Serial, WiFiSerial, BTSerial*/
void Write(String string, uint8_t terminal)
{
/*B001 Serial | B010 WiFi | B100 BT*/
if bitRead(terminal, 0) DEBUG_PRINT(string);
if bitRead(terminal, 1) TelnetClient.print(string);
if bitRead(terminal, 2) SerialBT.print(string);
}
/**/

/*Вывод в терминалы Serial, WiFiSerial, BTSerial*/
void Writeln(String string, uint8_t terminal)
{
/*B001 Serial | B010 WiFi | B100 BT*/
if bitRead(terminal, 0) DEBUG_PRINTLN(string);
if bitRead(terminal, 1) TelnetClient.println(string);
if bitRead(terminal, 2) SerialBT.println(string);
}
/**/

//Вывод записанных в память датчиков
void print_Detector()
{
memory.getBytes("mem", (detector*)detector_ram, sizeof(detector_ram));
Writeln("", B110);
for (int i=0; i<35; i++) {Writeln(String(detector_ram[i].number) + "  " + String(detector_ram[i].type) + "  " + String(detector_ram[i].code, HEX) + "  " +  String(detector_ram[i].title), B110); Serial.flush(); TelnetClient.flush(); SerialBT.flush();}
Writeln ("> ", B110);
}

/*Обработка строки принятой через Telnet или Bluetooth*/
void Terminal(String string, uint8_t terminal)
{
String string_1 = "", string_2 = "", string_3 = "" , string_4 = "";

/*Парсинг входой строки на 3 элемента по "/"*/
if (string.indexOf("/") >=0) {string_1 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_1 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_2 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_2 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_3 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_3 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_4 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_4 = string; string.remove(0, string.length());}

/* Если был введен пароль*/
if (flag_entry_password)
{

/* "Пароль". Отключение пароля, если он включен. Режим триггера*/
if (string_1.equalsIgnoreCase("cbhLtpjr")) {flag_entry_password = false; Writeln ("< ", terminal); return;}

/* "Login". Запись в память логина и пароля WiFi*/
else if (string_1.equalsIgnoreCase("Login")) {memory.putString("login", string_2); memory.putString("pass", string_3); Writeln("> " + memory.getString("login") + " " + memory.getString("pass"), terminal);}

/* "Save". Вывод логина и пароля WiFi из памяти*/
else if (string_1.equalsIgnoreCase("Save")) {Writeln("> Login: " + memory.getString("login") + " | Pass: " + memory.getString("pass"), terminal);}

/* "Reconnect". Переподключение WiFi по логину и паролю хранящимися в памяти*/
else if (string_1.equalsIgnoreCase("Reconnect"))
{
Writeln ("> Reconnect WiFi to: " + memory.getString("login"), terminal);
WiFi.disconnect(); WiFiConnect();
}

/* "Count". Вывод счетчиков перезагрузок и реконнектов*/
else if (string_1.equalsIgnoreCase("Count")) Writeln("> Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi")), terminal);

/* "CountR". Сброс счетчиков перезагрузок и реконнектов*/
else if (string_1.equalsIgnoreCase("CountR")) {memory.putInt("countReset", 0); memory.putInt("countWifi", 0); Writeln("> Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi")), terminal);}

/* "WiFi". Вывод текущих параметров WiFi*/
else if (string_1.equalsIgnoreCase("WiFi")) {Writeln("> " + String(WiFi.SSID()) + " : " + WiFi.channel() + " (" + String(WiFi.RSSI()) + ") " + WiFi.localIP().toString(), terminal);}

/* "Scan". Сканирование WiFi*/
else if (string_1.equalsIgnoreCase("Scan")) {
Writeln("> Scan: ", terminal);
int n = WiFi.scanNetworks();
String str = "";
if (n == 0) {Writeln (">Сети не найдены", terminal);}
else {for (int i = 0; i < n; ++i) {str += String(i + 1); str += ". "; str += WiFi.SSID(i); str += " : "; str +=  WiFi.channel(i); str += " (";  str += WiFi.RSSI(i); str += ") \n";} Write(str, terminal);}
}

/* "Time". Вывод текущего времени, обращение к элементам структуры: timeinfo.tm_sec|timeinfo.tm_min|timeinfo.tm_hour и т.д., тип - int*/
else if (string_1.equalsIgnoreCase("Time")) {char time_str[50]; GetTime(); strftime(time_str, 50, "%A %d %b %Y %H:%M:%S", &timeinfo); Writeln("> " + String(time_str), terminal);}

/* "Mem". Вывод количества свободной памяти*/
else if (string_1.equalsIgnoreCase("Mem")) {Writeln ("> Free memory: " + String(ESP.getFreeHeap()), terminal);}

/* "OTA". Разрешение загрузки по воздуху*/
else if (string_1.equalsIgnoreCase("OTA")) {ArduinoOTA.begin(); Writeln ("> OTA On: " + String(ESP.getFreeSketchSpace()), terminal); flag_activation_OTA = true;}

//"Write". Запись в память данных датчика
else if (string_1.equalsIgnoreCase("Write"))
{
number = string_2.toInt(); type = string_3.toInt(); title = string_4;
if ((number>=0 && number<=255) && (type>=0 && type<=255) && (title.length() >=1 && title.length() <=48)) {flag_code_save = true; Writeln ("!!! Wait !!!", terminal);}
}

//"Read". Чтение записанных датчиков
else if (string_1.equalsIgnoreCase("Read")) print_Detector();

//"Clear". Стирание записанного датчика
else if (string_1.equalsIgnoreCase("Clear"))
{
number = string_2.toInt();
if (number>=0 && number<=255)
{
detector_ram[number].number = 0; detector_ram[number].type = 0; detector_ram[number].code = 0; title = "0"; title.toCharArray(detector_ram[number].title, 50);
memory.putBytes("mem", (detector*)detector_ram, sizeof(detector_ram));
print_Detector();
}
}

//"On". Включение сигнализации
else if (string_1.equalsIgnoreCase("On")) {ON(); Writeln (">On", terminal);}

//"Off". Отключение сигнализации
else if (string_1.equalsIgnoreCase("Off")) {OFF(); Writeln (">Off", terminal);}

//"Alarm On". Включение сирены
else if (string_1.equalsIgnoreCase("Alarm On")) {AlarmON(); Writeln (">Alarm On", terminal);}

//"Alarm Off". Отключение сирены
else if (string_1.equalsIgnoreCase("Alarm Off")) {AlarmOFF(); Writeln (">Alarm Off", terminal);}

//"INA". Показания INA219
else if (string_1.equalsIgnoreCase("INA")) {Writeln (">" + String(ina219.getBusVoltage_V()) + " V   |   " + String(ina219.getShuntVoltage_mV()) + " mV   |   " + String(ina219.getCurrent_mA()) + " mA", terminal);}

/*"SIM". Команды SIM800 вида SIM/<Команда>*/
else if (string_1.equalsIgnoreCase("SIM")) {sendATCommand(string_2, true);}

/* "Help".*/
else if (string_1.equalsIgnoreCase("?") || string_1.equalsIgnoreCase("Help"))
{Writeln (">\n| Login/<SSID>/<Password> |\n| Write/<Номер>/<Тип>/<Описание> |\n| Read | Clear/<Номер> | Save |\n| Mem | WiFi | Scan | Reconnect |\n| Count | CountR | Time | Reset |\n| OTA0 | ОТА1 | SIM/<Команда> |\n| On | Off | Alarm On | Alarm Off | INA |\n| ? | Help |\n>", terminal);}

/* Все остальное*/
else
{
Writeln ("1>/" + string_1, terminal);
Writeln ("2>/" + string_2, terminal);
Writeln ("3>/" + string_3, terminal);
Writeln ("4>/" + string_4, terminal);
Writeln (">>/" + string, terminal);
}
}

/* Обработка пароля после перезагрузки*/
if (!flag_entry_password) {if (!string_1.compareTo("cbhLtpjr")) {flag_entry_password = true; Writeln ("> ", terminal);} else Writeln ("Пароль > ", terminal);}



}
