//IFTTT отправка данных без использования специальной библиотеки с помощью GET запроса
bool SendIFTTT (String key, String event_name, String value_1, String value_2, String value_3)
{
if (WiFi.status() != WL_CONNECTED) return false;
String url = "https://maker.ifttt.com/trigger/" + event_name + "/with/key/" + key + "?value1=" + value_1 + "&value2=" + value_2 + "&value3=" + value_3;
url.replace(" ", "%20");
if (GET ("maker.ifttt.com", url)) return true;
else return false;
}



//GET запрос
bool GET (String host_string, String url)
{
digitalWrite(Onboard_Led, HIGH);

char host [30];
host_string.toCharArray(host, host_string.length() + 1);
#ifdef Serial_On
Serial.print("\nConnecting to ");
Serial.println(host);
#endif
WiFiClient client;

if (!client.connect((char*)host, 80))
{
#ifdef Serial_On
Serial.println("Connection failed !");
#endif
return false;
}
#ifdef Serial_On
Serial.print("Requesting URL: ");
Serial.println(url);
#endif
//Отправка запроса на сервер
client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
unsigned long timeout = millis();
while (client.available() == 0)
{
if (millis() - timeout > 3000)
{
#ifdef Serial_On
Serial.println("Client Timeout!");
#endif
client.stop();
return false;
}
}
//Вывод в Serial ответа сервера
String line = "";
while (client.available()) {line += client.readStringUntil('\r');}

#ifdef Serial_On
Serial.print(line);
Serial.println("\n\nСlosing connection\n");
#endif

digitalWrite(Onboard_Led, LOW);

if (line.startsWith("HTTP/1.1 200 OK"))  return true;
else return false;
}
