/*
 * bme280.cpp
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include "include/BME280.h"

static char tag[] = "BME280";


#define OVERSAMPLING_SETTINGS		UINT8_C(0x07)
/* To identify filter and standby settings selected by user */
#define FILTER_STANDBY_SETTINGS		UINT8_C(0x18)

void BME280::init() {

	i2cBus.init(0x77, GPIO_NUM_21, GPIO_NUM_22, I2C::DEFAULT_CLK_SPEED, I2C_NUM_0);

	ESP_LOGD(tag, ">> BME280Sensor");

	int8_t rslt;
	/* chip id read try count */
	uint8_t try_count = 5;
	uint8_t chip_id = 0;

	while (try_count) {
		/* Read the chip-id of bme280 sensor */
		rslt = _getRegs(BME280_CHIP_ID_ADDR, &chip_id, 1);
		/* Check for chip id validity */
		if ((rslt == BME280_OK) && (chip_id == BME280_CHIP_ID)) {
			chipId = chip_id;
			/* Reset the sensor */
			rslt = softReset();
			if (rslt == BME280_OK) {
				/* Read the calibration data */
				//rslt = get_calib_data(dev);
			}
			break;
		}
		/* Wait for 1 ms */
		vTaskDelay(1/portTICK_RATE_MS);
		--try_count;
	}
	/* Chip id check failed */
	if (!try_count) {
		ESP_LOGD(tag, ">> BME280Sensor not found");
	}

	start();

}

void BME280::run(void *data) {

	while(true) {

		ESP_LOGD(tag, ">> BME280Sensor refresh");

		streamSensorDataNormalMode();
		vTaskDelay(5000/portTICK_RATE_MS);

	}
}

int8_t BME280::softReset()
{
	int8_t rslt;
	uint8_t reg_addr = BME280_RESET_ADDR;
	/* 0xB6 is the soft reset command */
	uint8_t soft_rst_cmd = 0xB6;

	rslt = _setRegs(&reg_addr, &soft_rst_cmd, 1);

	return rslt;
}

int8_t BME280::streamSensorDataNormalMode()
{
	int8_t rslt;
	uint8_t settings_sel;
	struct bme280_data comp_data;

	/* Recommended mode of operation: Indoor navigation */
	settings.osr_h = BME280_OVERSAMPLING_1X;
	settings.osr_p = BME280_OVERSAMPLING_16X;
	settings.osr_t = BME280_OVERSAMPLING_2X;
	settings.filter = BME280_FILTER_COEFF_16;
	settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

	settings_sel = BME280_OSR_PRESS_SEL;
	settings_sel |= BME280_OSR_TEMP_SEL;
	settings_sel |= BME280_OSR_HUM_SEL;
	settings_sel |= BME280_STANDBY_SEL;
	settings_sel |= BME280_FILTER_SEL;
	//rslt = _setSensorSettings(settings_sel);
	//rslt = _setSensorMode(BME280_NORMAL_MODE);

	printf("Temperature, Pressure, Humidity %d\r\n", chipId);
	rslt = _getSensorData(BME280_ALL, &comp_data);
	printSensorData(&comp_data);

	return rslt;
}

void BME280::printSensorData(struct bme280_data *comp_data)
{
	ESP_LOGD(tag, ">> BME280Sensor %0.2f, %0.2f, %0.2f\r\n",(double)comp_data->temperature, (double)comp_data->pressure, (double)comp_data->humidity);
}

int8_t BME280::_setSensorSettings(uint8_t desired_settings)
{
	int8_t rslt;
	uint8_t sensor_mode;

	rslt = getSensorMode(&sensor_mode);
	if ((rslt == BME280_OK) && (sensor_mode != BME280_SLEEP_MODE))
		rslt = putDeviceToSleep();
	if (rslt == BME280_OK) {

		if (_areSettingsChanged(OVERSAMPLING_SETTINGS, desired_settings))
			rslt = _setOsrSettings(desired_settings);
		if ((rslt == BME280_OK) && _areSettingsChanged(FILTER_STANDBY_SETTINGS, desired_settings))
			rslt = _setFilterStandbySettings(desired_settings);
	}

	rslt = BME280_OK;

	return rslt;
}

