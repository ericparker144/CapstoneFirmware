#include "GD3/GD3.h"
#include "HMI.h"
#include "DATA.h"
#include <vector>


#define SW 800 // Screen Width
#define SH 480 // Screen Height
#define TEMPERATURE_READ_INTERVAL 10000
#define SERIAL_BUFFER_SIZE 100
#define DEFAULT_MIN_TEMP_FAHR 68
#define DEFAULT_MAX_TEMP_FAHR 74
#define DEFAULT_MIN_TEMP_CELS 20
#define DEFAULT_MAX_TEMP_CELS 24

char serial_buffer[SERIAL_BUFFER_SIZE] = {'\0'};
size_t serial_buffer_offset = 0;

custom_settings user_settings = {TEMP_MODE_FAHR, 0, 0, 0.0};
char previousTouch;
char tag;
unsigned long timeSinceLCDUpdate = 0;
int networks_found;
WiFiAccessPoint aps[5];
int wifi_network_selected = 0;
char wifi_password[25] = {'\0'};
int calibration_helper;

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

timer main_hub_temp_timer(TEMPERATURE_READ_INTERVAL);
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

    int address = serial_string.substring(serial_string.indexOf(param1) + param1.length(), serial_string.indexOf(param2) - 1).toInt();
    boolean batState = boolean(serial_string.substring(serial_string.indexOf(param2) + param2.length(), serial_string.indexOf(param3) - 1).toInt());
    int temp_val = serial_string.substring(serial_string.indexOf(param3) + param3.length(), serial_string.indexOf("}")).toInt();


    // Update information about the specific zone or vent, or add new zone/vent if not found
    if (temp_val == 0) {
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
            vent new_vent = {address, batState};
            vents.push_back(new_vent);
        }
    }
    else {
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
            zone new_zone = {ATMEGA, temp_val, 0, 0, 1, "New Zone", address, 0, batState};
            new_zone.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode);
            new_zone.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode);
            zones.push_back(new_zone);
            // Add it to temp_commander
            temp_commander.add_zone(new_zone);
        }
    }


}




void setup()
{

    Serial.begin(115200);
    Serial1.begin(115200);
    // EEPROM.write(10, 0); // Calibrate screen
    delay(2000);

    GD.begin();
    // GD.cmd_calibrate();
    GD.cmd_setrotate(0);


    // EEPROM.get(EEPROM_START, user_settings);
    user_settings.temp_mode = TEMP_MODE_FAHR;
    user_settings.num_of_zones = 1;
    user_settings.num_of_vents = 0;
    user_settings.time_zone = -4.0;
    // Configure temperature reading pin
    pinMode(A0, INPUT);
    pinMode(D0, OUTPUT);
    main_hub_temp_timer.setToZero();
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
    zone temp = {PHOTON, 905, 0, 0, 1, "Living Room", 01, 0, true};
    // zone temp1 = {ATMEGA, 905, 0, 0, 1, "Basement", 02, 0, true};
    // zone temp2 = {ATMEGA, 915, 0, 0, 1, "Bedroom", 03, 0, true};
    temp.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode);
    temp.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode);
    // temp1.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode);
    // temp1.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode);
    // temp2.set_desired_min_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MIN_TEMP_FAHR : DEFAULT_MIN_TEMP_CELS), user_settings.temp_mode);
    // temp2.set_desired_max_temp(((user_settings.temp_mode == TEMP_MODE_FAHR) ? DEFAULT_MAX_TEMP_FAHR : DEFAULT_MAX_TEMP_CELS), user_settings.temp_mode);

    zones.push_back(temp);
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

    temp_commander.init(zones);


    // for (auto i : zones) {
    //     Serial.println(i.temp);
    //     Serial.println(i.min_temp);
    //     Serial.println(i.max_temp);
    //     Serial.println(i.zone_name);
    //     Serial.println(i.address);
    // }





    Serial.println("SETUP COMPLETE.");


}

