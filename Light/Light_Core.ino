/*Задача основная*/
void Main_Function(void *parameter)
{
portTickType time_last = xTaskGetTickCount();										//Сохранение времени вызова
static uint16_t cycle_counter;														//Статический счетчик циклов Main_Function
   for(;;)
   {
    // log_i("--- %03d --- | %010.3f", cycle_counter, TICKCOUNT);						//Лог задачи
	
    xSemaphoreTake(xBinSemaphore_Get_End, portMAX_DELAY);							//Разблокировка при получении данных ESP-NOW
    now_put.exchange  = now_get.exchange;													//Обработка полученных данных
    now_put.hash  = now_get.hash;
    
    xSemaphoreGive(xBinSemaphore_Put_Start);										//Выдача семафора для отправки данных
  }
} 



/*Задача вспомогательная*/
void Slave_Function(void *parameter)
{
   for(;;)
	{
		// log_i("%010.3f", TICKCOUNT);												//Лог задачи
		Waiting(10000);																//Задержка
	}
 } 
