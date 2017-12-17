/*
 * bme280.h
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "I2C.h"
#include "BME280_defs.h"

#ifndef COMPONENTS_BME280_INCLUDE_BME280_H_
#define COMPONENTS_BME280_INCLUDE_BME280_H_


class BME280 {
private:
	I2C i2cBus;
	uint8_t chipId = 0;
	bme280_calib_data calib_data;
	bme280_settings settings;

	int8_t _getRegs(uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
	int8_t _setRegs(uint8_t *reg_addr, const uint8_t *reg_data, uint8_t len);
	int8_t _getCalibData();
	void _interleaveRegAddr(const uint8_t *reg_addr, uint8_t *temp_buff, const uint8_t *reg_data, uint8_t len);
	void _parseTempPressCalibData(const uint8_t *reg_data);
	void _parseHumidityCalibData(const uint8_t *reg_data);
	int8_t _setSensorSettings(uint8_t desired_settings);
	int8_t _setSensorMode(uint8_t sensor_mode);
	int8_t _getSensorData(uint8_t sensor_comp);
	void _parseSensorData(const uint8_t *reg_data, struct bme280_uncomp_data *uncomp_data);
	int8_t _setOsrSettings(uint8_t desired_settings);
	void _fillFilterSettings(uint8_t *reg_data);
	void _fillStandbySettings(uint8_t *reg_data);
	void _fillOsrTempSettings(uint8_t *reg_data);
	void _fillOsrPressSettings(uint8_t *reg_data);
	int8_t _setOsrPressTempSettings(uint8_t desired_settings);
	int8_t _setOsrHumiditySettings();
	double _compensateTemperature(const struct bme280_uncomp_data *uncomp_data);
	int8_t _setFilterStandbySettings(uint8_t desired_settings);
	double _compensatePressure(const struct bme280_uncomp_data *uncomp_data);
	void _parseDeviceSettings(const uint8_t *reg_data);
	double _compensateHumidity(const struct bme280_uncomp_data *uncomp_data);
	int8_t _compensateData(uint8_t sensor_comp, const struct bme280_uncomp_data *uncomp_data);
	uint8_t _areSettingsChanged(uint8_t sub_settings, uint8_t desired_settings);
public:
	struct bme280_data comp_data;
	void init();
	int8_t softReset();
	void printSensorData();
	int8_t streamSensorDataNormalMode();
	int8_t getSensorMode(uint8_t *sensor_mode);
	int8_t putDeviceToSleep();
	int8_t reloadDeviceSettings();
	int8_t writePowerMode(uint8_t sensor_mode);
};

#endif /* COMPONENTS_BME280_INCLUDE_BME280_H_ */
