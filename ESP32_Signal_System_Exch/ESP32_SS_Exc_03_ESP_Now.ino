/*Функция малого цикла*/
void SmallCycleFunction ()
{

/*Отправка структуры*/
esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &struct_Send, sizeof(struct_Send));

/*Сброс сторожевого таймера*/
timerWrite(timerWDT, 0);
}
/**/



/*Функция большого цикла*/
void BigCycleFunction ()
{
GetTime();

flag_BigCycleFunction = true;
}
/**/



/*Функция обратного вызова при приеме данных ESPNOW*/
void Now_Receiving_Complete(const uint8_t * mac, const uint8_t *incomingData, int len)
{

/*Заполнение структуры принятыми данными*/
memcpy(&struct_Receiv, incomingData, sizeof(struct_Receiv));

if (struct_Receiv.situation != receiv_Saved)
{
  receiv_Saved = struct_Receiv.situation;
  switch (receiv_Saved) 
  {
  case 0: 
  DEBUG_PRINTLN("Instruction:\tAlarm OFF");
  break;
  
  case 1:
  DEBUG_PRINTLN("Instruction:\tAlarm ON");
  break;

  case 2:
  DEBUG_PRINTLN("Instruction:\tAlarm ON Perimeter");
  break;

  case 10:
  DEBUG_PRINTLN(String("Instruction:\tWarning signal\t") + String(detector_title));
  break;

  case 20:
  DEBUG_PRINTLN("Instruction:\tTurn on the light");
  }
}
}
/**/



/*Функция обратного вызова при отправке данных ESPNOW*/
/*Пустая*/
void Now_Sending_Complete(const uint8_t *mac_addr, esp_now_send_status_t status) {}
/**/

