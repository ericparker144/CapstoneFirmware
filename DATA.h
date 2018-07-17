#define EEPROM_START 40 // Starting usable address of EEPROM
#define PHOTON_ADC_MAX_RES 4095.0
#define PHOTON_ADC_MAX_VOLTAGE 3.3
#define ATMEGA_ADC_MAX_RES 1023.0
#define ATMEGA_ADC_MAX_VOLTAGE 3.3
#define TEMP_MODE_FAHR 1
#define TEMP_MODE_CELS 0

#ifndef PHOTON
    #define PHOTON 1
#endif
#ifndef ATMEGA
    #define ATMEGA 0
#endif


// EEPROM Structure
//          _____________________
// 0 - 39: |____Screen Stuff_____|
// 40:     |___settings struct___|
//         |_____________________|
//         |_____________________|
//         |_____________________|
//         |_____________________|
//         |_____________________|



struct custom_settings {

    int temp_mode; // TEMP_MODE_CELS or TEMP_MODE_FAHR
    int num_of_zones;
    float time_zone;

};




struct zone {

    // int min_temp_fahr; // desired minimum temperature - fahrenheit
    // int min_temp_cels; // desired minimum temperature - celsius
    //
    // int max_temp_fahr; // desired maximum temperature - fahrenheit
    // int max_temp_cels; // desired maximum temperature - celsius


public:
    int device_type; // PHOTON or ATMEGA
    int temp; // temperature reading - raw ADC
    int min_temp; // min allowable temp - raw ADC
    int max_temp; // max allowable temp - raw ADC
    int heat_or_cool;
    char zone_name[20];
    uint32_t address; // Address
    uint32_t calibration_factor;

    int conv_adc_to_temp(int temp_adc, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return conv_adc_to_fahr(temp_adc);
        }
        else {
            return conv_adc_to_cels(temp_adc);
        }
    }

    int conv_temp_to_adc(int temp, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return conv_fahr_to_adc(temp);
        }
        else {
            return conv_cels_to_adc(temp);
        }
    }

    int increment_temp_adc(int temp_adc, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return temp_adc + conv_fahr_to_adc(1);
        }
        else {
            return temp_adc + conv_cels_to_adc(1);
        }
    }

    int decrement_temp_adc(int temp_adc, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return temp_adc - conv_fahr_to_adc(1);
        }
        else {
            return temp_adc - conv_cels_to_adc(1);
        }
    }



    int conv_adc_to_cels(int temp_adc) {
        // 0.5 added to implement rounding to nearest whole number
        if (device_type == PHOTON) {
            return int((float(temp_adc + calibration_factor)*PHOTON_ADC_MAX_VOLTAGE/PHOTON_ADC_MAX_RES*100.0 - 273.15) + 0.5);
        }
        else {
            return int((float(temp_adc + calibration_factor)*ATMEGA_ADC_MAX_VOLTAGE/ATMEGA_ADC_MAX_RES*100.0 - 273.15) + 0.5);
        }
    }

    int conv_adc_to_fahr(int temp_adc) {
        // 0.5 added to implement rounding to nearest whole number
        if (device_type == PHOTON) {
            return int((float(temp_adc + calibration_factor)*PHOTON_ADC_MAX_VOLTAGE/PHOTON_ADC_MAX_RES*100.0 - 273.15)*9.0/5.0 + 32.0 + 0.5);
        }
        else {
            return int((float(temp_adc + calibration_factor)*ATMEGA_ADC_MAX_VOLTAGE/ATMEGA_ADC_MAX_RES*100.0 - 273.15)*9.0/5.0 + 32.0 + 0.5);
        }
    }

    int conv_cels_to_adc(int temp_cels) {
        if (device_type == PHOTON) {
            return (int((float(temp_cels) + 273.15)*PHOTON_ADC_MAX_RES/(100.0*PHOTON_ADC_MAX_VOLTAGE) + 0.5)) - calibration_factor;
        }
        else {
            return (int((float(temp_cels) + 273.15)*ATMEGA_ADC_MAX_RES/(100.0*ATMEGA_ADC_MAX_VOLTAGE) + 0.5)) - calibration_factor;
        }
    }

    int conv_fahr_to_adc(int temp_fahr) {
        if (device_type == PHOTON) {
            return (int((((float(temp_fahr) - 32.0)*5.0/9.0) + 273.15)*PHOTON_ADC_MAX_RES/(100.0*PHOTON_ADC_MAX_VOLTAGE) + 0.5)) - calibration_factor;
        }
        else {
            return (int((((float(temp_fahr) - 32.0)*5.0/9.0) + 273.15)*ATMEGA_ADC_MAX_RES/(100.0*ATMEGA_ADC_MAX_VOLTAGE) + 0.5)) - calibration_factor;
        }
    }


    int conv_cels_to_fahr(int temp_cels) {
        return int(float(temp_cels)*9.0/5.0 + 32.0 + 0.5);
    }

    int conv_fahr_to_cels(int temp_fahr) {
        return int((float(temp_fahr) - 32.0)*5.0/9.0 + 0.5);
    }

    void set_desired_min_temp(int temp, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            min_temp = conv_fahr_to_adc(temp);
        }
        else {
            min_temp = conv_cels_to_adc(temp);

        }
    }

    void set_desired_max_temp(int temp, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            max_temp = conv_fahr_to_adc(temp);
        }
        else {
            max_temp = conv_cels_to_adc(temp);

        }
    }

};



class timer {

private:
    uint32_t value;
    uint32_t start_time;
    boolean enabled = true;

public:
	timer(uint32_t _time) {
        value = _time;
    }

	boolean check() {
        if ((abs(millis() - start_time) > value) && enabled) {
            return true;
        }
        else {
            return false;
        }
    }

	void reset() {
        start_time = millis();
    }

    void setToZero() {
        start_time = 0;
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }
};
