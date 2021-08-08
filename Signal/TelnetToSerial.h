WiFiClient client = server.available();

if (client)
{
	Serial.println("New Client.");
    while (client.connected()) 
	{
			if (client.available())
			{
				c = client.read();
				Serial.write(c);
			}
			
			if (Serial.available())
			{
				c = Serial.read();
				client.write(c);
			}
    }
    client.stop();
    Serial.println("Client Disconnected.");
}