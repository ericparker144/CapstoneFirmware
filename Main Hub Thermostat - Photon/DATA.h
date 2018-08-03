#define EEPROM_START 40 // Starting usable address of EEPROM
#define EEPROM_ZONES_START 100
#define EEPROM_VENTS_START 1074
#define PHOTON_ADC_MAX_RES 4095.0
#define PHOTON_ADC_MAX_VOLTAGE 3.3
#define ATMEGA_ADC_MAX_RES 1023.0
#define ATMEGA_ADC_MAX_VOLTAGE 3.3
#define TEMP_MODE_FAHR 1
#define TEMP_MODE_CELS 0
#define DEGREES_FULLY_OPEN 180
#define DEGREES_FULLY_CLOSED 90

#ifndef PHOTON
    #define PHOTON 1
#endif
#ifndef ATMEGA
    #define ATMEGA 0
#endif

#include <vector>

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


    uint8_t version_number;
    int temp_mode; // TEMP_MODE_CELS or TEMP_MODE_FAHR
    int num_of_zones;
    int num_of_vents;
    float time_zone;

    void save() {
        EEPROM.put(EEPROM_START, (*this));
    }

};

boolean is_router(int address) {
    return (((address & 00000000070) >> 3) == 0);
}

struct vent {

    uint32_t address;
    boolean batState;

    int zone_number() {
        return address % 8;
    }

    void save_back(int num_of_vents) {
        EEPROM.put(EEPROM_VENTS_START + (num_of_vents - 1)*sizeof(vent), (*this));
    }

    // int vent_number() {
    //     return (address & 00000000070) >> 3;
    // }

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


    void save(int selected_zone) {
        EEPROM.put(EEPROM_ZONES_START + (selected_zone - 1)*sizeof(zone), (*this));
    }


    int get_calibrated_temp(int temp_mode) {
        if (temp_mode == TEMP_MODE_FAHR) {
            return conv_adc_to_fahr(temp - calibration_factor);
        }
        else {
            return conv_adc_to_cels(temp - calibration_factor);
        }
    }