void loop()
{

    user_settings.num_of_zones = zones.size();


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



    // Get reference to the selected zone
    auto it = zones.begin();

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









    // Handle screen touch
    if (tag != previousTouch) {
        if (screen == MAIN) {
            if (previousTouch == 1) {
                networks_found = WiFi.scan(aps, 5);
                screen = SETTINGS;
            }
            else if ((previousTouch == 2) || (previousTouch == 3)) {
                temp_selector = previousTouch;
                screen = TEMP_ADJUST;
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
                    (*it).max_temp -= one_degree;
                }
                // attempting to decrement lower limit
                else if ((temp_selector == 3) && (((*it).min_temp - one_degree) <= MAX_ALLOWABLE_TEMP) && (((*it).min_temp - one_degree) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).min_temp -= one_degree;
                }
            }
            else if (previousTouch == 4) {
                // attempting to increment upper limit
                if ((temp_selector == 2) && (((*it).max_temp + one_degree) <= MAX_ALLOWABLE_TEMP) && (((*it).max_temp + one_degree) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).max_temp += one_degree;
                }
                // attempting to increment lower limit
                else if ((temp_selector == 3) && (((*it).max_temp - ((*it).min_temp + one_degree)) >= MAX_ALLOWABLE_TEMP_DIFFERENCE) && (((*it).min_temp + one_degree) <= MAX_ALLOWABLE_TEMP) && (((*it).min_temp + one_degree) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).min_temp += one_degree;
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
                (*it).calibrate(calibration_helper, (*it).temp, user_settings.temp_mode);
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
            else if (((previousTouch >= '0') && (previousTouch <= '9')) || ((previousTouch >= 'A') && (previousTouch <= 'Z')) || ((previousTouch >= 'a') && (previousTouch <= 'z')) || (previousTouch == gui_keyboard.keyboard_sh1.tag) || (previousTouch == gui_keyboard.keyboard_sh2.tag) || (previousTouch == gui_keyboard.keyboard_sh3.tag) || (previousTouch == gui_keyboard.keyboard_sh4.tag) || (previousTouch == gui_keyboard.keyboard_sh5.tag) || (previousTouch == gui_keyboard.keyboard_sh6.tag) || (previousTouch == gui_keyboard.keyboard_sh7.tag) || (previousTouch == gui_keyboard.keyboard_sh8.tag) || (previousTouch == gui_keyboard.keyboard_sh9.tag) || (previousTouch == gui_keyboard.keyboard_sh0.tag)) {
                // keyboard press
                char temp[2] = {char(previousTouch), '\0'};
                strncat(wifi_password, temp, sizeof(wifi_password));
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

    }

    previous_settings_screen = settings_screen;
    previousScreen = screen;
    previousTouch = tag;











    // Draw the screen
    if ((((tag != 0 && tag != 255) || touch_drag == true) && millis() - timeSinceLCDUpdate > 100) || (millis() - timeSinceLCDUpdate > 100)) {

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
            GD.cmd_text(SW/2, 40, 30, OPT_CENTER, (*it).zone_name);
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
            GD.cmd_text(SW/2, SH*3/4, 29, OPT_CENTER, "Done");
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

                for (auto i : vents) {
                    if (i.zone_number() == selected_zone_address) {
                        // Vent belongs to this zone
                        vents_in_zone.push_back(i);
                    }
                }

                int num_of_vents_in_zone = vents_in_zone.size();
                auto vents_it = vents_in_zone.begin();

                for (int i = 0; i < num_of_vents_in_zone; i++) {
                    drawVent((SW/4 + SW)/2 + i*120 - 120*(num_of_vents_in_zone-1)/2-50, SH/2+40+40-30+80-coord_offset, 100, 60, 0xffffff);
                    if ((*vents_it).batState) {
                        drawBattery((SW/4 + SW)/2 + i*120 - 120*(num_of_vents_in_zone-1)/2 - 18/2, SH/2+40+40-50-20+80-coord_offset, 18, 30, 0x00923f);
                    }
                    else {
                        drawBattery((SW/4 + SW)/2 + i*120 - 120*(num_of_vents_in_zone-1)/2 - 18/2, SH/2+40+40-50-20+80-coord_offset, 18, 30, 0xeb3332);
                    }


                }







            }














        }













        GD.swap();
        timeSinceLCDUpdate = millis();

    }


}
