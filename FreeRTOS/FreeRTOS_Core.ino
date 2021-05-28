/*Задача основная*/
void Main_Function(void *parameter)
{
portTickType time_last = xTaskGetTickCount();										//Сохранение времени вызова
static uint16_t cycle_counter;														//Статический счетчик циклов Main_Function
   for(;;)
   {
    log_i("--- %03d --- | %010.3f", cycle_counter, TICKCOUNT);						//Лог задачи
	
    /*Для главной платы*/
    #ifdef MAIN //--------------------------------------------------------------------------------------------------------------------------
    cycle_counter += 1;																//Подсчет циклов Main_Function
    if (cycle_counter >= 1)															//Действия при наборе количества циклов
    {
      cycle_counter = 0;
      xSemaphoreGive(xBinSemaphore_Put_Start);										//Выдача семафора отправки ESP-NOW
    }
    vTaskDelayUntil(&time_last, (10000));											//Таймаут между циклами Main_Function
    
    
    /*Для починенной платы*/
    #else //--------------------------------------------------------------------------------------------------------------------------------                          
    xSemaphoreTake(xBinSemaphore_Get_End, portMAX_DELAY);							//Разблокировка при получении данных ESP-NOW
    now_put.flag  = now_get.flag;													//Обработка полученных данных
    now_put.hash  = now_get.hash;
    
    xSemaphoreGive(xBinSemaphore_Put_Start);										//Выдача семафора для отправки данных
    #endif //-------------------------------------------------------------------------------------------------------------------------------
  }
} 



/*Задача вспомогательная*/
void Slave_Function(void *parameter)
{
   for(;;)
	{
		log_i("%010.3f", TICKCOUNT);												//Лог задачи
		Waiting(10000);																//Задержка
	}
 } 