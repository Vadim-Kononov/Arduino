/*Функция обработки WiFi событий*/
void WiFiEvent(WiFiEvent_t event)
{
  switch(event)
  {
    case SYSTEM_EVENT_STA_GOT_IP:
    flag_WiFiConnected = true;
	break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    flag_WiFiConnected = false;
    break;
  }
}



/*Функция подключения WiFi при потери связи*/
void WiFiConnect(void *parameter)
{
  for(;;)
  {
	if (!flag_WiFiConnected)
	{
		/*Получение хранящегося логина и пароля WiFi и перевод их в массивы char[]*/
		memory.getString("login").toCharArray(ssid, memory.getString("login").length() + 1);
		memory.getString("passw").toCharArray(pass, memory.getString("passw").length() + 1);
		if (flag_AfterBoot) memory.putInt("countWifi", memory.getInt("countWifi") + 1); else flag_AfterBoot = true;
		WiFi.begin(ssid, pass);
		WiFi.setSleep(false);
	}
    vTaskDelay(60000); 
  }
}



/*Отправка на сервер IFTTT методом POST*/
bool IFTTTSend (String event, String value1, String value2, String value3)
{
 if (!flag_WiFiConnected) return false;
 
 log_i(); 
 String data_to_send = "";
 data_to_send += "{\"value1\":\"";
 data_to_send += value1; 
 data_to_send += "\",\"value2\":\""; 
 data_to_send += value2; 
 data_to_send += "\",\"value3\":\""; 
 data_to_send += value3; 
 data_to_send += "\"}";   
 
 char server[] = "maker.ifttt.com";
 
 WiFiClient iftttClient;
 iftttClient.stop();

  if (iftttClient.connect(server, 80))
    {
    log_i("%s", data_to_send);
	iftttClient.println
    (
     String("POST /trigger/") 
    + event
    + String("/with/key/")
    + ifttt_api_key 
    + String(" HTTP/1.1\n")
    + "Host: maker.ifttt.com\nConnection: close\nContent-Type: application/json\nContent-Length: "
    + String(data_to_send.length())
    + "\n\n" 
    + String(data_to_send)
    );
    }
    else
    {
    log_i("Failed to connect to IFTTT");
    }
    
    /*Ответ сервера*/
	uint32_t time_last = xTaskGetTickCount();	
	while (iftttClient.available() == 0 && xTaskGetTickCount() - time_last < 500) {};	
	if (iftttClient.available() == 0) {iftttClient.stop(); return false;} 
    String resp = String(iftttClient.parseInt());
    iftttClient.parseFloat();
    log_i("Response code: %s", resp);
	if (resp.equalsIgnoreCase("200")) return true; else return false;
 }



/*Функция подключения MQTT*/
void MQTTConnect(void *parameter)
{
  for(;;)
  {
    if (flag_WiFiConnected && !flag_MQTTConnected)
    {
		mqttClient.connect();
		log_i();
	}
	vTaskDelay(60000); 
  }
}



/*Функция отправки сообщений MQTT*/
void MQTTSend (void *parameter)
{
	static uint16_t cycle_counter;
	bool flag_SendAlarm = false;
	for(;;)
	{
		cycle_counter ++;																	//Подсчет циклов
		if (cycle_counter >= 180)															//Действия при наборе количества циклов
		{
			cycle_counter = 0;
			flag_SendAlarm = true;
		}
		uint32_t time = xTaskGetTickCount() / 1000, remains;
		uint8_t sec, min, hour, day;
		day 	= time/86400; 	remains = time%86400;
		hour 	= remains/3600; remains = remains%3600;
		min 	= remains/60; 	sec 	= remains%60;
		char str[35];
		sprintf (str, "%02d 🌙 %02d:%02d:%02d", day, hour, min, sec);
		mqttClient.publish("time", 0, false, String(str).c_str());
		mqttClient.publish("enable", 0, false, String(flag_AlarmEnable).c_str());
		mqttClient.publish("light", 0, false, String(now_get.light).c_str());
		mqttClient.publish("protect", 0, false, String(flag_ProtectOn).c_str());
		mqttClient.publish("exchange", 0, false, String(now_put.exchange).c_str());
	
		float busvoltage = ina219.getBusVoltage_V();
		float current_mA = ina219.getCurrent_mA();
		if (current_mA > 500.0 && flag_SendAlarm)
		{
			IFTTTSend (String(ifttt_event), String(board_name), "!!! Alarm is running !!!", "");
			sendSMS(my_number_2, "!!! Alarm is running !!!");
			flag_SendAlarm = false;
			cycle_counter = 0;
		}
		sprintf (str, "Voltage: %04.1fV    Current: %03.1fA", busvoltage, current_mA/1000.0);
		mqttClient.publish("ina", 0, false, String(str).c_str());
		
		//log_i("%010.3f", TICKCOUNT);
		vTaskDelay(1000); 
   }
}

/*Обработчик события подключения, здесь подиска на топики */
void mqtt_Connected_Complete(bool sessionPresent)
{
log_i();
mqttClient.subscribe("on", 0);
mqttClient.subscribe("off", 0);
mqttClient.subscribe("alarm", 0);
mqttClient.subscribe("enable", 0);
mqttClient.subscribe("lighton", 0);
mqttClient.subscribe("lightoff", 0);
flag_MQTTConnected = true;
}

/*Обработчик события обрыва связи*/
void mqtt_Disconnect_Complete(AsyncMqttClientDisconnectReason reason)
{
log_i();
flag_MQTTConnected = false;
}

/*Обработчик события приема сообщения, здесь обработка принятых сообщений*/
void mqtt_Receiving_Complete(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
String Payload = String (payload).substring(0, len); 
if (String(topic).equalsIgnoreCase("on"))  ProtectOn();
if (String(topic).equalsIgnoreCase("off"))  ProtectOff();
if (String(topic).equalsIgnoreCase("alarm"))  AlarmOn();
if (String(topic).equalsIgnoreCase("lighton"))  LightOn();
if (String(topic).equalsIgnoreCase("lightoff"))  LightOff();
}