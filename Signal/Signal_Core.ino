/*Получение кода*/
void CodeProcessing (void *parameter)
{
	static uint32_t code, code_last;												//Полученный код, предшествующий код
	static uint16_t cycle_counter;													//Счетчик циклов
	portTickType time_last = xTaskGetTickCount();									//Сохранение времени вызова	
	for(;;)
	{
		cycle_counter++;															//Обнуление кода после 10 циклов
		if (cycle_counter >= 10)														
		{
			cycle_counter 	= 0;
			code_last 		= 0;
		}
		
		if (receiver.available())													//Если поступили данные от приемника
		{
			code = receiver.getReceivedValue();										//Получение данных
			receiver.resetAvailable();												//Сброс буфера
			if (code != code_last)													//Если кол не првторился
			{
				code_last 		= code;												//Сохранение кода
				cycle_counter 	= 0;												//Обновление счетчика циклов до обнуления кода
				if (flag_SavingCode)												//Если режим записи кода в NVS
				{
					flag_SavingCode = false;										//Отключение этого режима для последующего приема
					detector_ram[number].code 	= code;								//Запись элементов структуры
					detector_ram[number].type 	= type;								//Запись элементов структуры
					title.toCharArray(detector_ram[number].title, 50);				//Запись элементов структуры
					memory.putBytes("mem", (detector*)detector_ram, sizeof(detector_ram));	//Запись структуры в NVS
					TelnetClient.println (PrintDetector());									//Вывод в терминал таблицы кодов
					SerialBT.println (PrintDetector());										//Вывод в терминал таблицы кодов
					IFTTTSend (String(ifttt_event), "There was a sensor code entry", "", "");	//Отправка уведомления
				}
				else																//Если НЕ режим записи кода в NVS
				{
					log_i("-------------------------------------------> %0X", code); 
					
					int k=-1;														//Поиск в таблице сработавшего датчика
					for (int i=0; i<35; i++)
					{
						if (detector_ram[i].code == code)
						{
							k = detector_ram[i].type;								//Получение типа датчика
							title = detector_ram[i].title;							//Получение названия датчика
							break;
						}
					}
					switch (k) 														//Определение последующих действий по типу датчика
					{
						case 1: 
							if (flag_ProtectOn) xSemaphoreGive(xBinSemaphore_SensorSignal); //Запуск с помощью семафора задачи обработки кода при включенной сигнализации
							break;
						case 2:
							ProtectOn();											//Включение сигнализации
							break;
						case 3: 
							AlarmOn();												//Включение сирены
							break;
						case 4:
							ProtectOff();											//Отключение сигнализации и сирены
							break;
						case 5:
							LightOn();												//Включение света
							break;
						case 6:
							LightOff();												//Отключение света
					}
				}		
			}
		}
	vTaskDelayUntil(&time_last, (500));												//Таймаут между циклами	
	}
}



/*Ожидание ввода кода*/
void SavingCode()
{
	if (flag_SavingCode)
	{
		TelnetClient.println ("End of waiting");
		TelnetClient.print ("> ");
		SerialBT.println ("End of waiting");
		SerialBT.print ("> ");
	}
	flag_SavingCode = false;
}


