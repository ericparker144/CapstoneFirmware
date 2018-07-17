#include "GD3/GD3.h"
#include "HMI.h"
#include "DATA.h"
#include <vector>


#define SW 800 // Screen Width
#define SH 480 // Screen Height
#define MAX_ALLOWABLE_TEMP 85
#define MIN_ALLOWABLE_TEMP 55
#define MAX_ALLOWABLE_TEMP_DIFFERENCE 4


char previousTouch;
char tag;
unsigned long timeSinceLCDUpdate = 0;
int networks_found;
WiFiAccessPoint aps[5];
int wifi_network_selected = 0;
char wifi_password[25] = {'\0'};


// Variables used to implement dragging
boolean touch_drag = false;
int x_drag;
int y_drag;
int x_start = 0;
int y_start = 0;
int subpage = 0;
int temp_selector = 0;



pages screen = MAIN;
settings settings_screen = WIFI;
keyboard gui_keyboard(SW/4+10+40, 80+(SH-80)/2-10, 5, 45, 0x215968, 29);
button wifi_scan = {3, SW-10-70, 80+40+30+60-15, 70, 30, 28, 0x215968, "Scan"};
button wifi_connect = {9, (SW/4+SW)/2-5-100, 80+40+40+40-5, 100, 60, 28, 0x215968, "Connect"};
button wifi_cancel = {10, (SW/4+SW)/2+5, 80+40+40+40-5, 100, 60, 28, 0x215968, "Cancel"};

int num_of_zones;
int selected_zone = 1;
std::vector<zone> zones;





void setup()
{

    Serial.begin(9600);
    // EEPROM.write(10, 0); // Calibrate screen


    GD.begin();
    // GD.cmd_calibrate();
    GD.cmd_setrotate(0);

    delay(3000);


    // Fake data
    EEPROM.write(EEPROM_START, 3);
    Time.zone(-4);
    num_of_zones = EEPROM.read(EEPROM_START);
    networks_found = WiFi.scan(aps, 5);
    // WiFi.clearCredentials();
    // WiFi.setCredentials("Eric's Phone", "erociscool", WLAN_SEC_WPA2);


    zone temp = {70, 66, 74, 1, "Living Room", 1, 20};
    zone temp1 = {68, 64, 76, 1, "Basement", 2, 40};
    zone temp2 = {72, 68, 78, 1, "Bedroom", 3, 50};
    zones.push_back(temp);
    zones.push_back(temp1);
    zones.push_back(temp2);


    for (auto i : zones) {
        Serial.println(i.temp);
        Serial.println(i.min_temp);
        Serial.println(i.max_temp);
        Serial.println(i.zone_name);
        Serial.println(i.address);
    }





    Serial.println("SETUP COMPLETE.");


}

