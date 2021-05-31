/*Функция Telnet терминала*/
void Telnet(void *parameter)
{
	for(;;)
	{
		/*Подключения клиента Telnet*/
		if (TelnetServer.hasClient())
		{
			if(TelnetClient) TelnetClient.stop();
			TelnetClient = TelnetServer.available();
			String str = "> " + String(board_name) + "\n> IP: " + WiFi.localIP().toString() + "  |  MAC: " +  WiFi.macAddress() + "\n> ";
			TelnetClient.print(str);
		}
		/*Получение строки из Telnet, при ее наличии, обработка*/
		if (TelnetClient && TelnetClient.connected() && TelnetClient.available())
		{
		String str = TelnetClient.readStringUntil('\n');
		TelnetClient.println (Terminal(str));
		TelnetClient.print ("> ");
		log_i();
		}
	}
}



/*Функция загрузки по OTA*/
void OTA(void *parameter)
{
  for(;;)
  {
  ArduinoOTA.handle();  
  }
}



/*Функция задержки*/
void Waiting(uint16_t lag)
{
uint32_t time_last = xTaskGetTickCount();  
while (xTaskGetTickCount() - time_last < lag) {}
}


/*Функция переподключения Wifi*/
void Recon()
{
	WiFi.disconnect();
	memory.putInt("countWifi", memory.getInt("countWifi") + 1);
	WiFi.begin(ssid, pass);
}



/*Функция перезагрузки модуля*/
void Reset()
{
	ESP.restart();	
}



/*Обработка строки принятой через Telnet или Bluetooth*/
String Terminal(String &string)
{
String string_1 = "", string_2 = "", string_3 = "";

/*Парсинг входой строки на 3 элемента по "/"*/
if (string.indexOf("/") >=0) {string_1 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_1 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_2 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_2 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_3 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_3 = string; string.remove(0, string.length());}

if (flag_Terminal_pass)
{
/* "Password". Отключение пароля, если он включен. Режим триггера*/
if (string_1.equalsIgnoreCase("cbhLtpjr")) {flag_Terminal_pass = false; return "Password";}

/* "Scan". Сканирование WiFi*/
else if (string_1.equalsIgnoreCase("Scan"))
{
String str = "Scan: ";
int n = WiFi.scanNetworks();
if (n == 0) return str += "Сети не найдены";
else 
{
	str += "\n> ----------------------------------------------";
	for (int i = 0; i < n; ++i)
	{	
		str += "\n> ";
		str += String(i + 1);
		str += ". "; 
		str += WiFi.SSID(i); 
		str += " : "; 
		str += WiFi.channel(i); 
		str += " (";  str += WiFi.RSSI(i); str += ")";
	} 
	str += "\n> ----------------------------------------------";
	return str;
}
}

/* "Login". Запись в память логина и пароля WiFi*/
else if (string_1.equalsIgnoreCase("Login"))
{
	if (!string_2.equalsIgnoreCase("") && !string_3.equalsIgnoreCase("")) {memory.putString("login", string_2); memory.putString("passw", string_3);}
	return "Login: " + memory.getString("login") + " | Pass: " + memory.getString("passw");
}

/* "Reconnect". Переподключение WiFi по логину и паролю хранящимися в памяти*/
else if (string_1.equalsIgnoreCase("Reconnect"))
{
	memory.getString("login").toCharArray(ssid, memory.getString("login").length() + 1);
	memory.getString("passw").toCharArray(pass, memory.getString("passw").length() + 1);
	xTimerStart(timerRecon, 10); return "Reconnect WiFi to " + memory.getString("login") + " & Disconnect";
}

/* "Reset". Перезагрузка модуля*/
else if (string_1.equalsIgnoreCase("Reset"))
{
	xTimerStart(timerReset, 10); return "Reset & Disconnect";
}

/* "WiFi". Вывод текущих параметров WiFi*/
else if (string_1.equalsIgnoreCase("WiFi")) 
{
	String str = String(WiFi.SSID()) + " : " + WiFi.channel() + " (" + String(WiFi.RSSI()) + ") " + WiFi.localIP().toString();
	return str;	
}

/* "Count". Вывод счетчиков перезагрузок и реконнектов*/
else if (string_1.equalsIgnoreCase("Count"))
{
	String str = "Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi"));
	return str;
}	

/* "CountRes". Сброс счетчиков перезагрузок и реконнектов*/
else if (string_1.equalsIgnoreCase("CountRes"))
{
	memory.putInt("countReset", 0); memory.putInt("countWifi", 0);
	String str = "Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi"));
	return str;
}

/* "Mem". Отправка IFTTT сообщения c объемами Heap*/
else if (string_1.equalsIgnoreCase("Mem"))
{
	IFTTTSend (String(ifttt_event), String(board_name), "All:" + String(ESP.getHeapSize()), "Free:" + String(ESP.getFreeHeap()));	
	return "Mem";	
}

/* "Help".*/
else if (string_1.equalsIgnoreCase("?") || string_1.equalsIgnoreCase("Help"))
{
	String str = 
	(String)
	"\n> | Scan | WiFi | Reconnect | Reset |" +
	"\n> | Login/<SSID>/<Password> |" +
	"\n> | Count | CountRes | Mem |" +
	"\n> | ? | Help |";
	return str;
}

/* Все остальное*/
else
{
	String str =
	"\n1>/" + string_1 +
	"\n2>/" + string_2 +
	"\n3>/" + string_3;
	return str;
}
}

/* Обработка пароля*/
if (!flag_Terminal_pass) {if (!string_1.compareTo("cbhLtpjr")) {flag_Terminal_pass = true; return "";} else return "Password";}
}