uint8_t BME280::_areSettingsChanged(uint8_t sub_settings, uint8_t desired_settings)
{
	uint8_t settings_changed = FALSE;

	if (sub_settings & desired_settings) {
		/* User wants to modify this particular settings */
		settings_changed = TRUE;
	} else {
		/* User don't want to modify this particular settings */
		settings_changed = FALSE;
	}

	return settings_changed;
}

int8_t BME280::_setSensorMode(uint8_t sensor_mode)
{
	int8_t rslt;
	uint8_t last_set_mode;

		rslt = getSensorMode(&last_set_mode);
	if ((rslt == BME280_OK) && (last_set_mode != BME280_SLEEP_MODE))
		rslt = putDeviceToSleep();
	if (rslt == BME280_OK)
		rslt = writePowerMode(sensor_mode);

	rslt = BME280_OK;
	return rslt;
}

int8_t BME280::putDeviceToSleep()
{
	int8_t rslt;
	uint8_t reg_data[4];

	rslt = _getRegs(BME280_CTRL_HUM_ADDR, reg_data, 4);
	if (rslt == BME280_OK) {
		_parseDeviceSettings(reg_data);
		rslt = softReset();
		if (rslt == BME280_OK)
			rslt = reloadDeviceSettings();
	}

	return rslt;
}

void BME280::_parseDeviceSettings(const uint8_t *reg_data)
{
	settings.osr_h = BME280_GET_BITS_POS_0(reg_data[0], BME280_CTRL_HUM);
	settings.osr_p = BME280_GET_BITS(reg_data[2], BME280_CTRL_PRESS);
	settings.osr_t = BME280_GET_BITS(reg_data[2], BME280_CTRL_TEMP);
	settings.filter = BME280_GET_BITS(reg_data[3], BME280_FILTER);
	settings.standby_time = BME280_GET_BITS(reg_data[3], BME280_STANDBY);
}

int8_t BME280::reloadDeviceSettings()
{
	int8_t rslt;

	rslt = _setOsrSettings(BME280_ALL_SETTINGS_SEL);
	if (rslt == BME280_OK)
		rslt = _setFilterStandbySettings(BME280_ALL_SETTINGS_SEL);

	return rslt;
}

int8_t BME280::writePowerMode(uint8_t sensor_mode)
{
	int8_t rslt;
	uint8_t reg_addr = BME280_PWR_CTRL_ADDR;
	/* Variable to store the value read from power mode register */
	uint8_t sensor_mode_reg_val;

	/* Read the power mode register */
	rslt = _getRegs(reg_addr, &sensor_mode_reg_val, 1);
	/* Set the power mode */
	if (rslt == BME280_OK) {
		sensor_mode_reg_val = BME280_SET_BITS_POS_0(sensor_mode_reg_val, BME280_SENSOR_MODE, sensor_mode);
		/* Write the power mode in the register */
		rslt = _setRegs(&reg_addr, &sensor_mode_reg_val, 1);
	}

	return rslt;
}

int8_t BME280::_setOsrSettings(uint8_t desired_settings)
{
	int8_t rslt = BME280_W_INVALID_OSR_MACRO;

	if (desired_settings & BME280_OSR_HUM_SEL)
		rslt = _setOsrHumiditySettings();
	if (desired_settings & (BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL))
		rslt = _setOsrPressTempSettings(desired_settings);

	return rslt;
}