void loop()
{
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
        if (screen == MAIN) {

            // Prevent swiping right if on the first zone, or left if on the last zone, or in either direction if only one zone
            if (((selected_zone == 1) && (x_drag > 0)) || ((selected_zone == num_of_zones) && (x_drag < 0)) || (num_of_zones == 1)) {
                x_drag = 0;
            }
            // Handle acceptable swipes on first and last zones
            else if ((selected_zone == 1) && (x_drag < -120) && (num_of_zones > 1)) {
                selected_zone++;
            }
            else if ((selected_zone == num_of_zones) && (x_drag > 120) && (num_of_zones > 1)) {
                selected_zone--;
            }
            // Allow swiping left and right if in middle zones
            else if ((selected_zone > 1) && (selected_zone < num_of_zones)) {
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


    // Reference to selected zone object
    auto it = zones.begin();
    for (int i = 1; i < selected_zone; i++) {
        ++it;
    }


    if (tag != previousTouch) {
        if (screen == MAIN) {
            if (previousTouch == 1) {
                screen = SETTINGS;
            }
            else if ((previousTouch == 2) || (previousTouch == 3)) {
                temp_selector = previousTouch;
                screen = TEMP_ADJUST;
            }
        }
        else if (screen == TEMP_ADJUST) {
            if (previousTouch == 1) {
                screen = SETTINGS;
            }
            else if (previousTouch == 2) {
                screen = MAIN;
            }
            else if (previousTouch == 3) {
                // attempting to decrement upper limit
                if ((temp_selector == 2) && (((*it).max_temp - 1 - (*it).min_temp) >= MAX_ALLOWABLE_TEMP_DIFFERENCE) && (((*it).max_temp - 1) <= MAX_ALLOWABLE_TEMP) && (((*it).max_temp - 1) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).max_temp--;
                }
                // attempting to decrement lower limit
                else if ((temp_selector == 3) && (((*it).min_temp - 1) <= MAX_ALLOWABLE_TEMP) && (((*it).min_temp - 1) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).min_temp--;
                }
            }
            else if (previousTouch == 4) {
                // attempting to increment upper limit
                if ((temp_selector == 2) && (((*it).max_temp + 1) <= MAX_ALLOWABLE_TEMP) && (((*it).max_temp + 1) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).max_temp++;
                }
                // attempting to increment lower limit
                else if ((temp_selector == 3) && (((*it).max_temp - ((*it).min_temp + 1)) >= MAX_ALLOWABLE_TEMP_DIFFERENCE) && (((*it).min_temp + 1) <= MAX_ALLOWABLE_TEMP) && (((*it).min_temp + 1) >= MIN_ALLOWABLE_TEMP)) {
                    (*it).min_temp++;
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
            else if (previousTouch == gui_keyboard.keyboard_shift_lock.tag) {
                gui_keyboard.shift_press();
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

    previousTouch = tag;


    if ((((tag != 0 && tag != 255) || touch_drag == true) && millis() - timeSinceLCDUpdate > 100) || (millis() - timeSinceLCDUpdate > 100)) {

        GD.VertexFormat(0); // Need this command here or else every pixel is rendered too small
        GD.Tag(255);

        if (screen == MAIN) {

            if (subpage == 0 && x_drag > 0) {
                x_drag = 0;
            }
            else if (subpage == num_of_zones && x_drag < 0) {
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
            for (int i = 0; i < num_of_zones; i++) {
                GD.Vertex2f(SW/2 + i*15 - 15*(num_of_zones-1)/2, 80);
            }
            GD.ColorRGB(0x000000);
            GD.PointSize(3*16);
            for (int i = 0; i < num_of_zones; i++) {
                if ((selected_zone - 1) == i) {
                    continue;
                }
                GD.Vertex2f(SW/2 + i*15 - 15*(num_of_zones-1)/2, 80);
            }

            GD.ColorRGB(0xffffff);
            GD.cmd_romfont(1, 34);
            GD.cmd_text(SW/2, 40, 30, OPT_CENTER, (*it).zone_name);
            GD.cmd_number(SW/2, SH/2, 1, OPT_CENTER, (*it).temp);

            GD.Tag(2);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/2+100, SH/2+80, 27, OPT_CENTER, "Upper Limit");
            GD.ColorRGB(0xd72539);
            GD.cmd_number(SW/2+100, SH/2+80+40, 31, OPT_CENTER, (*it).max_temp);

            GD.Tag(3);
            GD.ColorRGB(0xffffff);
            GD.cmd_text(SW/2-100, SH/2+80, 27, OPT_CENTER, "Lower Limit");
            GD.ColorRGB(0x3a7fe8);
            GD.cmd_number(SW/2-100, SH/2+80+40, 31, OPT_CENTER, (*it).min_temp);







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
                GD.cmd_number(SW/2, SH/2, 1, OPT_CENTER, (*it).max_temp);
            }
            else if (temp_selector == 3) {
                GD.cmd_text(SW/2, SH/2-80, 30, OPT_CENTER, "Lower Limit");
                GD.ColorRGB(0x3a7fe8);
                GD.cmd_number(SW/2, SH/2, 1, OPT_CENTER, (*it).min_temp);
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

            GD.cmd_text((SW-10-100 + SW/4)/2, 80/2, 29, OPT_CENTER, settings_title);

            int num_of_settings = 10;
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










        }













        GD.swap();
        timeSinceLCDUpdate = millis();

    }


}
