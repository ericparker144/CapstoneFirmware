#include "GD3/GD3.h"
#include "HMI.h"
#include "DATA.h"
#include <vector>

#define VERSION_NUMBER 2
#define SW 800 // Screen Width
#define SH 480 // Screen Height
#define TEMPERATURE_READ_INTERVAL 10000
#define CLOUD_UPDATE_INTERVAL 60000
#define SERIAL_BUFFER_SIZE 100
#define DEFAULT_MIN_TEMP_FAHR 68
#define DEFAULT_MAX_TEMP_FAHR 74
#define DEFAULT_MIN_TEMP_CELS 20
#define DEFAULT_MAX_TEMP_CELS 24

SYSTEM_THREAD(ENABLED);

boolean screen_SPI_active = false;
uint32_t time_since_lcd_check_in = 0;

char serial_buffer[SERIAL_BUFFER_SIZE] = {'\0'};
size_t serial_buffer_offset = 0;

String cloud_data = "";

custom_settings user_settings;
char previousTouch;
char tag;
unsigned long timeSinceLCDUpdate = 0;
int networks_found;
WiFiAccessPoint aps[5];
int wifi_network_selected = 0;
char wifi_password[25] = {'\0'};
int calibration_helper;
char zone_name_helper[20] = {'\0'};

command_manager temp_commander;


// Variables used to implement dragging
boolean touch_drag = false;
int x_drag;
int y_drag;
int x_start = 0;
int y_start = 0;
int subpage = 0;
int temp_selector = 0;



pages screen = MAIN;
pages previousScreen = MAIN;
settings_pages settings_screen = WIFI;
settings_pages previous_settings_screen = WIFI;
keyboard gui_keyboard(SW/4+10+40, 80+(SH-80)/2-10, 5, 45, 0x215968, 29);
button wifi_scan = {3, SW-10-70, 80+40+30+60-15, 70, 30, 28, 0x215968, "Scan"};
button wifi_connect = {9, (SW/4+SW)/2-5-100, 80+40+40+40-5, 100, 60, 28, 0x215968, "Connect"};
button wifi_cancel = {10, (SW/4+SW)/2+5, 80+40+40+40-5, 100, 60, 28, 0x215968, "Cancel"};
button calibrate_btn = {12, (SW/4 + SW)/2-125,SH/2+40+40+80-20,250,40, 29, 0x215968, "Done"};
button new_name_save = {1, SW/2-150-5, 80+40+40+40-5-20, 150, 60, 28, 0x215968, "Save"};
button new_name_cancel = {2, SW/2+5, 80+40+40+40-5-20, 150, 60, 28, 0x215968, "Cancel"};

timer main_hub_temp_timer(TEMPERATURE_READ_INTERVAL);
timer cloud_update_timer(CLOUD_UPDATE_INTERVAL);
int selected_zone = 1;
std::vector<zone> zones;
std::vector<vent> vents;





void handleSerialInput(std::vector<zone> & zones, std::vector<vent> & vents) {

    String param1 = "address:";
    String param2 = "batState:";
    String param3 = "temp:";

    Serial.print("Message received: ");
    Serial.println(serial_buffer);
    String serial_string = String(serial_buffer);

    int address = octalToDecimal(serial_string.substring(serial_string.indexOf(param1) + param1.length(), serial_string.indexOf(param2) - 1).toInt());
    boolean batState = boolean(serial_string.substring(serial_string.indexOf(param2) + param2.length(), serial_string.indexOf(param3) - 1).toInt());
    int temp_val = serial_string.substring(serial_string.indexOf(param3) + param3.length(), serial_string.indexOf("}")).toInt();


    // Update information about the specific zone or vent, or add new zone/vent if not found
    if (temp_val == 0) {
        Serial.println("Is a vent");
        // Message was from a vent
        boolean device_found = false;

        for (auto it = vents.begin(); it != vents.end(); ++it) {
            if ((*it).address == address) {
                (*it).batState = batState;
                device_found = true;
                break;
            }
        }

        if (!device_found) {
            // New vent, add it to the vents vector
            Serial.print("New vent: ");
            Serial.println(address);

            user_settings.num_of_vents += 1;
            vent new_vent = {address, batState};
            vents.push_back(new_vent);
            // Save the new vent to EEPROM
            new_vent.save_back(user_settings.num_of_vents);
            user_settings.save();
        }
    }
    else if ((address == 02) || (address == 03) || (address == 04)) {
        // Message was from a router
        boolean device_found = false;

        for (auto it = zones.begin(); it != zones.end(); ++it) {
            if ((*it).address == address) {
                (*it).temp = temp_val;
                (*it).batState = batState;
                device_found = true;
                temp_commander.update_zone((*it));
                break;
            }
        }

        if (!device_found) {
            // New router, add it to the zones vector
            Serial.print("New zone: ");
            Serial.println(address);
            user_settings.num_of_zones += 1;
            zone new_zone = {ATMEGA, temp_val, 0, 0, 1, "New Zone", address, 0, batState};
            new_zone.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode, selected_zone);
            new_zone.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode, selected_zone);
            zones.push_back(new_zone);
            // Add it to temp_commander
            temp_commander.add_zone(new_zone);
            // Save the new zone to EEPROM
            new_zone.save(user_settings.num_of_zones);
            user_settings.save();
        }
    }


}


// void update_cloud() {
//
//     cloud_data = "[";
//     for (auto it = zones.begin(); it != zones.end(); ++it) {
//
//         String temp = (*it).zone_name;
//
//         cloud_data += "{";
//         cloud_data += "\"name\":" + String::format("\"%s\",", temp);
//         cloud_data += "\"tmp\":" + String::format("%d,", (*it).conv_adc_to_temp((*it).get_calibrated_temp_adc(), user_settings.temp_mode));
//         cloud_data += "\"mintmp\":" + String::format("%d,", (*it).conv_adc_to_temp((*it).min_temp, user_settings.temp_mode));
//         cloud_data += "\"maxtmp\":" + String::format("%d", (*it).conv_adc_to_temp((*it).max_temp, user_settings.temp_mode));
//         cloud_data += "},";
//     }
//     cloud_data = cloud_data.substring(0, cloud_data.length()-1);
//     cloud_data += "]";
//
//     Serial.print("Cloud data: ");
//     Serial.println(cloud_data);
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// }















