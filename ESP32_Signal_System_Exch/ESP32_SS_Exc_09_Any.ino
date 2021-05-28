/*Получение времени по NTP*/
void GetTime()
{
if (flag_WiFi_connected && Ping.ping("8.8.8.8", 1)) {configTime(10800, 0, "pool.ntp.org"); getLocalTime(&timeinfo);}
}
/**/


/*Отключение таймеров перед заагрузкой OTA*/
void TimerStop()
{
xTimerStop(timerBigCycleFunction, 0);
xTimerStop(timerESPNowSend, 0);
xTimerStop(timerMqttSend, 0);

DEBUG_PRINTLN(F("------------- TimerStop -------------"));
}
/**/



/*Процедура включения сигнализации*/
void ON()
{
struct_Send.situation = 1;
flag_activation_alarm = true; flag_perimeter_alarm = false; flag_alarm_triggered = false;
AlarmRelayOn();
xTimerChangePeriod(timerRelayOff_1, pdMS_TO_TICKS(75), 0);
if (!IFTTTSend (String(ifttt_event), "Signal System ON", "", "")) {sendSMS(my_number, "Signal System ON"); sendSMS(boris_number, "Signal System ON"); sendSMS(gleb_number, "Signal System ON");}
}
/**/


/*Процедура включения сигнализации по периметру*/
void ON_Perimeter()
{
struct_Send.situation = 2;
flag_activation_alarm = false; flag_perimeter_alarm = true; flag_alarm_triggered = false;
AlarmRelayOn();
xTimerChangePeriod(timerRelayOff_1, pdMS_TO_TICKS(35), 0);
if (!IFTTTSend (String(ifttt_event), "Signal System Perimeter ON", "", "")) {sendSMS(my_number, "Signal System Perimeter ON"); sendSMS(boris_number, "Signal System Perimeter ON"); sendSMS(gleb_number, "Signal System Perimeter ON");}
}
/**/



/*Процедура отключения сигнализации*/
void OFF()
{
struct_Send.situation = 0;
flag_activation_alarm = false; flag_perimeter_alarm = false;
AlarmRelayOn();
xTimerChangePeriod(timerRelayOff_1, pdMS_TO_TICKS(75), 0);
xTimerChangePeriod(timerRelayOn_1,  pdMS_TO_TICKS(150), 0);
xTimerChangePeriod(timerRelayOff_2, pdMS_TO_TICKS(225), 0);
if (flag_alarm_triggered)
{
xTimerChangePeriod(timerRelayOn_2,  pdMS_TO_TICKS(300), 0);
xTimerChangePeriod(timerRelayOff_3, pdMS_TO_TICKS(375), 0);
}
if (!IFTTTSend (String(ifttt_event), "Signal System OFF", "", "")) {sendSMS(my_number, "Signal System OFF"); sendSMS(boris_number, "Signal System OFF"); sendSMS(gleb_number, "Signal System OFF");}
}
/**/



/*Процедура включения сирены*/
void AlarmON()
{
struct_Send.situation = 1;
AlarmRelayOn();
xTimerChangePeriod(timerRelayOff_4, pdMS_TO_TICKS(1800000), 0);
}
/**/



/*Процедура отключения сирены*/
void AlarmOFF()
{
struct_Send.situation = 0;
AlarmRelayOff();
}
/**/




/*Процедура включения двух реле серены*/
void AlarmRelayOn()
{
digitalWrite(ALARM_RELAY_PIN_1, LOW);
digitalWrite(ALARM_RELAY_PIN_2, LOW);
xTimerStart(timerAlarmLongTime, 0);
}
/**/




/*Процедура включения реле первой серены*/
void AlarmRelayOn_1()
{
digitalWrite(ALARM_RELAY_PIN_1, LOW);
xTimerStart(timerAlarmLongTime, 0);
}
/**/




/*Процедура включения реле второй серены*/
void AlarmRelayOn_2()
{
digitalWrite(ALARM_RELAY_PIN_2, LOW);
xTimerStart(timerAlarmLongTime, 0);
}
/**/




/*Процедура отключения двух реле серены*/
void AlarmRelayOff()
{
digitalWrite(ALARM_RELAY_PIN_1, HIGH);
digitalWrite(ALARM_RELAY_PIN_2, HIGH);
xTimerStop(timerAlarmLongTime, 0);
}
/**/