/*Получение SMS*/
void SIM800Processing (void *parameter)
{
		static uint16_t cycle_counter;														//Счетчик циклов
		static String str_check;
		portTickType time_last = xTaskGetTickCount();										//Сохранение времени вызова	
		for(;;)
		{
			cycle_counter++;																//Обнуление code после 60 циклов
			if (cycle_counter >= 60)														
			{
				cycle_counter 	= 0;
				if (sendATCommand("AT+CMGDA=\"DEL ALL\"", 5000).equalsIgnoreCase("Timeout"))//Удаление сообщений и сброс при отсутствии ответа
				{
					digitalWrite(SIM800_PWR, LOW);		
					vTaskDelay(1000);
					digitalWrite(SIM800_PWR, HIGH);
				}
			}
			//Чтение с SIM800
			if (Serial2.available())
			{
				String respond = Serial2.readString(); Serial.println("Available: " + respond);
				respond.trim();
				if (respond.indexOf("+CMTI:") >=0 )  										//Если пришло SMS
				{
					int index = respond.lastIndexOf(",");                           		//Находим последнюю запятую, перед индексом
					String result = respond.substring(index + 1, respond.length()); 		//Получаем индекс
					result.trim();                                                  		//Убираем пробелы в начале/конце
					respond=sendATCommand("AT+CMGR="+result, 5000);                 		//Содержимое SMS
					respond = respond.substring(respond.indexOf("+CMGR: "));
					String msgheader = respond.substring(0, respond.indexOf("\r"));
					String msgbody = respond.substring(msgheader.length() + 2);				//Содержание
					msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));
					msgbody.trim();
					int firstIndex = msgheader.indexOf("\",\"") + 3;
					int secondIndex = msgheader.indexOf("\",\"", firstIndex);
					String msgphone = msgheader.substring(firstIndex, secondIndex);			//Номер телефона
					
					/*Обработка поступивших комад */
					if (msgphone.equalsIgnoreCase(my_number_1) || msgphone.equalsIgnoreCase(my_number_2) || msgphone.equalsIgnoreCase(boris_number) || msgphone.equalsIgnoreCase(gleb_number))
					{
						if        (msgbody.equalsIgnoreCase("1"))           ProtectOn();
						else if   (msgbody.equalsIgnoreCase("2"))           {ProtectOff(); sendSMS(msgphone, "Yes, is OFF");}
						else if   (msgbody.equalsIgnoreCase("3"))           LightOn();
						else if   (msgbody.equalsIgnoreCase("4"))           LightOff();
						else if   (msgbody.equalsIgnoreCase("5"))           {str_check = String(random(10, 100)); sendSMS(msgphone, "Confirmation, send - " + str_check);}
						else if   (msgbody.equalsIgnoreCase(str_check))		{AlarmOn();  sendSMS(msgphone, "Yes, the Siren is ON"); str_check = "999";}
						else      sendSMS(msgphone, "1 -> On\n2 -> Off\n\n3 -> Light On\n4 -> Light Off\n\n5 -> Siren On, get code...");
					}
				}
			}
		vTaskDelayUntil(&time_last, (1000));												//Таймаут между циклами		
		}
}



/*Отправка SMS*/
void sendSMS(String phone, String message)
{
sendATCommand("AT+CMGS=\"" + phone + "\"", 5000);
sendATCommand(message + "\r\n" + (String)((char)26), 5000);
}



/*Оправка команды SIM800*/
String sendATCommand(String str, uint32_t waiting)
{
	Serial2.println(str);
	String answer;
	uint32_t time_last = xTaskGetTickCount();	
	while (Serial2.available() == 0 && xTaskGetTickCount() - time_last < waiting) {};	
	if (Serial2.available()) answer = Serial2.readString(); else answer = "Timeout";
	//Serial.println(answer);
	return answer;
}



/*Контроль 220В*/
void PowerControl(void *parameter)
{
	static bool flag_Power;
	for(;;)
	{
		//log_i("%010.3f", TICKCOUNT);														//Лог задачи
		if (flag_Power != digitalRead(POWER_PIN))
		{
			flag_Power = digitalRead(POWER_PIN);
			if (flag_Power) {IFTTTSend (String(ifttt_event), String(board_name), "Voltage 220V OFF", "");}
			else 			{IFTTTSend (String(ifttt_event), String(board_name), "Voltage 220V ON", "");}
		}
		vTaskDelay(1000);																	//Таймаут между циклами
	} 
}



/*ESP-NOW обмен*/
void Now_Exchange(void *parameter)
{
	portTickType time_last = xTaskGetTickCount();											//Сохранение времени вызова
	for(;;)
	{
		if (now_get.hash == now_put.hash)   now_put.exchange = true; 						//Проверка принятого хэша
		else                				now_put.exchange = false;
		now_put.hash = random(1000);														//Запись случайного хэша
    
		esp_err_t result = esp_now_send(mac_Peer_One, (uint8_t *) &now_put, sizeof(now_put));
		if (result != ESP_OK) log_i("! Failed Sending the data !");
		
		vTaskDelayUntil(&time_last, (1000));												//Таймаут между циклами
	} 
}



/*Обработчик события приема данных ESP-NOW*/
void OnReceiving(const uint8_t *mac, const uint8_t *data, int len)
{							//Запись в очередь ссылки на принятые данные
memcpy(&now_get, data, sizeof(now_get));
}