int8_t BME280::_setOsrHumiditySettings()
{
	int8_t rslt;
	uint8_t ctrl_hum;
	uint8_t ctrl_meas;
	uint8_t reg_addr = BME280_CTRL_HUM_ADDR;

	ctrl_hum = settings.osr_h & BME280_CTRL_HUM_MSK;
	/* Write the humidity control value in the register */
	rslt = _setRegs(&reg_addr, &ctrl_hum, 1);
	/* Humidity related changes will be only effective after a
	   write operation to ctrl_meas register */
	if (rslt == BME280_OK) {
		reg_addr = BME280_CTRL_MEAS_ADDR;
		rslt = _getRegs(reg_addr, &ctrl_meas, 1);
		if (rslt == BME280_OK)
			rslt = _setRegs(&reg_addr, &ctrl_meas, 1);
	}

	return rslt;
}

int8_t BME280::_setOsrPressTempSettings(uint8_t desired_settings)
{
	int8_t rslt;
	uint8_t reg_addr = BME280_CTRL_MEAS_ADDR;
	uint8_t reg_data;

	rslt = _getRegs(reg_addr, &reg_data, 1);

	if (rslt == BME280_OK) {
		if (desired_settings & BME280_OSR_PRESS_SEL)
			_fillOsrPressSettings(&reg_data);
		if (desired_settings & BME280_OSR_TEMP_SEL)
			_fillOsrTempSettings(&reg_data);
		/* Write the oversampling settings in the register */
		rslt = _setRegs(&reg_addr, &reg_data, 1);
	}

	return rslt;
}


int8_t BME280::_setFilterStandbySettings(uint8_t desired_settings)
{
	int8_t rslt;
	uint8_t reg_addr = BME280_CONFIG_ADDR;
	uint8_t reg_data;

	rslt = _getRegs(reg_addr, &reg_data, 1);

		if (desired_settings & BME280_FILTER_SEL)
			_fillFilterSettings(&reg_data);
		if (desired_settings & BME280_STANDBY_SEL)
			_fillStandbySettings(&reg_data);
		/* Write the oversampling settings in the register */
	rslt = _setRegs(&reg_addr, &reg_data, 1);
	rslt = BME280_OK;

	return rslt;
}

void BME280::_fillFilterSettings(uint8_t *reg_data)
{
	*reg_data = BME280_SET_BITS(*reg_data, BME280_FILTER, settings.filter);
}

void BME280::_fillStandbySettings(uint8_t *reg_data)
{
	*reg_data = BME280_SET_BITS(*reg_data, BME280_STANDBY, settings.standby_time);
}

void BME280::_fillOsrPressSettings(uint8_t *reg_data)
{
	*reg_data = BME280_SET_BITS(*reg_data, BME280_CTRL_PRESS, settings.osr_p);
}

void BME280::_fillOsrTempSettings(uint8_t *reg_data)
{
	*reg_data = BME280_SET_BITS(*reg_data, BME280_CTRL_TEMP, settings.osr_t);
}

int8_t BME280::_getSensorData(uint8_t sensor_comp, struct bme280_data *comp_data)
{
	int8_t rslt;
	/* Array to store the pressure, temperature and humidity data read from
	the sensor */
	uint8_t reg_data[BME280_P_T_H_DATA_LEN] = {0};
	struct bme280_uncomp_data uncomp_data = {0};

	/* Check for null pointer in the device structure*/

	if ((comp_data != NULL)) {
		/* Read the pressure and temperature data from the sensor */
		rslt = _getRegs(BME280_DATA_ADDR, reg_data, BME280_P_T_H_DATA_LEN);
		printf("READ %d\r\n", reg_data[1]);
		if (rslt == BME280_OK) {
			/* Parse the read data from the sensor */
			_parseSensorData(reg_data, &uncomp_data);
			/* Compensate the pressure and/or temperature and/or
			   humidity data from the sensor */
			rslt = _compensateData(sensor_comp, &uncomp_data, comp_data);
		}

		rslt = BME280_OK;
	} else {
		rslt = BME280_E_NULL_PTR;
	}


	return rslt;
}

int8_t BME280::_compensateData(uint8_t sensor_comp, const struct bme280_uncomp_data *uncomp_data,
				     struct bme280_data *comp_data)
{
	int8_t rslt = BME280_OK;

