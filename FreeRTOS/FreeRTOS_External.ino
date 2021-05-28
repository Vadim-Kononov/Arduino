/*Функция обработки WiFi событий*/
void WiFiEvent(WiFiEvent_t event)
{
  switch(event)
  {
    case SYSTEM_EVENT_STA_GOT_IP:
    flag_WiFi_connected = true;
	break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    flag_WiFi_connected = false;
    break;
  }
}



/*Функция подключения WiFi при потери связи*/
void WiFiConnect(void *parameter)
{
  for(;;)
  {
	if (!flag_WiFi_connected)
	{
		/*Получение хранящегося логина и пароля WiFi и перевод их в массивы char[]*/
		memory.getString("login").toCharArray(ssid, memory.getString("login").length() + 1);
		memory.getString("passw").toCharArray(pass, memory.getString("passw").length() + 1);
		if (flag_After_Boot) memory.putInt("countWifi", memory.getInt("countWifi") + 1); else flag_After_Boot = true;
		WiFi.begin(ssid, pass);
	}
    vTaskDelay(60000); 
  }
}



/*Отправка на сервер IFTTT методом POST*/
bool IFTTTSend (String event, String value1, String value2, String value3)
{
 if (!flag_WiFi_connected) return false;
 
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
    flag_IFTTT_time = false;
    xTimerChangePeriod(timerIFTTTTime,  pdMS_TO_TICKS(500), 0);
    while (iftttClient.available() == 0 && !flag_IFTTT_time) {};
    if (flag_IFTTT_time) {iftttClient.stop(); return false;}  
        
    iftttClient.parseFloat();
    String resp = String(iftttClient.parseInt());
    log_i("Response code: %s", resp);
	if (resp.equalsIgnoreCase("200")) return true; else return false;
 }


/*Срабатываниe таймера ожидания ответа сервера*/
void IFTTTTime ()
{
flag_IFTTT_time = true;  
}



#ifdef MAIN //----------------------------------------------------------------------------------------------------------------------------------
/*Функция подключения MQTT*/
void MQTTConnect(void *parameter)
{
  for(;;)
  {
    if (flag_WiFi_connected && !flag_MQTT_connected)
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
	for(;;)
   {
		mqttClient.publish("freertos", 0, false, String(TICKCOUNT).c_str());
		log_i("%010.3f", TICKCOUNT);
		vTaskDelay(10000); 
   }
}



/*Обработчик события подключения, здесь подиска на топики */
void mqtt_Connected_Complete(bool sessionPresent)
{
log_i();
mqttClient.subscribe("freertos", 0);
flag_MQTT_connected = true;
}

/*Обработчик события обрыва связи*/
void mqtt_Disconnect_Complete(AsyncMqttClientDisconnectReason reason)
{
log_i();
flag_MQTT_connected = false;
}

/*Обработчик события приема сообщения, здесь обработка принятых сообщений*/
void mqtt_Receiving_Complete(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
String Payload = String (payload).substring(0, len); 
if (String(topic).equalsIgnoreCase("freertos"))  if  (Payload.toInt() != 100) log_i("%s", Payload);
}
#endif //---------------------------------------------------------------------------------------------------------------------------------------