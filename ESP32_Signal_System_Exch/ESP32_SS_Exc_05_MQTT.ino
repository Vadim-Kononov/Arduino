/*Функция подключения*/
void mqtt_Connect()
{
DEBUG_PRINTLN("Connecting to MQTT...");
mqttClient.connect();
}
/**/

/*Обработчик события подключения*/
/*Здесь подиска на топики*/
void mqtt_Connected_Complete(bool sessionPresent)
{
DEBUG_PRINTLN("Time:\t" + String(xTaskGetTickCount()/1000.0) + "\t" + "mqtt_Connected_Complete");

flag_MQTT_connected = true;
DEBUG_PRINTLN("Connected to MQTT :)");
mqttClient.subscribe("test1", 0);
}
/**/

/*Обработчик события обрыва связи*/
void mqtt_Disconnect_Complete(AsyncMqttClientDisconnectReason reason)
{
/*Флаг обрыва связи*/
flag_MQTT_connected = false;
DEBUG_PRINTLN("Disconnected from MQTT :(");
}
/**/

/*Обработчик события подписки*/
void mqtt_Subscribe_Complete(uint16_t packetId, uint8_t qos)
{
DEBUG_PRINTLN("Subscription confirmed, packetId: " + String(packetId) + "  qos: " + String(qos));
}
/**/

/*Обработчик события отписки*/
void mqtt_Unsubscribe_Complete(uint16_t packetId)
{
DEBUG_PRINTLN("Unsubscribe confirmed, packetId: " + String(packetId));
}
/**/

/*Обработчик события публикации*/
void mqtt_Publishe_Complete(uint16_t packetId)
{
DEBUG_PRINTLN("Publication confirmed, packetId: " + String(packetId));
}
/**/

/*Обработчик события приема сообщения*/
/*Здесь обработка принятых сообщений*/
void mqtt_Receiving_Complete(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
String Payload = String (payload).substring(0, len);
DEBUG_PRINTLN("Time:\t" + String(xTaskGetTickCount()/1000.0) + "\t" + "topic:\t" + String(topic) + "\t| payload:\t" + String(Payload));
}
/**/

/*Функция отправки сообщений MQTT*/
void mqtt_Send ()
{
//DEBUG_PRINTLN("Time:\t" + String(xTaskGetTickCount()/1000.0) + "\t" + "mqtt_Send");

if (flag_MQTT_connected)
{
mqttClient.publish("test1", 0, false, String(struct_Receiv.situation).c_str());
}
}
/**/
