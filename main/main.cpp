#include <esp_log.h>
#include <string>

#include "BME280.h"
#include "GxEPD.h"
#include "sdkconfig.h"
#include "I2C.h"

static char tag[]="cpp_helloworld";

extern "C" {
	void app_main(void);
}

void app_main(void)
{

	BME280 bme280Sensor;
	bme280Sensor.init();

	GxEPD display;
	display.init();

	while (1) {
		vTaskDelay(200/portTICK_RATE_MS);
	}
}