	if ((uncomp_data != NULL) && (comp_data != NULL)) {
		/* Initialize to zero */
		comp_data->temperature = 0;
		comp_data->pressure = 0;
		comp_data->humidity = 0;
		/* If pressure or temperature component is selected */
		if (sensor_comp & (BME280_PRESS | BME280_TEMP | BME280_HUM)) {
			/* Compensate the temperature data */
			comp_data->temperature = _compensateTemperature(uncomp_data);
		}
		if (sensor_comp & BME280_PRESS) {
			/* Compensate the pressure data */
			comp_data->pressure = _compensatePressure(uncomp_data);
		}
		if (sensor_comp & BME280_HUM) {
			/* Compensate the humidity data */
			comp_data->humidity = _compensateHumidity(uncomp_data);
		}
	} else {
		rslt = BME280_E_NULL_PTR;
	}

	return rslt;
}

void BME280::_parseSensorData(const uint8_t *reg_data, struct bme280_uncomp_data *uncomp_data)
{
	/* Variables to store the sensor data */
	uint32_t data_xlsb;
	uint32_t data_lsb;
	uint32_t data_msb;

	/* Store the parsed register values for pressure data */
	data_msb = (uint32_t)reg_data[0] << 12;
	data_lsb = (uint32_t)reg_data[1] << 4;
	data_xlsb = (uint32_t)reg_data[2] >> 4;
	uncomp_data->pressure = data_msb | data_lsb | data_xlsb;

	/* Store the parsed register values for temperature data */
	data_msb = (uint32_t)reg_data[3] << 12;
	data_lsb = (uint32_t)reg_data[4] << 4;
	data_xlsb = (uint32_t)reg_data[5] >> 4;
	uncomp_data->temperature = data_msb | data_lsb | data_xlsb;

	/* Store the parsed register values for temperature data */
	data_lsb = (uint32_t)reg_data[6] << 8;
	data_msb = (uint32_t)reg_data[7];
	uncomp_data->humidity = data_msb | data_lsb;
}

int8_t BME280::getSensorMode(uint8_t *sensor_mode)
{
	int8_t rslt;

	rslt = _getRegs(BME280_PWR_CTRL_ADDR, sensor_mode, 1);
		/* Assign the power mode in the device structure */
	*sensor_mode = BME280_GET_BITS_POS_0(*sensor_mode, BME280_SENSOR_MODE);
	rslt = BME280_OK;

	return rslt;
}

int8_t BME280::_getRegs(uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
	int8_t rslt;

	i2cBus.setAddress(BME280_I2C_ADDR_SEC);
	i2cBus.beginTransaction();
	i2cBus.write(reg_addr, true);
	i2cBus.start();
	i2cBus.read(reg_data, len, false);
	i2cBus.endTransaction();

	rslt = BME280_OK;

	return rslt;
}

int8_t BME280::_setRegs(uint8_t *reg_addr, const uint8_t *reg_data, uint8_t len)
{
	int8_t rslt;
	uint8_t temp_buff[20]; /* Typically not to write more than 10 registers */

	if (len > 10)
		len = 10;

	uint16_t temp_len;

	/* Check for arguments validity */
	if ((reg_addr != NULL) && (reg_data != NULL)) {
		if (len != 0) {
			temp_buff[0] = reg_data[0];

			/* Burst write mode */
			if (len > 1) {
				/* Interleave register address w.r.t data for
				burst write*/
				_interleaveRegAddr(reg_addr, temp_buff, reg_data, len);
				temp_len = len * 2;
			} else {
				temp_len = len;
			}

			i2cBus.setAddress(BME280_I2C_ADDR_SEC);
			i2cBus.beginTransaction();
			i2cBus.write(reg_addr, true);
			i2cBus.start();
			i2cBus.write(temp_buff, temp_len, true);
			i2cBus.endTransaction();
			/* Check for communication error */
			rslt = BME280_OK;
		} else {
			rslt = BME280_E_INVALID_LEN;
		}
	} else {
		rslt = BME280_E_NULL_PTR;
	}


	return rslt;
}

