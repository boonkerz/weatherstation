#include <esp_log.h>
#include <string>

#include "Weather.h"
#include "sdkconfig.h"
#include "I2C.h"
#include "BootWiFi.h"

static char tag[]="WeatherStation";

Weather weatherObj;


extern "C" {
	void app_main(void);
}

void app_main(void)
{
	BootWiFi bootWiFi;
	bootWiFi.boot();
	weatherObj.init();

	while (1) {
		vTaskDelay(200/portTICK_RATE_MS);
	}
}
