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
    int num_of_vents;
    float time_zone;

};




struct vent {

    uint32_t address;
    boolean batState;

    int zone_number() {
        return (address & 00000000070) >> 3;
    }

    int vent_number() {
        return address % 8;
    }

};


// Hey, since Photon never directly pairs with the vents, what is the procedure for finding out
// they exist and who they belong to?
//
// The coordinator is really only concerned with the routers (which he has to pair with
// hence they will all be known) The routers then pair with their respective vents so they
// have all the vents connected to them (since you have to pair vents with routers). So
// the coordinator has an array of all the connectd routers and each router has all its
// connected vents. The coordinator knows the battery of each vent because they all send their
// battery level in type E messages to the coordinator once they pair with a router.
//
//
// Few questions:
// 1) This means the pairing order must be coordinator with router(s), and them routers with
// respective vent(s)?
// 2) Are the batState messages from each vent only sent when the pairing occurs? Or are messages
// from the vent to the coordinator sent at regular intervals?
// 3) We are dealing with

// 00
// 01
// 02
// 03
//
// 011
// 012




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
    boolean batState;


    int get_calibrated_temp(int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return conv_adc_to_fahr(temp - calibration_factor);
        }
        else {
            return conv_adc_to_cels(temp - calibration_factor);
        }
    }



    int conv_adc_to_temp(int temp_adc, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return conv_adc_to_fahr(temp_adc);
        }
        else {
            return conv_adc_to_cels(temp_adc);
        }
    }

    int conv_temp_to_adc(int _temp, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return conv_fahr_to_adc(_temp);
        }
        else {
            return conv_cels_to_adc(_temp);
        }
    }

    void calibrate(int _temp, int current_adc_value, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            calibration_factor = current_adc_value - conv_fahr_to_adc(_temp);
        }
        else {
            calibration_factor = current_adc_value - conv_cels_to_adc(_temp);
        }
    }



    int conv_adc_to_cels(int temp_adc) {
        // 0.5 added to implement rounding to nearest whole number
        if (device_type == PHOTON) {
            return int((float(temp_adc)*PHOTON_ADC_MAX_VOLTAGE/PHOTON_ADC_MAX_RES*100.0 - 273.15) + 0.5);
        }
        else {
            return int((float(temp_adc)*ATMEGA_ADC_MAX_VOLTAGE/ATMEGA_ADC_MAX_RES*100.0 - 273.15) + 0.5);
        }
    }

    int conv_adc_to_fahr(int temp_adc) {
        // 0.5 added to implement rounding to nearest whole number
        if (device_type == PHOTON) {
            return int((float(temp_adc)*PHOTON_ADC_MAX_VOLTAGE/PHOTON_ADC_MAX_RES*100.0 - 273.15)*9.0/5.0 + 32.0 + 0.5);
        }
        else {
            return int((float(temp_adc)*ATMEGA_ADC_MAX_VOLTAGE/ATMEGA_ADC_MAX_RES*100.0 - 273.15)*9.0/5.0 + 32.0 + 0.5);
        }
    }

    int conv_cels_to_adc(int temp_cels) {
        if (device_type == PHOTON) {
            return (int((float(temp_cels) + 273.15)*PHOTON_ADC_MAX_RES/(100.0*PHOTON_ADC_MAX_VOLTAGE) + 0.5));
        }
        else {
            return (int((float(temp_cels) + 273.15)*ATMEGA_ADC_MAX_RES/(100.0*ATMEGA_ADC_MAX_VOLTAGE) + 0.5));
        }
    }

    int conv_fahr_to_adc(int temp_fahr) {
        if (device_type == PHOTON) {
            return (int((((float(temp_fahr) - 32.0)*5.0/9.0) + 273.15)*PHOTON_ADC_MAX_RES/(100.0*PHOTON_ADC_MAX_VOLTAGE) + 0.5));
        }
        else {
            return (int((((float(temp_fahr) - 32.0)*5.0/9.0) + 273.15)*ATMEGA_ADC_MAX_RES/(100.0*ATMEGA_ADC_MAX_VOLTAGE) + 0.5));
        }
    }

    void set_desired_min_temp(int _temp, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            min_temp = conv_fahr_to_adc(_temp);
        }
        else {
            min_temp = conv_cels_to_adc(_temp);

        }
    }

    void set_desired_max_temp(int _temp, int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            max_temp = conv_fahr_to_adc(_temp);
        }
        else {
            max_temp = conv_cels_to_adc(_temp);

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


int conv_cels_to_fahr(int temp_cels) {
    return int(float(temp_cels)*9.0/5.0 + 32.0 + 0.5);
}

int conv_fahr_to_cels(int temp_fahr) {
    return int((float(temp_fahr) - 32.0)*5.0/9.0 + 0.5);
}
