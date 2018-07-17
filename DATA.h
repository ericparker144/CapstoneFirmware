#define EEPROM_START 40 // Starting usable address of EEPROM

// EEPROM Structure
//          ________________
// 0 - 39: |__Screen Stuff__|
// 40:     |___# of Zones___|
//         |________________|
//         |________________|
//         |________________|
//         |________________|
//         |________________|






class zone {

public:
    int temp;
    int min_temp;
    int max_temp;
    int heat_or_cool;
    char zone_name[20];
    uint32_t address; // Address

    uint32_t calibration_factor;







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
        if ((abs(millis()- start_time) > value) && enabled) {
            return true;
        }
        else {
            return false;
        }
    }

	void reset() {
        start_time = millis();
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }
};
