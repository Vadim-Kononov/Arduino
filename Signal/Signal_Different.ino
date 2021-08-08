/*Включение сигнализации*/
void ProtectOn()
{
	flag_ProtectOn = true;
	now_put.protect = true;
	if(flag_AlarmEnable)
	{
		alarm_start.on 		= 75;
		alarm_start.off 	= 0;
		alarm_start.count 	= 1;		
		vTaskResume(handleAlarmProcessing);
		xQueueSend(queueAlarm, &alarm_start, 250);
	}
}



/*Включение сирены*/
void AlarmOn()
{
	now_put.alarm = true;
	if(flag_AlarmEnable)
	{
	alarm_start.on 		= 60000;
	alarm_start.off 	= 0;
	alarm_start.count 	= 1;		
	vTaskResume(handleAlarmProcessing);
	xQueueSend(queueAlarm, &alarm_start, 250);
	}
}



/*Отключение сигнализации*/
void ProtectOff()
{
	flag_ProtectOn = false;
	now_put.protect = false;
	now_put.alarm = false;
	
	vTaskSuspend(handleAlarmProcessing);
	if(flag_AlarmEnable)
	{
		digitalWrite(ALARM_RELAY_PIN_1, LOW);
		digitalWrite(ALARM_RELAY_PIN_2, LOW);
		vTaskDelay(50);
		digitalWrite(ALARM_RELAY_PIN_1, HIGH);
		digitalWrite(ALARM_RELAY_PIN_2, HIGH);
		vTaskDelay(50);
		digitalWrite(ALARM_RELAY_PIN_1, LOW);
		digitalWrite(ALARM_RELAY_PIN_2, LOW);
		vTaskDelay(50);
	}
	digitalWrite(ALARM_RELAY_PIN_1, HIGH);
	digitalWrite(ALARM_RELAY_PIN_2, HIGH);		
}



/*Включение cвета*/
void LightOn()
{
	now_put.light = true;
}



/*JОтключение света*/
void LightOff()
{
	now_put.light = false;
}



