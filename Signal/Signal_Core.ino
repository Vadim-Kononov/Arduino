/*Получение кода*/
void CodeProcessing (void *parameter)
{
	static uint32_t code, code_last;													//Код датчика   
	static uint16_t cycle_counter;														//Счетчик циклов
	portTickType time_last = xTaskGetTickCount();										//Сохранение времени вызова	
	for(;;)
	{
		cycle_counter++;																//Обнуление code после 10 циклов
		if (cycle_counter >= 10)														
		{
			cycle_counter 	= 0;
			code_last 		= 0;
		}
		
		if (receiver.available())
		{
			code = receiver.getReceivedValue();
			receiver.resetAvailable();
			if (code != code_last)
			{
				code_last 		= code;
				cycle_counter 	= 0;
				if (flag_SavingCode)
				{
					flag_SavingCode = false;
					detector_ram[number].code 	= code;										//Запись элементов структуры
					detector_ram[number].type 	= type;
					title.toCharArray(detector_ram[number].title, 50);
					memory.putBytes("mem", (detector*)detector_ram, sizeof(detector_ram));	//Запись в NVS
					TelnetClient.println (PrintDetector());									//Вывод в терминалы таблицы кодов
					SerialBT.println (PrintDetector());		
					IFTTTSend (String(ifttt_event), "There was a sensor code entry", "", "");	
				}
				else
				{
					log_i("-------------------------------------------> %0X", code); 
					
					int k=-1;
					//Определение наименования сработавшего датчика
					for (int i=0; i<35; i++)
					{
						if (detector_ram[i].code == code)
						{
							k = detector_ram[i].type;
							title = detector_ram[i].title;
							break;
						}
					}
					switch (k) 
					{
						case 1: 
							if (flag_ProtectOn) xSemaphoreGive(xBinSemaphore_SensorSignal);
							break;
						case 2:
							ProtectOn();
							break;
						case 3: 
							AlarmOn();
							break;
						case 4:
							ProtectOff();
							break;
						case 5:
							LightOn();
							break;
						case 6:
							LightOff();
					}
				}		
			}
		}
	vTaskDelayUntil(&time_last, (500));														//Таймаут между циклами	
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
	Serial.println(answer);
	return answer;
}



/*ESP-NOW обмен*/
void Now_Exchange(void *parameter)
{
	portTickType time_last = xTaskGetTickCount();											//Сохранение времени вызова
	static uint16_t cycle_counter;															//Статический счетчик циклов Now_Exchange
	for(;;)
	{
		cycle_counter += 1;																	//Подсчет циклов Now_Exchange
		if (cycle_counter >= 1)																//Действия при наборе количества циклов
		{
			cycle_counter = 0;
			xSemaphoreGive(xBinSemaphore_PutStart);										//Выдача семафора отправки ESP-NOW
		}
		vTaskDelayUntil(&time_last, (1000));												//Таймаут между циклами
	} 
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