void BME280::_interleaveRegAddr(const uint8_t *reg_addr, uint8_t *temp_buff, const uint8_t *reg_data, uint8_t len)
{
	uint8_t index;

	for (index = 1; index < len; index++) {
		temp_buff[(index * 2) - 1] = reg_addr[index];
		temp_buff[index * 2] = reg_data[index];
	}
}

int8_t BME280::_getCalibData()
{
	int8_t rslt;
	uint8_t reg_addr = BME280_TEMP_PRESS_CALIB_DATA_ADDR;
	/* Array to store calibration data */
	uint8_t calib_data[BME280_TEMP_PRESS_CALIB_DATA_LEN] = {0};

	/* Read the calibration data from the sensor */
	rslt = _getRegs(reg_addr, calib_data, BME280_TEMP_PRESS_CALIB_DATA_LEN);

	if (rslt == BME280_OK) {
		/* Parse temperature and pressure calibration data and store
		   it in device structure */
		_parseTempPressCalibData(calib_data);

		reg_addr = BME280_HUMIDITY_CALIB_DATA_ADDR;
		/* Read the humidity calibration data from the sensor */
		rslt = _getRegs(reg_addr, calib_data, BME280_HUMIDITY_CALIB_DATA_LEN);
		if (rslt == BME280_OK) {
			/* Parse humidity calibration data and store it in
			   device structure */
			_parseHumidityCalibData(calib_data);
		}
	}

	return rslt;
}

void BME280::_parseTempPressCalibData(const uint8_t *reg_data)
{

	calib_data.dig_T1 = BME280_CONCAT_BYTES(reg_data[1], reg_data[0]);
	calib_data.dig_T2 = (int16_t)BME280_CONCAT_BYTES(reg_data[3], reg_data[2]);
	calib_data.dig_T3 = (int16_t)BME280_CONCAT_BYTES(reg_data[5], reg_data[4]);
	calib_data.dig_P1 = BME280_CONCAT_BYTES(reg_data[7], reg_data[6]);
	calib_data.dig_P2 = (int16_t)BME280_CONCAT_BYTES(reg_data[9], reg_data[8]);
	calib_data.dig_P3 = (int16_t)BME280_CONCAT_BYTES(reg_data[11], reg_data[10]);
	calib_data.dig_P4 = (int16_t)BME280_CONCAT_BYTES(reg_data[13], reg_data[12]);
	calib_data.dig_P5 = (int16_t)BME280_CONCAT_BYTES(reg_data[15], reg_data[14]);
	calib_data.dig_P6 = (int16_t)BME280_CONCAT_BYTES(reg_data[17], reg_data[16]);
	calib_data.dig_P7 = (int16_t)BME280_CONCAT_BYTES(reg_data[19], reg_data[18]);
	calib_data.dig_P8 = (int16_t)BME280_CONCAT_BYTES(reg_data[21], reg_data[20]);
	calib_data.dig_P9 = (int16_t)BME280_CONCAT_BYTES(reg_data[23], reg_data[22]);
	calib_data.dig_H1 = reg_data[25];

}

/*!
 *  @brief This internal API is used to parse the humidity calibration data
 *  and store it in device structure.
 */
void BME280::_parseHumidityCalibData(const uint8_t *reg_data)
{
	int16_t dig_H4_lsb;
	int16_t dig_H4_msb;
	int16_t dig_H5_lsb;
	int16_t dig_H5_msb;

	calib_data.dig_H2 = (int16_t)BME280_CONCAT_BYTES(reg_data[1], reg_data[0]);
	calib_data.dig_H3 = reg_data[2];

	dig_H4_msb = (int16_t)(int8_t)reg_data[3] * 16;
	dig_H4_lsb = (int16_t)(reg_data[4] & 0x0F);
	calib_data.dig_H4 = dig_H4_msb | dig_H4_lsb;

	dig_H5_msb = (int16_t)(int8_t)reg_data[5] * 16;
	dig_H5_lsb = (int16_t)(reg_data[4] >> 4);
	calib_data.dig_H5 = dig_H5_msb | dig_H5_lsb;
	calib_data.dig_H6 = (int8_t)reg_data[6];
}

