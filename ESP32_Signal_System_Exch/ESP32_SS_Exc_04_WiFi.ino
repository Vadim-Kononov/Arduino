/*Строковые константы*/
const char * string_wifi [] PROGMEM = 
{
"Connecting to Wi-Fi...",     //0
"[WiFi-event] event: ",       //1
"WiFi connected :)",          //2
"WiFi connection failure :("  //3
};

/*Функция подключения к WiFi*/
void WiFiConnect()
{
/*Получение хранящегося логина и пароля WiFi и перевод их в массивы char[]*/
memory.getString("login").toCharArray(ssid, memory.getString("login").length() + 1);
memory.getString("pass").toCharArray(password, memory.getString("pass").length() + 1);

DEBUG_PRINTLN(string_wifi [0]);
WiFi.mode(WIFI_AP_STA);
WiFi.begin(ssid, password);
WiFi.setHostname(board_name);
WiFi.setAutoConnect(false);
WiFi.setAutoReconnect(false);
WiFi.softAP("Empty", "abkKbgnf", 1, 0);

memory.putInt("countWifi", memory.getInt("countWifi") + 1);
}
/**/

/*Функция определения WiFi событий*/
void WiFiEvent(WiFiEvent_t event)
{
DEBUG_PRINTLN(String(string_wifi [1]) + String(event));
switch(event)
 {
 case SYSTEM_EVENT_STA_GOT_IP:
    flag_WiFi_connected = true;
    DEBUG_PRINTLN(string_wifi [2]);
    DEBUG_PRINTLN(WiFi.localIP());
    break;
 case SYSTEM_EVENT_STA_DISCONNECTED:
    flag_WiFi_connected = false;
    DEBUG_PRINTLN(string_wifi [3]);
    break;
  }
}
/**/

/*Функция восстановления подключения к WiFi и MQTT*/
void WiFiReconnect()
{
if (!flag_WiFi_connected) {WiFiConnect();}
/*Main MQTT Connect*/
else if (flag_WiFi_connected && !flag_MQTT_connected) {mqtt_Connect();}
}
/**/
