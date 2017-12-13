#include <esp_log.h>
#include <string>

#include "BME280.h"
#include "GxEPD.h"
#include "sdkconfig.h"
#include "I2C.h"
#include "esp_heap_trace.h"

static char tag[]="cpp_helloworld";

BME280 bme280Sensor;
GxEPD display;

extern "C" {
	void app_main(void);
}

void app_main(void)
{
	bme280Sensor.init();
	display.init();

	while (1) {
		vTaskDelay(200/portTICK_RATE_MS);
	}
}
