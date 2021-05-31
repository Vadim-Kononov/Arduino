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
