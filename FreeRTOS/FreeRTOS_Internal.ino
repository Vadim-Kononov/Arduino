/*Обработчик события приема данных ESP-NOW*/
void OnReceiving(const uint8_t *mac, const uint8_t *data, int len)
{
xQueueSend(queueReceiving, data, portMAX_DELAY);									//Запись в очередь ссылки на принятые данные
}



/*Функция обработки события приема данных ESP-NOW*/
void Receiving(void *parameter)
{
   for(;;)
   {
    xQueueReceive(queueReceiving, &now_get, portMAX_DELAY);
    Serial.print("Receiving <<< ");
    Serial.println (TICKCOUNT);
    Serial.println(String(now_get.board_name));
    Serial.println(now_get.flag);
    Serial.println(now_get.hash);
    Serial.println();
  
    xSemaphoreGive(xBinSemaphore_Get_End);											//Выдача семафора окончания обработки данных
  }
}



/*Функция отправки данных ESP-NOW*/
void Sending(void *parameter)
{
   for(;;)
   {
    xSemaphoreTake(xBinSemaphore_Put_Start, portMAX_DELAY);							//Получение семафора отправки данных
    
    /*Для главной платы*/
    #ifdef MAIN //--------------------------------------------------------------------------------------------------------------------------
    if (now_get.hash == now_put.hash)   now_put.flag = true; 						//Проверка принятого хэша
    else                				now_put.flag = false;
    now_put.hash = random(1000);													//Запись случайного хэша
    #endif //-------------------------------------------------------------------------------------------------------------------------------
    
    esp_err_t result = esp_now_send(mac_Peer_One, (uint8_t *) &now_put, sizeof(now_put));
    if (result == ESP_OK)
    {
      Serial.print("Sending >>> ");
      Serial.println (TICKCOUNT);
      Serial.println(String(now_put.board_name));
      Serial.println(now_put.flag);
      Serial.println(now_put.hash);
      Serial.println();
    }
    else log_i("! Failed Sending the data !");
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