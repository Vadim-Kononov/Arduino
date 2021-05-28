//Подключение к WiFi
void WiFi_reconnect()
{
//Выход при отсутствии питания 220 В
if (power_flag) return;

//Получение хранящегося логина и пароля WiFi и перевод их в массивы char[]
memory.getString("login").toCharArray(ssid, memory.getString("login").length() + 1);
memory.getString("pass").toCharArray(password, memory.getString("pass").length() + 1);

#ifdef Serial_On
Serial.print("\nConnecting to " + String(ssid));
#endif

WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);
WiFi.setHostname(NAME);

//Инициализация счетчика попыток подключения
int count = 0;
while (WiFi.status() != WL_CONNECTED)
{
#ifdef Serial_On
Serial.print (".");
#endif
//Если попыток больше Connection_Count_WiFi, то прекратить
count++; if (count >= Connection_Count_WiFi) break;
delay(250);
}
//Увеличение счетчика реконнектов к WiFi на 1, если это не перезагрузка модуля
if (res_flag)
{
memory.putInt("countWifi", memory.getInt("countWifi") + 1);
//Отправка IFTTT сообщения о реконнекте
SendIFTTT(KEY, EVENT_NAME2, String(NAME) + "   ", String (memory.getInt("countReset")), String(memory.getInt("countWifi")));
}
#ifdef Serial_On
if (WiFi.status() != WL_CONNECTED) Serial.println ("\nНе удалось подключиться к WiFI");
else {Serial.print ("\nWiFi connected, IP address: "); Serial.println(WiFi.localIP());}
#endif
}



//Сканирование WiFi
void WiFiScan ()
{
int n = WiFi.scanNetworks();
String str = "";
if (n == 0) {str = "Сети не найдены"; Out(str, "Wifi"); Out(str, "BT");}
else {for (int i = 0; i < n; ++i) {str += String(i + 1); str += ". "; str += WiFi.SSID(i); str += " (";  str += WiFi.RSSI(i); str += ") \n";} Out(str, "Wifi"); Out(str, "BT");}
}



//Подключение к MQTT
void MQTT_reconnect()
{
while (!client.connected())
{
#ifdef Serial_On
Serial.println("MQTT connection to " + String(mqtt_server) + ", wait...");
#endif
mqtt_device += String(random(0xffff), HEX);
if (client.connect(mqtt_device.c_str(), mqtt_user, mqtt_pass))
{
#ifdef Serial_On
Serial.println("Connected user: " + String(mqtt_user));
#endif
//Подписка на топики
client.subscribe("signal/on");
client.subscribe("signal/off");
client.subscribe("signal/on_Light");
client.subscribe("signal/off_Light");
client.subscribe("signal/on_Alarm");
client.subscribe("signal/off_Alarm");
}
else
{
#ifdef Serial_On
Serial.print("Connected failed, rc= ");
Serial.println(client.state());
#endif

return;
}
}
}



//Чтение сообщений брокера
void callback(char* top, byte* pay, unsigned int length)
{
//Зажечь светодиод
digitalWrite(Onboard_Led, HIGH);
//Пробразование char* в String
String topic = top;

//Сообщение не длинее 50 символов, преобразование byte[] в char[], преобразование char[] в String
char string[50];
memcpy (string, pay, length);
string[length] = '\0';
String payload = string;

// Обработка принятых сообщений
if (topic == "signal/on"        && payload.toInt() == 1)  ON();
if (topic == "signal/off"       && payload.toInt() == 1)  OFF();
if (topic == "signal/on_Light"  && payload.toInt() == 1)  bitSet(situation, 2);
if (topic == "signal/off_Light" && payload.toInt() == 1)  bitClear(situation, 2);
if (topic == "signal/on_Alarm"  && payload.toInt() == 1)  AlarmON();
if (topic == "signal/off_Alarm" && payload.toInt() == 1)  AlarmOFF();

//Потушить светодиод
digitalWrite(Onboard_Led, LOW);
}


/*
//Отправка сообщений брокеру
void SendMqtt (String topic, String payload)
{
//Зажечь светодиод
digitalWrite(Onboard_Led, HIGH);
//преобразование String в char[]
char top [50]; char msg [50];
topic.toCharArray(top, topic.length() + 1);
payload.toCharArray(msg, payload.length() + 1);

client.publish(top, msg);
//Потушить светодиод
digitalWrite(Onboard_Led, LOW);
}
*/
