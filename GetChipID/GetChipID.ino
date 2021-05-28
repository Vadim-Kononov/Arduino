#define CONFIG_ARDUHAL_LOG_COLORS 1
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

#include "ArduinoTrace.h"

int testVar = 10;

void setup()
{
Serial.begin(115200);
DUMP(testVar);
TRACE();
}

void loop() 
{
	DUMP(testVar);
	log_e("ESP.getEfuseMac:\t\t%llX", ESP.getEfuseMac());
	log_e("ESP.getChipRevision:\t%u", ESP.getChipRevision());
	log_e("ESP.getFlashChipSize:\t%u", ESP.getFlashChipSize());
	log_i("ESP.getHeapSize:\t\t%u", ESP.getHeapSize());
	log_i("ESP.getPsramSize:\t\t%u", ESP.getPsramSize());
	log_i("ESP.getFreePsram:\t\t%u", ESP.getFreePsram());
	log_i("ESP.getCpuFreqMHz:\t%u", ESP.getCpuFreqMHz());
	log_i("esp_timer_get_time:\t%5.3f", esp_timer_get_time() / 1000000.0);
	log_i("ESP.getCycleCount:\t%5.3f", ESP.getCycleCount() / 1000000.0);

	testVar++;
	
	TRACE();
	delay(5000);

}