    int get_calibrated_temp_adc() {
        return temp - calibration_factor;
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

    void calibrate(int _temp, int current_adc_value, int temp_mode, int selected_zone) {
        if (temp_mode == TEMP_MODE_FAHR) {
            calibration_factor = current_adc_value - conv_fahr_to_adc(_temp);
        }
        else {
            calibration_factor = current_adc_value - conv_cels_to_adc(_temp);
        }
        save(selected_zone);
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

    void set_desired_min_temp(int _temp, int temp_mode, int selected_zone) {
        if (temp_mode == TEMP_MODE_FAHR) {
            min_temp = conv_fahr_to_adc(_temp);
        }
        else {
            min_temp = conv_cels_to_adc(_temp);
        }

        save(selected_zone);

    }

    void set_desired_max_temp(int _temp, int temp_mode, int selected_zone) {
        if (temp_mode == TEMP_MODE_FAHR) {
            max_temp = conv_fahr_to_adc(_temp);
        }
        else {
            max_temp = conv_cels_to_adc(_temp);
        }

        save(selected_zone);

    }

    void set_desired_min_temp_adc(int _temp_adc, int selected_zone) {

        min_temp = _temp_adc;
        save(selected_zone);

    }

    void set_desired_max_temp_adc(int _temp_adc, int selected_zone) {

        max_temp = _temp_adc;
        save(selected_zone);

    }

};



class timer {

private:
    uint32_t value;
    uint32_t start_time;
    boolean enabled = true;

public:
    timer() {

    }

	timer(uint32_t _time) {
        value = _time;
    }

    void setValue(uint32_t _time) {
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



class zone_command_metadata {

public:
    uint32_t address;
    int temp_indicator; // 0 if within temp range, 1 if below min_temp, 2 if above max_temp, 3 if currently in heating process
    int out_of_range_count;
};




class command_manager {

public:
    int allowable_out_of_range = 2;
    timer command_delay;
    int current_zone = 1;
    int num_of_zones;
    std::vector<zone_command_metadata> zones_to_control;
    boolean heat_on = false;
    // std::vector<zone_command_metadata> suspect_zones; // Zones that may be moved to zones_to_service if they are out of range too many times
    // std::queue<zone_command_metadata> zones_to_service; // Zones that will

    void init(std::vector<zone> zones) {
        command_delay.setValue(10000);
        command_delay.setToZero();
        num_of_zones = zones.size();
        for (auto it = zones.begin(); it != zones.end(); ++it) {
            zone_command_metadata temp = {(*it).address, 0, 0};
            zones_to_control.push_back(temp);
        }
    }

    void add_zone(zone _zone) {
        num_of_zones++;
        zone_command_metadata temp = {_zone.address, 0, 0};
        zones_to_control.push_back(temp);
    }

    // Call this every time a message is received from a router
    void update_zone(zone _zone) {
        auto it = zones_to_control.begin();
        for (it = zones_to_control.begin(); it != zones_to_control.end(); ++it) {
            if (_zone.address == (*it).address) {
                break;
            }
        }

        Serial.printf("Zone: %d, Temp: %d, MinTemp: %d, MaxTemp:%d, TempInd: %d, OutRangeCnt: %d", _zone.address, _zone.get_calibrated_temp_adc(), _zone.min_temp, _zone.max_temp, (*it).temp_indicator, (*it).out_of_range_count);
        Serial.println();



        if ((*it).temp_indicator != 3) {
            // Zone is currently not in heating process
            if (_zone.get_calibrated_temp_adc() < _zone.min_temp) {
                switch ((*it).temp_indicator) {
                    case 0:
                        (*it).out_of_range_count = 1;
                        break;
                    case 1:
                        (*it).out_of_range_count += 1;
                        break;
                    case 2:
                        (*it).out_of_range_count = 1;
                        break;
                }
                (*it).temp_indicator = 1;
            }
            else if (_zone.get_calibrated_temp_adc() > _zone.max_temp) {
                switch ((*it).temp_indicator) {
                    case 0:
                        (*it).out_of_range_count = 1;
                        break;
                    case 1:
                        (*it).out_of_range_count = 1;
                        break;
                    case 2:
                        (*it).out_of_range_count += 1;
                        break;
                }
                (*it).temp_indicator = 2;
            }
            else {
                (*it).out_of_range_count = 0;
                (*it).temp_indicator = 0;
            }
        }
        else {
            // Zone is currently being heated, wait until temperature is exceeded allowable_out_of_range times
            if (_zone.get_calibrated_temp_adc() > _zone.max_temp) {
                (*it).out_of_range_count += 1;
            }
            else {
                (*it).out_of_range_count = 0;
            }
        }
    }



    void execute() {
        if (command_delay.check()) {

            // Serial.printf("# of zones: %d", num_of_zones);
            // Serial.println();
            // Serial.printf("Current Zone: %d", current_zone);
            // Serial.println();

            auto it = zones_to_control.begin();
            for (int i = 1; i < current_zone; i++) {
                ++it;
            }

            int degrees;


            Serial.println("Check command");
            Serial.print("Address: ");
            Serial.println((*it).address);
            Serial.print("out_of_range_count: ");
            Serial.println((*it).out_of_range_count);
            Serial.println((*it).out_of_range_count >= allowable_out_of_range);



            if ((*it).out_of_range_count >= allowable_out_of_range) {

                Serial.printf("Temp indicator: %d", (*it).temp_indicator);
                Serial.println();
                Serial.println("allowable_out_of_range exceeded, issuing command");

                switch ((*it).temp_indicator) {
                    case 1:
                        degrees = DEGREES_FULLY_OPEN;
                        // Indicate that the zone needs to be heated to max_temp
                        (*it).temp_indicator = 3;
                        (*it).out_of_range_count = 0;
                        heat_on = true;
                        break;
                    case 3:
                        // If here, that means temp has been over max_temp enough times, change out of heating mode
                        (*it).temp_indicator = 2;
                        (*it).out_of_range_count = 0;
                    case 0:
                    case 2:
                        degrees = DEGREES_FULLY_CLOSED;
                        boolean needs_heat = false;
                        for (auto test_all_zones = zones_to_control.begin(); test_all_zones != zones_to_control.end(); ++test_all_zones) {
                            if (((*test_all_zones).temp_indicator == 1) || ((*test_all_zones).temp_indicator == 3)) {
                                needs_heat = true;
                            }
                        }
                        heat_on = needs_heat;
                        break;
                }

                Serial.printf("{address:%d,degrees:%d}", (*it).address, degrees);
                Serial.println();
                Serial1.printf("{address:%d,degrees:%d}", (*it).address, degrees);
                Serial.println();

            }





            if (current_zone == num_of_zones) {
                current_zone = 1;
            }
            else {
                current_zone++;
            }
            command_delay.reset();
        }
    }





    // Call this every time a new temperature value is received
    // void check_zone(zone _zone) {
    //
    //     if (_zone.temp < _zone.min_temp) {
    //         // Check to see if zone is already suspect
    //         auto it;
    //         for (it = suspect_zones.begin(); it != suspect_zones.end(); ++it) {
    //             if ((*it).address == address) {
    //                 break;
    //             }
    //         }
    //
    //         if (it == suspect_zones.end()) {
    //             // Zone not already suspect - add it to suspect vector
    //             zone_command_metadata new_suspect = {_zone.address, true, 1};
    //             suspect_zones.push_back(new_suspect);
    //         }
    //         else {
    //             // Zone already in suspect vector
    //             if ((*it).too_cold == true) {
    //                 (*it).out_of_range_count++;
    //                 if ((*it).out_of_range_count >= allowable_out_of_range) {
    //                     zones_to_service.push((*it));
    //                     suspect_zones.erase(it);
    //                 }
    //             }
    //             else {
    //                 suspect_zones.erase(it);
    //             }
    //         }
    //     }
    //     else if (_zone.temp > _zone.max_temp) {
    //
    //         // Check to see if zone is already suspect
    //         auto it;
    //         for (it = suspect_zones.begin(); it != suspect_zones.end(); ++it) {
    //             if ((*it).address == address) {
    //                 break;
    //             }
    //         }
    //
    //         if (it == suspect_zones.end()) {
    //             // Zone not already suspect - add it to suspect vector
    //             zone_command_metadata new_suspect = {_zone.address, false, 1};
    //             suspect_zones.push_back(new_suspect);
    //         }
    //         else {
    //             // Zone already in suspect vector
    //             if ((*it).too_cold == false) {
    //                 (*it).out_of_range_count++;
    //                 if ((*it).out_of_range_count >= allowable_out_of_range) {
    //                     zones_to_service.push((*it));
    //                     suspect_zones.erase(it);
    //                 }
    //             }
    //             else {
    //                 suspect_zones.erase(it);
    //             }
    //         }
    //     }
    // }


    // Call this if the zones_to_service queue is not empty
    // void send_command(std::vector<zone> zones) {
    //     if ((!zones_to_service.empty()) && (command_delay.check())) {
    //
    //         zone_command_metadata zone_to_service = zones_to_service.pop();
    //
    //         if (too_cold)
    //
    //         Serial1.printf("{address:%d,degrees:%d}", zone_to_service.address, degrees);
    //         command_delay.reset();
    //     }
    // }






};


int conv_cels_to_fahr(int temp_cels) {
    return int(float(temp_cels)*9.0/5.0 + 32.0 + 0.5);
}

int conv_fahr_to_cels(int temp_fahr) {
    return int((float(temp_fahr) - 32.0)*5.0/9.0 + 0.5);
}

int octalToDecimal(int n)
{
    int num = n;
    int dec_value = 0;

    // Initializing base value to 1, i.e 8^0
    int base = 1;

    int temp = num;
    while (temp) {

        // Extracting last digit
        int last_digit = temp % 10;
        temp = temp / 10;

        // Multiplying last digit with appropriate
        // base value and adding it to dec_value
        dec_value += last_digit * base;

        base = base * 8;
    }

    return dec_value;
}