/*Сигнал от датчика*/
void SensorSignal(void *parameter)
{
	for(;;)
   {
		xSemaphoreTake(xBinSemaphore_SensorSignal, portMAX_DELAY);	//Получение семафора
		Serial.println("Семафор получен <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
		IFTTTSend (String(ifttt_event), String(board_name), "Sensor signal", title);
		
		if (!flag_RepeatSignal)
		{
			alarm_start.on 		= 500;
			alarm_start.off 	= 250;
			alarm_start.count 	= 1;		
		}
		else
		{
			alarm_start.on 		= 1000;
			alarm_start.off 	= 1000;
			alarm_start.count 	= 3;
		}
		
		if (flag_AlarmEnable)
		{
			vTaskResume(handleAlarmProcessing);
			xQueueSend(queueAlarm, &alarm_start, 1000);
		}
		
		flag_RepeatSignal	= true;
		xTimerStart(timerRepeatSignal, 10);
	}
}



/*Ожидание повторного сигнала*/
void RepeatSignal()
{
	flag_RepeatSignal	= false;
}



/*Управление сиреной*/
void AlarmProcessing (void *parameter)
{
	for(;;)
	{
		xQueueReceive(queueAlarm, &alarm_start, portMAX_DELAY);
		for (int i = alarm_start.count; i > 0; i--)
		{
			digitalWrite(ALARM_RELAY_PIN_1, LOW);
			digitalWrite(ALARM_RELAY_PIN_2, LOW);
			vTaskDelay(alarm_start.on);
			digitalWrite(ALARM_RELAY_PIN_1, HIGH);
			digitalWrite(ALARM_RELAY_PIN_2, HIGH);
			vTaskDelay(alarm_start.off);
		}
	}
}



/*Обработка события подключения клиента*/
void BT_Event(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if(event == ESP_SPP_SRV_OPEN_EVT)
  {
    String str = "> " + String(board_name) + "\n> IP: " + WiFi.localIP().toString() + "  |  MAC: " +  WiFi.macAddress() + "\n> ";
    SerialBT.print(str);
  }
}



/*Функция Bluetooth терминала*/
void Bluetooth(void *parameter)
{
  for(;;)
  {
    if (SerialBT.available())
    {
    String str = SerialBT.readStringUntil('\n');
	SerialBT.println (Terminal(str));
	SerialBT.print ("> ");
	log_i();
    }
  }
}



/*Функция Telnet терминала*/
void Telnet(void *parameter)
{
	for(;;)
	{
		TelnetClient = TelnetServer.available();
		if (TelnetClient) 
		{
			Serial.println("New Client.");
			String str = "> " + String(board_name) + "\n> IP: " + WiFi.localIP().toString() + "  |  MAC: " +  WiFi.macAddress() + "\n> ";
			TelnetClient.print(str);
			while (TelnetClient.connected())
			{
				if (TelnetClient.available())
				{
					String str = TelnetClient.readStringUntil('\n');
					if (str.indexOf('\3') < 0)
					{
						TelnetClient.println (Terminal(str));
						TelnetClient.print ("> ");	
					}
					log_i("TelnetClient Available");
				}
			}
			
		}
		TelnetClient.stop();
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



//Вывод в строку записанных в память датчиков
String PrintDetector()
{
	memory.getBytes("mem", (detector*)detector_ram, sizeof(detector_ram));
	String str;
	for (int i=0; i<35; i++) str += "\n" + String(i) + "  " + String(detector_ram[i].type) + "  " + String(detector_ram[i].code, HEX) + "  " +  String(detector_ram[i].title);
	return str;
}



/*Обработка строки принятой через Telnet или Bluetooth*/
String Terminal(String &string)
{
String string_1 = "", string_2 = "", string_3 = "", string_4 = "";

/*Парсинг входой строки на 3 элемента по "/"*/
if (string.indexOf("/") >=0) {string_1 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_1 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_2 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_2 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_3 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_3 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_4 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_4 = string; string.remove(0, string.length());}

if (flag_TerminalPass)
{
/* "Password". Отключение пароля, если он включен. Режим триггера*/
if (string_1.equalsIgnoreCase("cbhLtpjr")) {flag_TerminalPass = false; return "Password";}

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
	String str = "All:" + String(ESP.getHeapSize()) + " Free:" + String(ESP.getFreeHeap());	
	return str;	
}

/*"Write". Запись в память данных датчика*/
else if (string_1.equalsIgnoreCase("Write"))
{
	number = string_2.toInt(); type = string_3.toInt(); title = string_4;
	if ((number>=0 && number<=255) && (type>=0 && type<=255) && (title.length() >=1 && title.length() <=48)) {flag_SavingCode = true; xTimerStart(timerSavingCode, 10); return "Start of waiting";}
	else return "Incorrect string";
}

/*"Read". Чтение записанных датчиков*/
else if (string_1.equalsIgnoreCase("Read")) return(PrintDetector());

/*"Clear". Стирание записанного датчика*/
else if (string_1.equalsIgnoreCase("Clear"))
{
	number = string_2.toInt();
	if (number>=0 && number<=255)
	{
		detector_ram[number].type = 0;
		detector_ram[number].code = 0;
		title = "0";
		title.toCharArray(detector_ram[number].title, 50);
		memory.putBytes("mem", (detector*)detector_ram, sizeof(detector_ram));
		return(PrintDetector());
	}
}

/* "On". Включение охраны*/
else if (string_1.equalsIgnoreCase("On"))
{
	ProtectOn(); return "On";	
}

/* "AlOn". Включение сирены*/
else if (string_1.equalsIgnoreCase("AlOn"))
{
	AlarmOn(); return "Alarml On";	
}

/* "Off". Отключение охраны*/
else if (string_1.equalsIgnoreCase("Off"))
{
	ProtectOff(); return "Off";	
}

/* "LOn". Включение света*/
else if (string_1.equalsIgnoreCase("LOn"))
{
	LightOn(); return "Light On";	
}

/* "LOff". Отключение света*/
else if (string_1.equalsIgnoreCase("LOff"))
{
	LightOff(); return "Light Off";	
}

/* "Enable". Разрешение-запрет работы сирены*/
else if (string_1.equalsIgnoreCase("Enable"))
{
	flag_AlarmEnable = !flag_AlarmEnable; return "Alarm Enable = " + String(flag_AlarmEnable);	
}

/* "Help".*/
else if (string_1.equalsIgnoreCase("?") || string_1.equalsIgnoreCase("Help"))
{
	String str = 
	(String)
	"\n> | Scan | WiFi | Reconnect | Reset |" +
	"\n> | Write/<Номер>/<Тип>/<Описание> |" +
	"\n> | Login/<SSID>/<Password> |" +
	"\n> | Count | CountRes | Mem |" +
	"\n> | Read | Clear/<Номер> |" +
	"\n> | AlOn | Enable |" +
	"\n> | LOn | LOff |" +
	"\n> | On | Off |" +
	"\n> | ? | Help |";
	return str;
}

/* Все остальное*/
else
{
	String str =
	"\n1>/" + string_1 +
	"\n2>/" + string_2 +
	"\n3>/" + string_3 +
	"\n4>/" + string_4;
	return str;
}
}

/* Обработка пароля*/
if (!flag_TerminalPass) {if (!string_1.compareTo("cbhLtpjr")) {flag_TerminalPass = true; return "";} else return "Password";}
}