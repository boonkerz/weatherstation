/*
 * weather.cpp
 *
 *  Created on: 16.12.2017
 *      Author: thomas
 */

#include "include/Weather.h"
#include "spiffs_vfs.h"

static char tag[] = "WatherThread";


void Weather::init() {

	vfs_spiffs_register();

	// the partition was mounted?
	if(spiffs_is_mounted) {
		printf("Partition correctly mounted!\r\n");
		vfs.init();
	}

	else {
		printf("Error while mounting the SPIFFS partition");
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}

	bme280Sensor.init();
	epd.init();
	api.init();
	start();
}

void Weather::run(void *data) {

	while(true) {
		ESP_LOGD(tag, ">> refresh");

		bme280Sensor.streamSensorDataNormalMode();
		api.refresh();

		display();

		vTaskDelay(20000/portTICK_RATE_MS);
	}
}



void Weather::display() {
	displayBase();
	displayLocal();
	displayApi();
	epd.update();
}


void Weather::displayBase() {
	epd.clean();

	epd.setFont(6, NULL);
	epd.drawText((char *)"Wetter Station", 0, 0);

	epd.drawFastHLine(0, 120, epd.getWidth(), EPD_BLACK);
}

void Weather::displayLocal() {
	epd.setFont(2, NULL);

	char buffer [50];
	sprintf(buffer, "Temperatur: %0.2f C", bme280Sensor.comp_data.temperature);

	epd.drawText(buffer, 0, 40);

	sprintf(buffer, "Luftfeuchte: %0.2f %%", bme280Sensor.comp_data.humidity);

	epd.drawText(buffer, 0, 65);

	sprintf(buffer, "Luftdruck: %0.2f hPa", bme280Sensor.comp_data.pressure/100);

	epd.drawText(buffer, 0, 90);
}

void Weather::displayApi() {

	JsonArray arr = api.apiObj.getArray("list");
	epd.setFont(1, NULL);
	char buffer [50];


	vfs.ls(SPIFFS_BASE_PATH"/images");

	int y = 0;
	for(int i = 4; i < arr.size(); i+=8) {
		epd.setFont(UBUNTU16_FONT, NULL);
		sprintf(buffer, "%s", timeStampToHReadble(arr.getObject(i).getInt("dt")).c_str());
		epd.drawText(buffer, 30 +(y*120), 140);

		sprintf(buffer, "%0.2f C", arr.getObject(i).getObject("main").getDouble("temp"));
		epd.drawText(buffer, 40 +(y*120), 160);

		uint8_t buf[3800];
		int length = 0;

		vfs.read(SPIFFS_BASE_PATH"/images/logo.jpg", buf, length);
		epd.drawImageJpg(100, 100, 0, buf, length);
		/*if(vfs.fileExists("10n.jpg")) {
				    ESP_LOGD(tag, ">> File Exists");
				}else{

				    ESP_LOGD(tag, ">> File Not Exists");
				}

		if(vfs.fileExists("images/10n_m.jpg")) {
		    ESP_LOGD(tag, ">> File Exists");
		}else{

		    ESP_LOGD(tag, ">> File Not Exists");
		}
*/
		if(y < 4) {
			epd.drawFastVLine(135+(y*120), 140, 200, EPD_BLACK);
		}
		y++;
	}
}

std::string Weather::timeStampToHReadble(long  timestamp)
{
    const time_t rawtime = (const time_t)timestamp;

    struct tm * dt;
    char timestr[30];
    char buffer [30];

    dt = localtime(&rawtime);
    // use any strftime format spec here
    strftime(timestr, sizeof(timestr), "%d.%m.%Y", dt);
    sprintf(buffer,"%s", timestr);
    std::string stdBuffer(buffer);
    ESP_LOGD(tag, ">> date %s", stdBuffer.c_str());
    return stdBuffer;
}