int updateTemperatureRange (String command)
{

    Serial.print("Cloud Update Zone");

    // Example command string: "temp_min:123,temp_max:321"
    int temp_min = command.substring(command.indexOf("temp_min:") + 9, command.indexOf(",temp_max:")).toInt();
    int temp_max = command.substring(command.indexOf("temp_max:") + 9, command.indexOf(",zone:")).toInt();
    int zone_num = command.substring(command.indexOf("zone:") + 5).toInt();
    Serial.println(temp_min);
    Serial.println(temp_max);
    Serial.println(zone_num);

    auto it = zones.begin();

    for (int i = 1; i < zone_num; i++) {

        it++;

    }


    (*it).set_desired_min_temp(temp_min, user_settings.temp_mode, zone_num);
    (*it).set_desired_max_temp(temp_max, user_settings.temp_mode, zone_num);


}





















void setup()
{

    Serial.begin(115200);
    Serial1.begin(115200);
    // EEPROM.write(10, 0); // Calibrate screen
    delay(2000);

    GD.begin();
    screen_SPI_active = true;
    // GD.cmd_calibrate();
    GD.cmd_setrotate(0);



    EEPROM.get(EEPROM_START, user_settings);

    // Force reset:
    // EEPROM.write(EEPROM_START, 0x00);

    if (user_settings.version_number != VERSION_NUMBER) {
        // Clear EEPROM and load default settings
        Serial.println("New version detected. Resetting to factory defaults.");
        for (int i = EEPROM_START; i < EEPROM.length(); i++) {
            EEPROM.write(i, 0x00);
        }
        user_settings.version_number = VERSION_NUMBER;
        user_settings.temp_mode = TEMP_MODE_FAHR;
        user_settings.num_of_zones = 1;
        user_settings.num_of_vents = 0;
        user_settings.time_zone = -4.0;
        zone default_main_hub = {PHOTON, 905, 0, 0, 1, "Main Thermostat", 01, 0, true};
        default_main_hub.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode, selected_zone);
        default_main_hub.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode, selected_zone);
        zones.push_back(default_main_hub);
        user_settings.save();
        default_main_hub.save(selected_zone);
    }
    else {
        // Load the stored user settings
        EEPROM.get(EEPROM_START, user_settings);
        // Load the stored zones/vents
        for (int i = 0; i < user_settings.num_of_zones; i++) {
            zone temp;
            EEPROM.get(EEPROM_ZONES_START + i*sizeof(zone), temp);
            zones.push_back(temp);
        }
        for (int i = 0; i < user_settings.num_of_vents; i++) {
            vent temp;
            EEPROM.get(EEPROM_VENTS_START + i*sizeof(vent), temp);
            vents.push_back(temp);
        }
    }

    // Serial.println("EERPOM:");
    // for (int i = EEPROM_START; i < EEPROM.length(); i++) {
    //     Serial.printf("%d: %d", i, EEPROM.read(i));
    //     Serial.println();
    // }

    // Configure temperature reading pin
    pinMode(A0, INPUT);
    pinMode(D0, OUTPUT);
    main_hub_temp_timer.setToZero();
    cloud_update_timer.reset();
    zones.at(0).temp = analogRead(A0);

    Time.zone(user_settings.time_zone);
    networks_found = WiFi.scan(aps, 5);
    if (user_settings.temp_mode == TEMP_MODE_FAHR) {
        calibration_helper = 72;
    }
    else {
        calibration_helper = 22;
    }




    // WiFi.clearCredentials();
    // WiFi.setCredentials("Eric's Phone", "erociscool", WLAN_SEC_WPA2);


    // Fake data
    // TODO: temp needs to be the first zone every time (main hub with address 01)
    // remove the rest of the fake data
    // zone temp1 = {ATMEGA, 905, 0, 0, 1, "Basement", 02, 0, true};
    // zone temp2 = {ATMEGA, 915, 0, 0, 1, "Bedroom", 03, 0, true};
    // temp1.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode);
    // temp1.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode);
    // temp2.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode);
    // temp2.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode);

    // zones.push_back(temp1);
    // zones.push_back(temp2);

    // TODO: fix problem with duplicate addresses for multiple vents associated with main hub
    // vent vent1 = {011, true};
    // vent vent2 = {021, true};
    // vent vent3 = {012, false};
    // vent vent4 = {013, true};
    // vent vent5 = {023, false};
    //
    // vents.push_back(vent1);
    // vents.push_back(vent2);
    // vents.push_back(vent3);
    // vents.push_back(vent4);
    // vents.push_back(vent5);


    Particle.function("updateTemp", updateTemperatureRange);

    // for (auto i : zones) {
    //     Serial.println(i.temp);
    //     Serial.println(i.min_temp);
    //     Serial.println(i.max_temp);
    //     Serial.println(i.zone_name);
    //     Serial.println(i.address);
    // }




    // Initialize the temperature appropriately for the zones
    temp_commander.init(zones);

    delay(1000);



    cloud_data = "[";
    for (auto it = zones.begin(); it != zones.end(); ++it) {

        String temp = (*it).zone_name;

        cloud_data += "{";
        cloud_data += "\"name\":";
        cloud_data += "\"";
        cloud_data += temp;
        cloud_data += "\",";
        cloud_data += "\"tmp\":" + String::format("%d,", (*it).conv_adc_to_temp((*it).get_calibrated_temp_adc(), user_settings.temp_mode));
        cloud_data += "\"mintmp\":" + String::format("%d,", (*it).conv_adc_to_temp((*it).min_temp, user_settings.temp_mode));
        cloud_data += "\"maxtmp\":" + String::format("%d", (*it).conv_adc_to_temp((*it).max_temp, user_settings.temp_mode));
        cloud_data += "},";
    }
    cloud_data = cloud_data.substring(0, cloud_data.length()-1);
    cloud_data += "]";

    Serial.print("Cloud data: ");
    Serial.println(cloud_data);


    Particle.variable("cloud_data", cloud_data);


    Serial.println("SETUP COMPLETE.");


}