double BME280::_compensateTemperature(const struct bme280_uncomp_data *uncomp_data)
{
	double var1;
	double var2;
	double temperature;
	double temperature_min = -40;
	double temperature_max = 85;
	ESP_LOGD(tag, ">> BME280Sensor TEMP %0.2f\r\n",(double)uncomp_data->temperature);

	var1 = ((double)uncomp_data->temperature) / 16384.0 - ((double)calib_data.dig_T1) / 1024.0;
	var1 = var1 * ((double)calib_data.dig_T2);
	var2 = (((double)uncomp_data->temperature) / 131072.0 - ((double)calib_data.dig_T1) / 8192.0);
	var2 = (var2 * var2) * ((double)calib_data.dig_T3);
	calib_data.t_fine = (int32_t)(var1 + var2);
	temperature = (var1 + var2) / 5120.0;

	if (temperature < temperature_min)
		temperature = temperature_min;
	else if (temperature > temperature_max)
		temperature = temperature_max;

	return temperature;
}

/*!
 * @brief This internal API is used to compensate the raw pressure data and
 * return the compensated pressure data in double data type.
 */
double BME280::_compensatePressure(const struct bme280_uncomp_data *uncomp_data)
{
	double var1;
	double var2;
	double var3;
	double pressure;
	double pressure_min = 30000.0;
	double pressure_max = 110000.0;

	var1 = ((double)calib_data.t_fine / 2.0) - 64000.0;
	var2 = var1 * var1 * ((double)calib_data.dig_P6) / 32768.0;
	var2 = var2 + var1 * ((double)calib_data.dig_P5) * 2.0;
	var2 = (var2 / 4.0) + (((double)calib_data.dig_P4) * 65536.0);
	var3 = ((double)calib_data.dig_P3) * var1 * var1 / 524288.0;
	var1 = (var3 + ((double)calib_data.dig_P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0) * ((double)calib_data.dig_P1);
	/* avoid exception caused by division by zero */
	if (var1) {
		pressure = 1048576.0 - (double) uncomp_data->pressure;
		pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
		var1 = ((double)calib_data.dig_P9) * pressure * pressure / 2147483648.0;
		var2 = pressure * ((double)calib_data.dig_P8) / 32768.0;
		pressure = pressure + (var1 + var2 + ((double)calib_data.dig_P7)) / 16.0;

		if (pressure < pressure_min)
			pressure = pressure_min;
		else if (pressure > pressure_max)
			pressure = pressure_max;
	} else { /* Invalid case */
		pressure = pressure_min;
	}

	return pressure;
}

/*!
 * @brief This internal API is used to compensate the raw humidity data and
 * return the compensated humidity data in double data type.
 */
double BME280::_compensateHumidity(const struct bme280_uncomp_data *uncomp_data)
{
	double humidity;
	double humidity_min = 0.0;
	double humidity_max = 100.0;
	double var1;
	double var2;
	double var3;
	double var4;
	double var5;
	double var6;

	var1 = ((double)calib_data.t_fine) - 76800.0;
	var2 = (((double)calib_data.dig_H4) * 64.0 + (((double)calib_data.dig_H5) / 16384.0) * var1);
	var3 = uncomp_data->humidity - var2;
	var4 = ((double)calib_data.dig_H2) / 65536.0;
	var5 = (1.0 + (((double)calib_data.dig_H3) / 67108864.0) * var1);
	var6 = 1.0 + (((double)calib_data.dig_H6) / 67108864.0) * var1 * var5;
	var6 = var3 * var4 * (var5 * var6);
	humidity = var6 * (1.0 - ((double)calib_data.dig_H1) * var6 / 524288.0);

	if (humidity > humidity_max)
		humidity = humidity_max;
	else if (humidity < humidity_min)
		humidity = humidity_min;

	return humidity;
}
