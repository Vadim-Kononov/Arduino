/*Оправка команды SIM800, второй bool параметр ожидание и получение ответа*/
String sendATCommand(String cmd, bool waiting)
{
String answer = "";
Serial2.println(cmd);
if (waiting) answer = waitRespond();
return answer;
}



//Функция ожидания ответа и возврата полученного результата, а также вывода полученной строки на WiFiSerial и BTSerial
String waitRespond()
{
String answer = "", str = "";
long timeout = millis() + 5000;
//Окончание таймаута или получение ответа, что произошло раньше
while (!Serial2.available() && millis() < timeout)  {};
if (Serial2.available()) {answer = Serial2.readString();}
//Если за время таймаута ответ не пришел функция возвращает "Timeout"
else answer = "Timeout";
//Замена перевола строки на пробелы и вывод на WiFiSerial и BTSerial терминалы
str = answer;
str.replace('\n', ' ');
str.trim();
#if   (DEBUG == 1)
Writeln ("$ " + str, B111);
#else
Writeln ("$ " + str, B110);
#endif
return answer;
}



//Функция разбора SMS на текст и номер отправителя
void analysisSMS(String msg)
{
String msgheader = "";
String msgbody = "";
String msgphone = "";

msg = msg.substring(msg.indexOf("+CMGR: "));
msgheader = msg.substring(0, msg.indexOf("\r"));

msgbody = msg.substring(msgheader.length() + 2);
msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));
msgbody.trim();

int firstIndex = msgheader.indexOf("\",\"") + 3;
int secondIndex = msgheader.indexOf("\",\"", firstIndex);
msgphone = msgheader.substring(firstIndex, secondIndex);

//Здесь происходит обработка комад поступивших в SMS
sendATCommand("AT+CMGDA=\"DEL ALL\"", true);

//------------------------------------------Signal System------------------------------------------//
if (msgphone.equalsIgnoreCase(my_number) || msgphone.equalsIgnoreCase(boris_number) || msgphone.equalsIgnoreCase(gleb_number))
{
    String str = String(random(10, 100));
    
    if        (msgbody.equalsIgnoreCase("1"))           ON();
    else if   (msgbody.equalsIgnoreCase("2"))           OFF();
    else if   (msgbody.equalsIgnoreCase("3"))           struct_Send.situation = 20;
    else if   (msgbody.equalsIgnoreCase("4"))           struct_Send.situation = 20;
    else if   (msgbody.equalsIgnoreCase("5"))           {string_check_random = String(random(10, 100)); sendSMS(msgphone, "Confirmation, send - " + string_check_random);}
    else if   (msgbody.equalsIgnoreCase("6"))           {AlarmOFF(); sendSMS(msgphone, "Yes, the Siren is OFF");}
    else if   (msgbody.equalsIgnoreCase(string_check_random))  {AlarmON();  sendSMS(msgphone, "Yes, the Siren is ON");}
    
    else      sendSMS(msgphone, "1 -> On\n2 -> Off\n\n3 -> Light On\n4 -> Light Off\n\n5 -> Siren On, get code...\n6 -> Siren Off");
}
//------------------------------------------Signal System------------------------------------------//
}



//Функция отправки SMS
void sendSMS(String phone, String message)
{
sendATCommand("AT+CMGS=\"" + phone + "\"", true);
sendATCommand(message + "\r\n" + (String)((char)26), true);
}