void loop()
{





    // Handle receiving a message from ATMEGA
    while (Serial1.available()) {
        if (serial_buffer_offset < (SERIAL_BUFFER_SIZE - 1)) {
            char c = Serial1.read();
            if (c != '\n') {
                //Add character to buffer
                serial_buffer[serial_buffer_offset++] = c;
                //Ensure the last character is always '\0'
                serial_buffer[serial_buffer_offset] = '\0';
            }
            else {
                //End of line character found
                handleSerialInput(zones, vents);
                serial_buffer_offset = 0;
                serial_buffer[serial_buffer_offset] = '\0';
            }
        }
    }


    if (millis() - time_since_lcd_check_in > 1000)
    {

        if (GD.rd(0xC0001) != 0x13 && screen_SPI_active)
        {
            screen_SPI_active = false;
            Serial.println("Screen SPI inactive");
        }

        if (!screen_SPI_active)
        {
            digitalWrite(D5, HIGH);

            SPI1.begin(D5);
            SPI1.setDataMode(SPI_MODE0);
            SPI1.setClockDivider(SPI_CLOCK_DIV64);
            SPI1.setBitOrder(MSBFIRST);

            digitalWrite(D5, LOW);

            SPI1.transfer(0x00);
            SPI1.transfer(0x00);
            SPI1.transfer(0x00);

            digitalWrite(D5, HIGH);
            delay(120);
            digitalWrite(D5, LOW);

            SPI.transfer(0x68);
            SPI.transfer(0x00);
            SPI.transfer(0x00);

            digitalWrite(D5, HIGH);
            delay(120);
            digitalWrite(D5, LOW);

            if (SPI.transfer(0x10) != 0xff)
            {
                digitalWrite(D5, HIGH);
                GD.begin();
                GD.cmd_setrotate(0);
                GD.VertexFormat(0);
                screen_SPI_active = true;

            }
            digitalWrite(D5, HIGH);
            delay(200);
        }

        time_since_lcd_check_in = millis();
    }



    // Get reference to the selected zone
    auto it = zones.begin();

    // Update temperature of main hun
    if (main_hub_temp_timer.check()) {
        (*it).temp = analogRead(A0);
        temp_commander.update_zone((*it));
        main_hub_temp_timer.reset();
    }

    // Send temperature commands to zones
    temp_commander.execute();
    if (temp_commander.heat_on) {
        digitalWrite(D0, HIGH);
    }
    else {
        digitalWrite(D0, LOW);
    }


    // Reference to selected zone object
    for (int i = 1; i < selected_zone; i++) {
        ++it;
    }







    if (screen_SPI_active) {

        // Handle touch inputs
        GD.get_inputs();
        tag = GD.inputs.tag;

        if (touch_drag == false && GD.inputs.x != -32768) {
            // Serial.println("drag started");
            x_start = GD.inputs.x;
            y_start = GD.inputs.y;
            touch_drag = true;
            x_drag = 0;
            y_drag = 0;

        }
        else if (touch_drag == true && GD.inputs.x != -32768) {
            x_drag = GD.inputs.x - x_start;
            y_drag = GD.inputs.y - y_start;
            // Serial.println(x_drag);
        }
        else if (touch_drag == true && GD.inputs.x == -32768) {
            touch_drag = false;

            // Serial.println("Drag Complete");


            // Handle drag
            if ((screen == MAIN) || ((screen == SETTINGS) && (settings_screen == CALIBRATION)) || ((screen == SETTINGS) && (settings_screen == NETWORK))) {

                // Prevent swiping right if on the first zone, or left if on the last zone, or in either direction if only one zone
                if (((selected_zone == 1) && (x_drag > 0)) || ((selected_zone == user_settings.num_of_zones) && (x_drag < 0)) || (user_settings.num_of_zones == 1)) {
                    x_drag = 0;
                }
                // Handle acceptable swipes on first and last zones
                else if ((selected_zone == 1) && (x_drag < -120) && (user_settings.num_of_zones > 1)) {
                    selected_zone++;
                }
                else if ((selected_zone == user_settings.num_of_zones) && (x_drag > 120) && (user_settings.num_of_zones > 1)) {
                    selected_zone--;
                }
                // Allow swiping left and right if in middle zones
                else if ((selected_zone > 1) && (selected_zone < user_settings.num_of_zones)) {
                    if (x_drag > 120) {
                        selected_zone--;
                    }
                    else if (x_drag < -120) {
                        selected_zone++;
                    }
                }

            }



            x_drag = 0;
            y_drag = 0;
        }
        else {
            x_drag = 0;
            y_drag = 0;
        }
    }









    // Handle screen touch
    if ((tag != previousTouch) && (screen_SPI_active)) {
        if (screen == MAIN) {
            if (previousTouch == 1) {
                // networks_found = WiFi.scan(aps, 5);
                screen = SETTINGS;
            }
            else if ((previousTouch == 2) || (previousTouch == 3)) {
                temp_selector = previousTouch;
                screen = TEMP_ADJUST;
            }
            else if (previousTouch == 4) {
                screen = EDIT_ZONE_NAME;
            }
        }
        else if (screen == TEMP_ADJUST) {
            // Convert all to ADC values to make comparison possible
            int one_degree = (*it).conv_temp_to_adc(1, user_settings.temp_mode) - (*it).conv_temp_to_adc(0, user_settings.temp_mode);
            int MAX_ALLOWABLE_TEMP = (*it).conv_fahr_to_adc(85);
            int MIN_ALLOWABLE_TEMP = (*it).conv_fahr_to_adc(55);
            int MAX_ALLOWABLE_TEMP_DIFFERENCE = 4*one_degree;

            if (previousTouch == 1) {
                screen = SETTINGS;
            }
            else if (previousTouch == 2) {
                screen = MAIN;
            }
            else if (previousTouch == 3) {
                // attempting to decrement upper limit
                if ((temp_selector == 2) && (((*it).max_temp - one_degree - (*it).min_temp) >= MAX_ALLOWABLE_TEMP_DIFFERENCE) && (((*it).max_temp - one_degree) <= MAX_ALLOWABLE_TEMP) && (((*it).max_temp - one_degree) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).set_desired_max_temp_adc((*it).max_temp - one_degree, selected_zone);
                }
                // attempting to decrement lower limit
                else if ((temp_selector == 3) && (((*it).min_temp - one_degree) <= MAX_ALLOWABLE_TEMP) && (((*it).min_temp - one_degree) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).set_desired_min_temp_adc((*it).min_temp - one_degree, selected_zone);
                }
            }
            else if (previousTouch == 4) {
                // attempting to increment upper limit
                if ((temp_selector == 2) && (((*it).max_temp + one_degree) <= MAX_ALLOWABLE_TEMP) && (((*it).max_temp + one_degree) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).set_desired_max_temp_adc((*it).max_temp + one_degree, selected_zone);
                }
                // attempting to increment lower limit
                else if ((temp_selector == 3) && (((*it).max_temp - ((*it).min_temp + one_degree)) >= MAX_ALLOWABLE_TEMP_DIFFERENCE) && (((*it).min_temp + one_degree) <= MAX_ALLOWABLE_TEMP) && (((*it).min_temp + one_degree) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).set_desired_min_temp_adc((*it).min_temp + one_degree, selected_zone);
                }
            }
        }
        else if (screen == EDIT_ZONE_NAME) {
            if (previousTouch == new_name_save.tag) {
                strncpy((*it).zone_name, zone_name_helper, sizeof(zone_name_helper));
                (*it).save(selected_zone);
                screen = MAIN;
            }
            else if (previousTouch == new_name_cancel.tag) {
                zone_name_helper[0] = '\0';
                screen = MAIN;
            }
            else if (previousTouch == gui_keyboard.keyboard_shift_lock.tag) {
                gui_keyboard.shift_press();
            }
            else if (previousTouch == gui_keyboard.keyboard_BS.tag) {
                if (strlen(zone_name_helper) > 0) {
                    zone_name_helper[strlen(zone_name_helper)-1] = '\0';
                }
            }
            else if (((previousTouch >= '0') && (previousTouch <= '9')) || ((previousTouch >= 'A') && (previousTouch <= 'Z')) || ((previousTouch >= 'a') && (previousTouch <= 'z')) || (previousTouch == gui_keyboard.keyboard_sh1.tag) || (previousTouch == gui_keyboard.keyboard_sh2.tag) || (previousTouch == gui_keyboard.keyboard_sh3.tag) || (previousTouch == gui_keyboard.keyboard_sh4.tag) || (previousTouch == gui_keyboard.keyboard_sh5.tag) || (previousTouch == gui_keyboard.keyboard_sh6.tag) || (previousTouch == gui_keyboard.keyboard_sh7.tag) || (previousTouch == gui_keyboard.keyboard_sh8.tag) || (previousTouch == gui_keyboard.keyboard_sh9.tag) || (previousTouch == gui_keyboard.keyboard_sh0.tag) || (previousTouch == gui_keyboard.keyboard_space.tag)) {
                // keyboard press
                if (strlen(zone_name_helper) < (sizeof(zone_name_helper) - 1)) {
                    char temp[2] = {char(previousTouch), '\0'};
                    strncat(zone_name_helper, temp, sizeof(zone_name_helper));
                }
            }
        }
        else if (screen == SETTINGS) {

            if (previousTouch == 1) {
                screen = MAIN;
            }
            else if (previousTouch == 2) {
                settings_screen = WIFI;
            }
            else if (previousTouch == wifi_scan.tag) {
                networks_found = WiFi.scan(aps, 5);
            }
            else if ((previousTouch >= 4) && (previousTouch <= 3+networks_found)) {
                wifi_password[0] = '\0';
                wifi_network_selected = previousTouch;
            }
            else if (previousTouch == wifi_connect.tag) {
                WiFi.setCredentials(aps[wifi_network_selected - 4].ssid, wifi_password, aps[wifi_network_selected - 4].security);
                WiFi.disconnect();
                wifi_network_selected = 0;
            }
            else if (previousTouch == wifi_cancel.tag) {
                wifi_network_selected = 0;
            }
            else if (previousTouch == 11) {
                settings_screen = CALIBRATION;
            }
            else if (previousTouch == calibrate_btn.tag) {
                (*it).calibrate(calibration_helper, (*it).temp, user_settings.temp_mode, selected_zone);
            }
            else if (previousTouch == 13) {
                calibration_helper--;
            }
            else if (previousTouch == 14) {
                calibration_helper++;
            }
            else if (previousTouch == gui_keyboard.keyboard_shift_lock.tag) {
                gui_keyboard.shift_press();
            }
            else if (previousTouch == 17) {
                settings_screen = NETWORK;
            }
            else if (previousTouch == gui_keyboard.keyboard_BS.tag) {
                if (strlen(wifi_password) > 0) {
                    wifi_password[strlen(wifi_password)-1] = '\0';
                }
            }
            else if (((previousTouch >= '0') && (previousTouch <= '9')) || ((previousTouch >= 'A') && (previousTouch <= 'Z')) || ((previousTouch >= 'a') && (previousTouch <= 'z')) || (previousTouch == gui_keyboard.keyboard_sh1.tag) || (previousTouch == gui_keyboard.keyboard_sh2.tag) || (previousTouch == gui_keyboard.keyboard_sh3.tag) || (previousTouch == gui_keyboard.keyboard_sh4.tag) || (previousTouch == gui_keyboard.keyboard_sh5.tag) || (previousTouch == gui_keyboard.keyboard_sh6.tag) || (previousTouch == gui_keyboard.keyboard_sh7.tag) || (previousTouch == gui_keyboard.keyboard_sh8.tag) || (previousTouch == gui_keyboard.keyboard_sh9.tag) || (previousTouch == gui_keyboard.keyboard_sh0.tag) || (previousTouch == gui_keyboard.keyboard_space.tag)) {
                // keyboard press
                if (strlen(wifi_password) < (sizeof(wifi_password) - 1)) {
                    char temp[2] = {char(previousTouch), '\0'};
                    strncat(wifi_password, temp, sizeof(wifi_password));
                }
            }

        }
    }








    // Handle screen changes
    if ((screen != previousScreen) || (settings_screen != previous_settings_screen)) {
        if ((screen == SETTINGS) && (settings_screen == CALIBRATION)) {
            if (user_settings.temp_mode == TEMP_MODE_FAHR) {
                calibration_helper = 72;
            }
            else {
                calibration_helper = 22;
            }
        }
        else if (screen == EDIT_ZONE_NAME) {
            gui_keyboard.change_keyboard_position(SW/2-250,80+(SH-80)/2-10);
        }
        else if ((screen == SETTINGS) && (settings_screen == WIFI)) {
            gui_keyboard.change_keyboard_position(SW/4+10+40, 80+(SH-80)/2-10);
        }

    }

    previous_settings_screen = settings_screen;
    previousScreen = screen;
    previousTouch = tag;











    // Draw the screen
    if ((screen_SPI_active) && ((((tag != 0 && tag != 255) || touch_drag == true) && millis() - timeSinceLCDUpdate > 100) || (millis() - timeSinceLCDUpdate > 100))) {

        GD.VertexFormat(0); // Need this command here or else every pixel is rendered too small
        GD.Tag(255);

        if (screen == MAIN) {

            if (subpage == 0 && x_drag > 0) {
                x_drag = 0;
            }
            else if (subpage == user_settings.num_of_zones && x_drag < 0) {
                x_drag = 0;
            }

            GD.ClearColorRGB(0x000000);
            GD.Clear();

            // Draw settings button
            GD.Tag(1);
            for (int i = 0; i < 3; i++) {
                for (int j = 3; j > 0; j--) {
                    drawRect(SW-10-j*20-(j-1)*5, 10+i*25, 20, 20, 0xffffff);
                }
            }
            GD.Tag(255);

            GD.cmd_text(SW-180, 30, 30, OPT_CENTER, Time.format(Time.now(), "%l:%M %P"));
            GD.cmd_text(SW-180, 60, 27, OPT_CENTER, Time.format(Time.now(), "%a, %b. %e, %Y"));



            GD.Begin(POINTS);
            GD.ColorRGB(0xffffff);
            GD.PointSize(5*16);
            for (int i = 0; i < user_settings.num_of_zones; i++) {
                GD.Vertex2f(SW/2 + i*15 - 15*(user_settings.num_of_zones-1)/2, 80);
            }
            GD.ColorRGB(0x000000);
            GD.PointSize(3*16);
            for (int i = 0; i < user_settings.num_of_zones; i++) {
                if ((selected_zone - 1) == i) {
                    continue;
                }
                GD.Vertex2f(SW/2 + i*15 - 15*(user_settings.num_of_zones-1)/2, 80);
            }

            GD.ColorRGB(0xffffff);
            GD.cmd_romfont(1, 34);
            GD.Tag(4);
            GD.cmd_text(SW/2, 40, 30, OPT_CENTER, (*it).zone_name);
            GD.Tag(255);
            GD.cmd_number(SW/2, SH/2, 1, OPT_CENTER, (*it).get_calibrated_temp(user_settings.temp_mode));

            GD.Tag(2);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/2+100, SH/2+80, 27, OPT_CENTER, "Upper Limit");
            GD.ColorRGB(0xd72539);
            GD.cmd_number(SW/2+100, SH/2+80+40, 31, OPT_CENTER, (*it).conv_adc_to_temp((*it).max_temp, user_settings.temp_mode));

            GD.Tag(3);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/2-100, SH/2+80, 27, OPT_CENTER, "Lower Limit");
            GD.ColorRGB(0x3a7fe8);
            GD.cmd_number(SW/2-100, SH/2+80+40, 31, OPT_CENTER, (*it).conv_adc_to_temp((*it).min_temp, user_settings.temp_mode));







        }

        else if (screen == TEMP_ADJUST) {

            GD.ClearColorRGB(0x000000);
            GD.Clear();

            // Draw settings button
            GD.Tag(1);
            for (int i = 0; i < 3; i++) {
                for (int j = 3; j > 0; j--) {
                    drawRect(SW-10-j*20-(j-1)*5, 10+i*25, 20, 20, 0xffffff);
                }
            }
            GD.Tag(255);

            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW-180, 30, 30, OPT_CENTER, Time.format(Time.now(), "%l:%M %P"));
            GD.cmd_text(SW-180, 60, 27, OPT_CENTER, Time.format(Time.now(), "%a, %b. %e, %Y"));
            GD.cmd_romfont(1, 34);
            GD.cmd_text(SW/2, 40, 30, OPT_CENTER, (*it).zone_name);



            if (temp_selector == 2) {
                GD.cmd_text(SW/2, SH/2-80, 30, OPT_CENTER, "Upper Limit");
                GD.ColorRGB(0xd72539);
                GD.cmd_number(SW/2, SH/2, 1, OPT_CENTER, (*it).conv_adc_to_temp((*it).max_temp, user_settings.temp_mode));
            }
            else if (temp_selector == 3) {
                GD.cmd_text(SW/2, SH/2-80, 30, OPT_CENTER, "Lower Limit");
                GD.ColorRGB(0x3a7fe8);
                GD.cmd_number(SW/2, SH/2, 1, OPT_CENTER, (*it).conv_adc_to_temp((*it).min_temp, user_settings.temp_mode));
            }


            GD.Tag(2);
            GD.LineWidth(RADIUS*16);
            drawRect(SW/2-125,SH*3/4-20,250,40, 0xffffff);
            drawRect(SW/2-125+2,SH*3/4-20+2,250-4,40-4, 0x000000);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/2, SH*3/4, 29, OPT_CENTER, "Back");
            GD.LineWidth(1*16);


            GD.Tag(3);
            GD.Begin(POINTS);
            GD.PointSize(35*16);
            GD.ColorRGB(0xffffff);
            GD.Vertex2f(SW/2-200, SH/2);
            GD.PointSize(33*16);
            GD.ColorRGB(0x000000);
            GD.Vertex2f(SW/2-200, SH/2);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/2-200, SH/2, 31, OPT_CENTER, "-");

            GD.Tag(4);
            GD.Begin(POINTS);
            GD.PointSize(35*16);
            GD.ColorRGB(0xffffff);
            GD.Vertex2f(SW/2+200, SH/2);
            GD.PointSize(33*16);
            GD.ColorRGB(0x000000);
            GD.Vertex2f(SW/2+200, SH/2);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/2+200, SH/2, 31, OPT_CENTER, "+");

        }

        else if (screen == EDIT_ZONE_NAME) {

            GD.ClearColorRGB(0x000000);
            GD.Clear();
            GD.Tag(255);

            GD.cmd_text(SW/2, 40, 30, OPT_CENTER, "Edit Zone Name");

            // (80+(80+(SH-80)/2-10))/2

            GD.cmd_text(SW/4-40+10, (80+(80+(SH-80)/2-10))/2-20-40+20, 29, OPT_CENTER, "New Zone Name:");
            drawRoundedRect(SW/2-(SW-40-(SW/4+20+120))/2+40+30+10, (80+(80+(SH-80)/2-10))/2-20-40, SW-40-(SW/4+20+120), 40, 0xffffff);
            GD.ColorRGB(0x000000);
            GD.cmd_text(SW/2-(SW-40-(SW/4+20+120))/2+40+30+10 + (SW-40-(SW/4+20+120))/2, (80+(80+(SH-80)/2-10))/2-20-40 + 20, 29, OPT_CENTER, zone_name_helper);

            if (strlen(zone_name_helper) > 0) {
                new_name_save.draw(tag);
            }
            new_name_cancel.draw(tag);
            gui_keyboard.draw(tag);













        }

        else if (screen == SETTINGS) {

            GD.ClearColorRGB(0x000000);
            GD.Clear();

            GD.Tag(1);
            drawRect(SW-10-100, 0, SW, 80, 0x222222);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW-100/2, 80/2, 29, OPT_CENTER, "Back");

            GD.Tag(255);
            drawRect(0, 0, SW-10-100, 80, 0x222222);
            GD.ColorRGB(0x868686);
            GD.Begin(LINES);
            GD.Vertex2f(SW-10-100,0);
            GD.Vertex2f(SW-10-100,80);
            GD.Vertex2f(0,80);
            GD.Vertex2f(SW,80);
            GD.Vertex2f(SW/4,0);
            GD.Vertex2f(SW/4,SH);

            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/4/2, 80/2, 29, OPT_CENTER, "Settings");

            char settings_title[20];

            if (settings_screen == WIFI) {
                strncpy(settings_title, "Wi-Fi", sizeof(settings_title));
            }
            else if (settings_screen == CALIBRATION) {
                strncpy(settings_title, "Calibration", sizeof(settings_title));
            }
            else if (settings_screen == NETWORK) {
                strncpy(settings_title, "Network", sizeof(settings_title));
            }

            GD.cmd_text((SW-10-100 + SW/4)/2, 80/2, 29, OPT_CENTER, settings_title);

            int num_of_settings = 5;
            for (int i = 1; i <= num_of_settings; i++) {

                GD.ColorRGB(0x868686);
                GD.Begin(LINES);
                GD.Vertex2f(0,80+((SH-80)/num_of_settings)*i);
                GD.Vertex2f(SW/4,80+((SH-80)/num_of_settings)*i);

                if (i == 1) {
                    // Wi-Fi Settings
                    GD.Tag(2);
                    drawRect(0+2, 80+((SH-80)/num_of_settings)*(i-1)+2, SW/4-4, ((SH-80)/num_of_settings)-4, 0x000000);
                    GD.ColorRGB(0xffffff);
                    GD.cmd_text(SW/4/2, 80+((SH-80)/num_of_settings)*(i-0.5), 28, OPT_CENTER, "Wi-Fi");
                    GD.Tag(255);
                }

                else if (i == 2) {
                    GD.Tag(11);
                    drawRect(0+2, 80+((SH-80)/num_of_settings)*(i-1)+2, SW/4-4, ((SH-80)/num_of_settings)-4, 0x000000);
                    GD.ColorRGB(0xffffff);
                    GD.cmd_text(SW/4/2, 80+((SH-80)/num_of_settings)*(i-0.5), 28, OPT_CENTER, "Calibration");
                    GD.Tag(255);
                }

                else if (i == 3) {
                    GD.Tag(17);
                    drawRect(0+2, 80+((SH-80)/num_of_settings)*(i-1)+2, SW/4-4, ((SH-80)/num_of_settings)-4, 0x000000);
                    GD.ColorRGB(0xffffff);
                    GD.cmd_text(SW/4/2, 80+((SH-80)/num_of_settings)*(i-0.5), 28, OPT_CENTER, "Network");
                    GD.Tag(255);
                }

            }


            if (settings_screen == WIFI) {

                if (wifi_network_selected) {
                    // Enter username and password for chosen network
                    GD.ColorRGB(0xffffff);
                    char wifi_info[100] = "Chosen Network: ";
                    strncat(wifi_info, aps[wifi_network_selected - 4].ssid, sizeof(wifi_info));
                    GD.cmd_text((SW/4+SW)/2, 80+40, 29, OPT_CENTER, wifi_info);

                    GD.cmd_text(SW/4+20, 80+40+40, 29, OPT_CENTERY, "Password:");
                    drawRoundedRect(SW/4+20+120, 80+40+40-20, SW-40-(SW/4+20+120), 40, 0xffffff);
                    int password_spacing = 15;
                    int password_start_pos = (SW-40+(SW/4+20+120))/2 - password_spacing*(strlen(wifi_password)+1)/2 + password_spacing/2;
                    GD.Begin(POINTS);
                    GD.PointSize(6*16);
                    GD.ColorRGB(0x000000);

                    for (int i = 0; i < strlen(wifi_password); i++) {
                        GD.Vertex2f(password_start_pos + (i+0.5)*password_spacing, 80+40+40);
                    }
                    if (strlen(wifi_password) > 0) {
                        wifi_connect.draw(tag);
                    }
                    wifi_cancel.draw(tag);

                    gui_keyboard.draw(tag);

                }
                else {

                    GD.Tag(255);

                    // list networks
                    char txtBuffer[50];

                    GD.ColorRGB(0xffffff);
                    if (WiFi.ready()) {

                        char wifi_info[50] = "SSID: ";
                        IPAddress localIP = WiFi.localIP();

                        strncat(wifi_info, WiFi.SSID(), sizeof(wifi_info));
                        GD.cmd_text((SW/4+SW)/2, 80+40, 29, OPT_CENTER, wifi_info);

                        strncpy(wifi_info, "IP Address: ", sizeof(wifi_info));
                        sprintf(txtBuffer, "%d.%d.%d.%d", localIP[0], localIP[1],localIP[2],localIP[3]);
                        strncat(wifi_info, txtBuffer, sizeof(wifi_info));
                        GD.cmd_text((SW/4+SW)/2, 80+40+30, 29, OPT_CENTER, wifi_info);

                    }
                    else {
                        GD.cmd_text((SW/4+SW)/2, 80+40, 29, OPT_CENTER, "Not Connected to any Wi-Fi Network");
                    }


                    sprintf(txtBuffer, "Available Networks - %d:", networks_found);
                    wifi_scan.draw(tag);
                    GD.cmd_text((SW/4+SW)/2, 80+40+30+60, 29, OPT_CENTER, txtBuffer);

                    int y_offset = 35;
                    for (int i = 0; i < networks_found; i++) {

                        GD.Tag(4+i);
                        drawRect(SW/4+10, 80+40+30+60+60+i*y_offset-y_offset/2, SW-10-(SW/4+10), y_offset, 0x000000);
                        GD.ColorRGB(0xffffff);
                        WiFiAccessPoint & ap = aps[i];
                        GD.cmd_text(SW/4+40, 80+40+30+60+60+i*y_offset, 29, OPT_CENTERY, ap.ssid);

                        if(ap.security == WLAN_SEC_UNSEC) {
                            GD.cmd_text(SW-10-120, 80+40+30+60+60+i*y_offset, 29, OPT_CENTERY, "Unsecured");
                        }
                        else if (ap.security == WLAN_SEC_WEP) {
                            GD.cmd_text(SW-10-120, 80+40+30+60+60+i*y_offset, 29, OPT_CENTERY, "WEP");
                        }
                        else if (ap.security == WLAN_SEC_WPA) {
                            GD.cmd_text(SW-10-120, 80+40+30+60+60+i*y_offset, 29, OPT_CENTERY, "WPA");
                        }
                        else if (ap.security == WLAN_SEC_WPA2) {
                            GD.cmd_text(SW-10-120, 80+40+30+60+60+i*y_offset, 29, OPT_CENTERY, "WPA2");
                        }
                        else {
                            GD.cmd_text(SW-10-120, 80+40+30+60+60+i*y_offset, 29, OPT_CENTERY, "Other");
                        }

                        // drawWifiStrength(20, 415+80*i+y_offset, ap.rssi);
                        // button wifi_connect = {100+i,380, 375+80*i+y_offset, 80,30,27, B_BLUE, "Connect"};
                        // wifi_connect.draw(tag);
                        // GD.Tag(255);
                        // GD.ColorRGB(0xBDC3C7);
                        // gd_switch_shape(LINES);
                        // //GD.Begin(LINES);
                        // GD.Vertex2f(40, 430+80*i+y_offset);
                        // GD.Vertex2f(440, 430+80*i+y_offset);
                    }

                    GD.Tag(255);

                }
            }
            else if (settings_screen == CALIBRATION) {

                GD.Tag(255);

                if (subpage == 0 && x_drag > 0) {
                    x_drag = 0;
                }
                else if (subpage == user_settings.num_of_zones && x_drag < 0) {
                    x_drag = 0;
                }


                GD.Begin(POINTS);
                GD.ColorRGB(0xffffff);
                GD.PointSize(5*16);
                for (int i = 0; i < user_settings.num_of_zones; i++) {
                    GD.Vertex2f((SW/4 + SW)/2 + i*15 - 15*(user_settings.num_of_zones-1)/2, 80+80);
                }
                GD.ColorRGB(0x000000);
                GD.PointSize(3*16);
                for (int i = 0; i < user_settings.num_of_zones; i++) {
                    if ((selected_zone - 1) == i) {
                        continue;
                    }
                    GD.Vertex2f((SW/4 + SW)/2 + i*15 - 15*(user_settings.num_of_zones-1)/2, 80+80);
                }

                GD.ColorRGB(0xffffff);
                GD.cmd_text((SW/4 + SW)/2, 40+80, 30, OPT_CENTER, (*it).zone_name);

                GD.cmd_text((SW/4 + SW)/2, 40+80+80+40, 28, OPT_CENTER, "Current Temperature");
                GD.cmd_romfont(1, 34);
                GD.cmd_number((SW/4 + SW)/2, SH/2+40+40, 1, OPT_CENTER, calibration_helper);




                calibrate_btn.draw(tag);

                GD.Tag(13);
                GD.Begin(POINTS);
                GD.PointSize(35*16);
                GD.ColorRGB(0xffffff);
                GD.Vertex2f((SW/4 + SW)/2-200, SH/2+40+40);
                GD.PointSize(33*16);
                GD.ColorRGB(0x000000);
                GD.Vertex2f((SW/4 + SW)/2-200, SH/2+40+40);
                GD.ColorRGB(0xffffff);
                GD.cmd_text((SW/4 + SW)/2-200, SH/2+40+40, 31, OPT_CENTER, "-");

                GD.Tag(14);
                GD.Begin(POINTS);
                GD.PointSize(35*16);
                GD.ColorRGB(0xffffff);
                GD.Vertex2f((SW/4 + SW)/2+200, SH/2+40+40);
                GD.PointSize(33*16);
                GD.ColorRGB(0x000000);
                GD.Vertex2f((SW/4 + SW)/2+200, SH/2+40+40);
                GD.ColorRGB(0xffffff);
                GD.cmd_text((SW/4 + SW)/2+200, SH/2+40+40, 31, OPT_CENTER, "+");

                GD.Tag(255);

            }

            else if (settings_screen == NETWORK) {

                GD.Tag(255);

                if (subpage == 0 && x_drag > 0) {
                    x_drag = 0;
                }
                else if (subpage == user_settings.num_of_zones && x_drag < 0) {
                    x_drag = 0;
                }


                GD.Begin(POINTS);
                GD.ColorRGB(0xffffff);
                GD.PointSize(5*16);
                for (int i = 0; i < user_settings.num_of_zones; i++) {
                    GD.Vertex2f((SW/4 + SW)/2 + i*15 - 15*(user_settings.num_of_zones-1)/2, 80+80);
                }
                GD.ColorRGB(0x000000);
                GD.PointSize(3*16);
                for (int i = 0; i < user_settings.num_of_zones; i++) {
                    if ((selected_zone - 1) == i) {
                        continue;
                    }
                    GD.Vertex2f((SW/4 + SW)/2 + i*15 - 15*(user_settings.num_of_zones-1)/2, 80+80);
                }

                GD.ColorRGB(0xffffff);
                GD.cmd_text((SW/4 + SW)/2, 40+80, 30, OPT_CENTER, (*it).zone_name);

                int coord_offset;
                if ((*it).address == 01) {
                    // Coordinator, no need to draw router
                    coord_offset = 90;
                }
                else {
                    coord_offset = 0;
                    // Draw router battery status
                    GD.Begin(POINTS);
                    GD.PointSize(26*16);
                    GD.Vertex2f((SW/4 + SW)/2, 80+40+30+60+45);
                    GD.PointSize(22*16);
                    GD.ColorRGB(0x222222);
                    GD.Vertex2f((SW/4 + SW)/2, 80+40+30+60+45);
                    if ((*it).batState) {
                        drawBattery((SW/4 + SW)/2 - 18/2, 80+40+30+60-22, 18, 30, 0x00923f);
                    }
                    else {
                        drawBattery((SW/4 + SW)/2 - 18/2, 80+40+30+60-22, 18, 30, 0xeb3332);
                    }
                }






                std::vector<vent> vents_in_zone;
                int selected_zone_address = (*it).address;

                // Serial.print("Num of vents: ");
                // Serial.println(vents.size());

                for (auto i : vents) {

                    // Serial.print("Vent zone number: ");
                    // Serial.println(i.zone_number());
                    // Serial.print("Selected zone address: ");
                    // Serial.println(selected_zone_address);

                    if (i.zone_number() == selected_zone_address) {
                        // Vent belongs to this zone
                        vents_in_zone.push_back(i);
                    }
                }

                int num_of_vents_in_zone = vents_in_zone.size();
                // Serial.print("Number of vents in zone: ");
                // Serial.println(num_of_vents_in_zone);
                auto vents_it = vents_in_zone.begin();

                for (int i = 0; i < num_of_vents_in_zone; i++) {
                    drawVent((SW/4 + SW)/2 + i*120 - 120*(num_of_vents_in_zone-1)/2-50, SH/2+40+40-30+80-coord_offset, 100, 60, 0xffffff);
                    if ((*vents_it).batState) {
                        drawBattery((SW/4 + SW)/2 + i*120 - 120*(num_of_vents_in_zone-1)/2 - 18/2, SH/2+40+40-50-20+80-coord_offset, 18, 30, 0x00923f);
                    }
                    else {
                        drawBattery((SW/4 + SW)/2 + i*120 - 120*(num_of_vents_in_zone-1)/2 - 18/2, SH/2+40+40-50-20+80-coord_offset, 18, 30, 0xeb3332);
                    }
                    vents_it++;

                }







            }














        }













        GD.swap();
        timeSinceLCDUpdate = millis();

    }




    if (cloud_update_timer.check()) {
        // update_cloud();
        cloud_data = "[";
        for (auto it = zones.begin(); it != zones.end(); ++it) {

            String temp = (*it).zone_name;

            cloud_data += "{";
            cloud_data += "\"name\":";
            cloud_data += "\"";
            cloud_data += temp;
            cloud_data += "\",";
            cloud_data += "\"tmp\":" + String::format("%d,", (*it).conv_adc_to_temp((*it).get_calibrated_temp_adc(), user_settings.temp_mode));
            cloud_data += "\"mintmp\":" + String::format("%d,", (*it).conv_adc_to_temp((*it).min_temp, user_settings.temp_mode));
            cloud_data += "\"maxtmp\":" + String::format("%d", (*it).conv_adc_to_temp((*it).max_temp, user_settings.temp_mode));
            cloud_data += "},";
        }
        cloud_data = cloud_data.substring(0, cloud_data.length()-1);
        cloud_data += "]";

        Serial.print("Cloud data: ");
        Serial.println(cloud_data);
        cloud_update_timer.reset();
    }


}
