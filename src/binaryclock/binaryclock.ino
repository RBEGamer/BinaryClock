//--- CHANGELOG ----


#define VERSION "2.0"
//#define USE_LITTLEFS
//#define ENABLE_RTC

#include <Arduino.h>
#include "FS.h"


#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif


#ifdef ESP32
#include <WebServer.h>

#define FORMAT_SPIFFS_IF_FAILED true

#ifdef USE_LITTLEFS
  #define SPIFFS LITTLEFS
  #include <LITTLEFS.h>
#else
#include <SPIFFS.h>
#endif
#include <ESPmDNS.h>
#endif



#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


#include <Wire.h>
#include "RTClib.h"

#if defined(ESP32)
#include "TZ.h"
#elif defined(ESP8266)
#include <TZ.h>
#else
#include "TZ.h"
#endif


#define MYTZ TZ_Europe_London



// CONFIG -------------------------------------
#define WEBSERVER_PORT 80 // set the port for the webserver eg 80 8080
#define MDNS_NAME "binaryclock" // set hostname
#define WEBSITE_TITLE "Binary Clock Configuration" // name your device
#define SERIAL_BAUD_RATE 9600
#define NTP_SEND_TIME_INTERVAL 1 //sende zeit an uhr alle x minuten
#define DEFAULT_NTP_SERVER "pool.ntp.org"
#define DEFAULT_TIMEZONE 1
#define DEFAULT_BEIGHTNESS 255
#define DEFAULT_DLS 0
#define DEFAULT_DIGIT_ORDER 0
#define DEFAULT_DARKMODE_DIMMING 50
#define DEFAULT_LED_OFFSET 0


// PIN CONFIG -----------------------------------

#ifdef WORDCLOCK_V0
  #define NEOPIXEL_PIN D1
#else

  #ifdef ESP8266
    #define NEOPIXEL_PIN D8
  #endif

  #ifdef ESP32
    #define NEOPIXEL_PIN 4
  #endif

#endif





// END CONFIG ---------------------------------


String time_last = "not synced";


#if defined(ESP8266)
const String BOARD_INFO= "CLOCK_FW_" + String(VERSION) + "_BOARD_" + "ESP8266";
#elif defined(ESP32)
const String BOARD_INFO= "CLOCK_FW_" + String(VERSION) + "_BOARD_" + "ESP32";
#endif


// DARKMODE
const int DARKMODE_START_HOURS = 0;
const int DARKMODE_STOP_HOURS = 6;

    


const int NUM_NEOPIXELS = 20;
const int ABI_COUNTER_MAX = 10;
const String ap_name = "BinaryClockConfiguration";


// END NEOPIXEL CONF ---------------------------
//FILES FOR STORiNG CONFIGURATION DATA
const char* file_ntp_server = "/file.txt";
const char* file_timezone = "/timezone.txt";
const char* file_syncmode = "/syncmode.txt";
const char* file_brightness = "/brightness.txt";
const char* file_dalight_saving_enabled = "/enabledls.txt";
const char* file_led_offset = "/ledoffset.txt";
const char* file_digit_order = "/digitorder.txt";
const char* file_darkmode_dimming = "/darkmode_dimming.txt";
//VARS
int sync_mode = 0;
int timezone = 0;
int brightness = 255;


long long last = 0;
long long last_abi = 0;

int rtc_hours = 0;
int rtc_mins = 0;
int rtc_secs = 0;
int rtc_day = 1;
int rtc_month = 1;
int rtc_year = 2020;
//0 FOR 4 DIGIT MODE 1 FOR TWO DIGIT MODE
int rtc_hours_tmp = 0;
bool abi_started = false;
int abi_counter = 0;

int dalight_saving_enabled = 0;
//IS SET TO TRUE IN SETUP IF A RTC CLOCK IS DETECTED
bool is_rtc_present = false;
//USEDE FOR SOFTWARE CLOCK
unsigned long timeNow = 0;
unsigned long timeLast = 0;
int digit_order = 0;
int darkmode_dimming = 50; // %
String ntp_server_url = "pool.ntp.org";
// INSTANCES --------------------------------------------

#ifdef ESP8266
ESP8266WebServer server(WEBSERVER_PORT);
#endif

#ifdef ESP32
WebServer server(WEBSERVER_PORT);
#endif

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,DEFAULT_NTP_SERVER,0,60000);
WiFiClient espClient;
Adafruit_NeoPixel pixels(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
RTC_DS1307 rtc;
WiFiManager wifiManager;






const String phead_1 = "<!DOCTYPE html><html><head><title>";
const String phead_2 = "</title>"
                       "<meta http-equiv='content-type' content='text/html; charset=utf-8'>"
                       "<meta charset='utf-8'>"
                       "<link "
                       "href='http://ajax.googleapis.com/ajax/libs/jqueryui/1.8.16/themes/base/"
                       "jquery-ui.css' rel=stylesheet />"
                       "<script "
                       "src='http://ajax.googleapis.com/ajax/libs/jquery/1.6.4/jquery.min.js'></"
                       "script>"
                       "<script "
                       "src='http://ajax.googleapis.com/ajax/libs/jqueryui/1.8.16/"
                       "jquery-ui.min.js'></script>"
                       "<style>"
                       "html, body {"
                       "  background: #F2F2F2;"
                       " width: 100%;"
                       " height: 100%;"
                       " margin: 0px;"
                       " padding: 0px;"
                       " font-family: 'Verdana';"
                       " font-size: 16px;"
                       " color: #404040;"
                       " }"
                       "img {"
                       " border: 0px;"
                       "}"
                       "span.title {"
                       " display: block;"
                       " color: #000000;"
                       " font-size: 30px;"
                       "}"
                       "span.subtitle {"
                       " display: block;"
                       " color: #000000;"
                       " font-size: 20px;"
                       "}"
                       ".sidebar {"
                       " background: #FFFFFF;"
                       " width: 250px;"
                       " min-height: 100%;"
                       " height: 100%;"
                       " height: auto;"
                       " position: fixed;"
                       " top: 0px;"
                       " left: 0px;"
                       " border-right: 1px solid #D8D8D8;"
                       "}"
                       ".logo {"
                       " padding: 25px;"
                       " text-align: center;"
                       " border-bottom: 1px solid #D8D8D8;"
                       "}"
                       ".menu {"
                       " padding: 25px 0px 25px 0px;"
                       " border-bottom: 1px solid #D8D8D8;"
                       "}"
                       ".menu a {"
                       " padding: 15px 25px 15px 25px;"
                       " display: block;"
                       " color: #000000;"
                       " text-decoration: none;"
                       " transition: all 0.25s;"
                       "}"
                       ".menu a:hover {"
                       " background: #0088CC;"
                       " color: #FFFFFF;"
                       "}"
                       ".right {"
                       " margin-left: 250px;"
                       " padding: 50px;"
                       "}"
                       ".content {"
                       " background: #FFFFFF;"
                       " padding: 25px;"
                       " border-radius: 5px;"
                       " border: 1px solid #D8D8D8;"
                       "}"
                       "</style>";

const String pstart = "</head>"
                      "<body style='font-size:62.5%;'>"
                      "<div class='sidebar'>"
                      "<div class='logo'>"
                      "<span class='title'>CLock</span>"
                      "<span class='subtitle'>- Configuration -</span>"
                      "</div>"
                      "<div class='menu'>"
                      "<a href='index.html'>Settings</a>"
                      "</div>"
                      "</div>"
                      "<div class='right'>"
                      "<div class='content'>";

const String pend = "</div>"
                    "</div>"
                    "</body>"
                    "</html>";

String last_error = "";

// ONLY READ THE FIRST LINE UNTIL NEW LINE !!!!!
String read_file(const char* _file, String _default = "")
{
    File f = SPIFFS.open(_file, "r");
    String tmp = _default;
    if (!f) {
        last_error = "open filesystem file_ntp_server failed";
    }
    else {
        tmp = f.readStringUntil('\n');
        last_error = "read from FS:" + String(_file) + " " + tmp;
    }
    return tmp;
}

void restore_eeprom_values()
{
    ntp_server_url = read_file(file_ntp_server,DEFAULT_NTP_SERVER);
    timezone = read_file(file_timezone, String(DEFAULT_TIMEZONE)).toInt();
    sync_mode = read_file(file_syncmode, "1").toInt();
    brightness = read_file(file_brightness, String(DEFAULT_BEIGHTNESS)).toInt();
   
    dalight_saving_enabled = read_file(file_dalight_saving_enabled, String(DEFAULT_DLS)).toInt();
    digit_order = read_file(file_digit_order, String(DEFAULT_DIGIT_ORDER)).toInt();
    darkmode_dimming = read_file(file_darkmode_dimming, String(DEFAULT_DARKMODE_DIMMING)).toInt();

}

bool write_file(const char* _file, String _content)
{
    File f = SPIFFS.open(_file, "w");
    if (!f) {
        last_error = "Oeffnen der Datei fehlgeschlagen";
        return -1;
    }
    f.print(_content);
    f.close();
    return 0;
}

void save_values_to_eeprom()
{
    write_file(file_ntp_server, ntp_server_url);
    write_file(file_syncmode, String(sync_mode));
    write_file(file_timezone, String(timezone));
    write_file(file_brightness, String(brightness));
    write_file(file_dalight_saving_enabled, String(dalight_saving_enabled));
    write_file(file_digit_order, String(digit_order));
    write_file(file_darkmode_dimming, String(darkmode_dimming));
 
}


void write_deffault_to_eeprom(){
          sync_mode = 1;
           timezone = DEFAULT_TIMEZONE;
           ntp_server_url = DEFAULT_NTP_SERVER;
           brightness = DEFAULT_BEIGHTNESS;
           dalight_saving_enabled = DEFAULT_DLS;
           digit_order = DEFAULT_DIGIT_ORDER;
           darkmode_dimming = DEFAULT_DARKMODE_DIMMING;

           
  save_values_to_eeprom();
}

uint32_t Wheel(int WheelPos, int _bright) {
  
   if(_bright > 255){
      _bright = 255;
    }
 
    if(_bright < 0){
    _bright = 0;
    }
    
 const float brgth_scale = _bright / 255.0;
    
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color((255 - WheelPos * 3) * brgth_scale, 0, (WheelPos * 3) * brgth_scale);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, (WheelPos * 3) * brgth_scale, (255 - WheelPos * 3) * brgth_scale);
  }
  WheelPos -= 170;
  return pixels.Color((WheelPos * 3) * brgth_scale, (255 - WheelPos * 3) * brgth_scale, 0);
}





//https://forum.arduino.cc/t/time-libary-sommerzeit-winterzeit/221884/2
boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7)))
    return true;
  else
    return false;
}

uint32_t get_esp_chip_id() {
#if defined(ESP8266)
  return ESP.getChipId();
#elif defined(ESP32)
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  return chipId;
#else
  return 0;
#endif

}







void update_rtc() {
  if(is_rtc_present){
    rtc.adjust(DateTime(rtc_year, rtc_month, rtc_day, rtc_hours, rtc_mins, rtc_secs));
  }
}

void update_rtc_via_ntp(){
  
  
  //UPDATE RTC IF USED
  rtc_hours = timeClient.getHours();;
  rtc_mins = timeClient.getMinutes();
  rtc_secs = 0;
 

  const unsigned long epochTime = timeClient.getEpochTime();
  const struct tm *ptm = gmtime ((time_t *)&epochTime); 
  rtc_day = ptm->tm_mday;
  rtc_month = ptm->tm_mon+1;
  rtc_year= ptm->tm_year+1900;

  
  update_rtc();

  }



//ANTI BURN IN CYLCE ANIMATION
//LIKE REAL NIXIES
void start_abi(){
//LEGACY SUPPORT
              
   Serial.println("_abi_");
   delay(10);
   abi_counter = ABI_COUNTER_MAX;
   abi_started = true;
}


void loop_abi(){
  if(abi_started && abi_counter <= 0){
   abi_started = false;
   abi_counter = 0;
  }
  //UPDATE CLOCK DISPLAY
  abi_counter--;
  const int c = (abi_counter % 10)*11;
  update_clock_display(c,c,c,0,128,false);
}


uint32_t digit_color(int _val,int _index, bool _banked, int _base_color, int _bright){
  //EACH DIGIT PAIR SHOULS HAVE AN DIFFERENT COLOR SO 
    const int MAX_COLOR = 255;
    const int color_offset_per_digit = MAX_COLOR / 2*2;
   
   if(_banked && _val <= 0){
      return pixels.Color(0, 0, 0);
   }else{
      return Wheel((_base_color + color_offset_per_digit*_index) % MAX_COLOR, _bright);
   }
}




void update_clock_display(int h, int m, int s, int col, int _bright, bool _disable_leading_zero){
    
    //LEGACY SEND TO ARDUINO BASED CLOCK
    Serial.flush();
    Serial.println();
    last_error = "_st_" + String(h) + "_" + String(m) + "_"+ String(s) + "_" + String(col) +"_" + String(_bright) +"_";
    Serial.println(last_error);          
    delay(100);
  
   
  
    pixels.clear();
    
    
    
  const uint32_t col_oth = Wheel((col + (255/4)*0) % 255, _bright);
  const uint32_t col_m = Wheel((col + (255/4)*1) % 255, _bright);
  const uint32_t col_h =Wheel((col + (255/4)*2) % 255, _bright);


  const uint8_t bin_h_z = byte((h % 100) / 10);
  const uint8_t bin_h_o = byte(h % 10);
  const uint8_t bin_m_z = byte((m % 100) / 10);
  const uint8_t bin_m_o = byte(m % 10);
   
  pixels.setPixelColor(3, col_h * bitRead(bin_h_z,0)); 
  pixels.setPixelColor(2, col_h * bitRead(bin_h_z,1)); 
  pixels.setPixelColor(1, col_h * bitRead(bin_h_z,2)); 
  pixels.setPixelColor(0, col_h * bitRead(bin_h_z,3)); 

  pixels.setPixelColor(4, col_h * bitRead(bin_h_o,0)); 
  pixels.setPixelColor(5, col_h * bitRead(bin_h_o,1)); 
  pixels.setPixelColor(6, col_h * bitRead(bin_h_o,2)); 
  pixels.setPixelColor(7, col_h * bitRead(bin_h_o,3)); 


  pixels.setPixelColor(11, col_m * bitRead(bin_m_z,0)); 
  pixels.setPixelColor(10, col_m * bitRead(bin_m_z,1)); 
  pixels.setPixelColor(9,  col_m * bitRead(bin_m_z,2)); 
  pixels.setPixelColor(8,  col_m * bitRead(bin_m_z,3)); 

  pixels.setPixelColor(12, col_m * bitRead(bin_m_o,0)); 
  pixels.setPixelColor(13, col_m * bitRead(bin_m_o,1)); 
  pixels.setPixelColor(14, col_m * bitRead(bin_m_o,2)); 
  pixels.setPixelColor(15, col_m * bitRead(bin_m_o,3)); 

  pixels.show();   // Send the updated pixel colors to the hardware.
 
}





 
void handleSave()
{
    // PARSE ALL GET ARGUMENTS
    for (uint8_t i = 0; i < server.args(); i++) {
        // SNY MODE = enable ntc sync
        if (server.argName(i) == "sync_mode") {
            sync_mode = server.arg(i).toInt();
            if(sync_mode < 0 || sync_mode > 2){
              sync_mode = 0;
            }
            last_error = "set sync_mode to" + String(sync_mode);
        }
        // timezone +-12hours
        if (server.argName(i) == "timezone") {
            timezone = server.arg(i).toInt();
            if (timezone > 12 || timezone < -12) {
                timezone = 0;
            }
            last_error = "set timezone to" + String(timezone);
            timeClient.setTimeOffset(timezone*3600);
            timeClient.forceUpdate();
            update_rtc_via_ntp();
        }
        // ntp_server_url
        if (server.argName(i) == "ntp_server_url") {
            ntp_server_url = server.arg(i);
            last_error = "set ntp_server_url to" + ntp_server_url;
            if(ntp_server_url != ""){
              timeClient.setPoolServerName(ntp_server_url.c_str());
              timeClient.forceUpdate();
            }
            last = 0;
        }

      

      // mqtt_display_mode
        if (server.argName(i) == "brightness") {
            brightness = server.arg(i).toInt();
            last_error = "set brightness to" + String(brightness);
            last = 0;
        }


        if(server.argName(i) == "darkmode_dimming"){
           darkmode_dimming = server.arg(i).toInt();
           last_error = "set darkmode_dimming to" + String(darkmode_dimming);
           last = 0;
           }

       if(server.argName(i) == "digit_order"){
           digit_order = server.arg(i).toInt();
           last_error = "set digit_order to" + String(digit_order);
           last = 0;
           }
           
       

        // mqtt_broker_port
        if (server.argName(i) == "dalight_saving_enabled") {
            dalight_saving_enabled = server.arg(i).toInt();
            last_error = "set dalight_saving_enabled to" + dalight_saving_enabled;
            last = 0;
        }

        
        // formats the filesystem= resets all settings
        if (server.argName(i) == "fsformat") {
            if (SPIFFS.format()) {
                last_error = "Datei-System formatiert";
            }
            else {
                last_error = "Datei-System formatiert Error";
            }
            
        }
        //LOAD CURRENT SAVED DATA
        if (server.argName(i) == "eepromread") {
            restore_eeprom_values();
        }

        //LOAD FACOTRY RESET
        if (server.argName(i) == "factreset") {
           write_deffault_to_eeprom();
        }

         //ANTI BURNOUT CYCLE
        if (server.argName(i) == "abi") {
            start_abi();
        }

      if (server.argName(i) == "sendtime") {
            update_rtc_via_ntp();
            delay(100);
        }  
    }
    //SAVE THESE DATA
    timeClient.forceUpdate();
    save_values_to_eeprom();
  
    
    server.send(300, "text/html","<html><head><meta http-equiv='refresh' content='1; url=/' /></head><body>SAVE SETTINGS PLEASE WAIT</body></html>");
}






void handleRoot()
{



    String control_forms = "<hr><h2>DEVICE INFO</h2>";
    control_forms+="<h3>" + String(MDNS_NAME) + String(get_esp_chip_id()) + "<br><br>"+BOARD_INFO+"<br>RTC:"+ String(is_rtc_present)+"</h3><br>";



  
    String timezonesign = "";
    if(timezone > 0){
      timezonesign = "+";
    }
   
 
     control_forms+="<hr><h2>CURRENT TIME</h2><h1>" + String(rtc_hours) + ":"+ String(rtc_mins) + ":" + String(rtc_secs) +"</h1>"
                    "<hr><h2>LAST NTP TIME</h2><h1>" + time_last + " ("+timezonesign+" "+ String(timezone)+" Hours)</h1><br>"
                    "<h2>LAST NTP DATE</h2><h1>" + String(rtc_day) + "."+String(rtc_month)+"."+ String(rtc_year)+"</h1>";





    control_forms += "<hr><h2>CONTROLS</h2>";
    control_forms += "<br><h3> MODE CONTROL </h3><br>";


        control_forms += "<form name='btn_on' action='/save' method='GET' required ><select name='sync_mode' id='sync_mode'>";

                         if(sync_mode == 0){
                            control_forms += "<option value='0' selected>DISABLED</option>";
                         }else{
                            control_forms += "<option value='0'>DISABLED</option>";
                         }

                         if(sync_mode == 1){
                            control_forms += "<option value='1' selected>NTP-SYNC</option>";
                         }else{
                            control_forms += "<option value='1'>NTP-SYNC</option>";
                         }
                     
        control_forms += "</select><input type='submit' value='SAVE'/></form>";
    
    
                     


     control_forms += "<br><h3> CLOCK CONTROLS </h3>"
                      "<form name='btn_off' action='/save' method='GET'>"
                     "<input type='number' value='"+ String(timezone) + "' name='timezone' min='-12' max='12' required placeholder='1'/>"
                     "<input type='submit' value='SET TIMEZONE'/>"
                     "</form><br>"
                     "<form name='btn_off' action='/save' method='GET'>"
                     "<input type='text' value='"+ ntp_server_url + "' name='ntp_server_url' required placeholder='pool.ntp.org'/>"
                     "<input type='submit' value='SET NTP SERVER URL'/>"
                     "</form><br>";
                     "<form name='btn_offmq' action='/save' method='GET'>"
                     "<input type='number' min='0' max='1' value='"+ String(dalight_saving_enabled) + "' name='dalight_saving_enabled' required placeholder='0'/>"
                     "<input type='submit' value='ENABLE GERMAN DAYLIGHT SAVING'/>"
                     "</form>"
                     "<form name='btn_on' action='/save' method='GET' required >"
                     "<input type='hidden' value='sendtime' name='sendtime' />"
                     "<input type='submit' value='SEND NTP TIME TO CLOCK'/>"
                     "</form><br>";
                   


     control_forms += "<br><h3>OTHER SETTINGS </h3>"
                    "<form name='btn_off' action='/save' method='GET'>"
                     "<input type='number' value='"+ String(brightness) + "' name='brightness' min='10' max='255' required placeholder='255'/>"
                     "<input type='submit' value='SET BRIGHTNESS'/>"
                     "</form><br><br><br>"
                      "<form name='btn_off' action='/save' method='GET'>"
                     "<input type='number' value='"+ String(darkmode_dimming) + "' name='darkmode_dimming' min='0' max='90' required placeholder='50'/>"
                     "<input type='submit' value='SET DARKMODE DIMMING %'/>"
                     "</form><br><br><br>"


                     
                     "<form name='btn_on' action='/save' method='GET' required >"
                     "<input type='hidden' value='eepromread' name='eepromread' />"
                     "<input type='submit' value='READ STORED CONFIG'/>"
                     "</form><br>"
                     "<form name='btn_on' action='/save' method='GET' required >"
                     "<input type='hidden' value='fsformat' name='fsformat' />"
                     "<input type='submit' value='DELETE CONFIGURATION'/>"
                     "</form><br>"
                     "<form name='btn_on' action='/save' method='GET' required >"
                     "<input type='hidden' value='abi' name='abi' />"
                     "<input type='submit' value='START ANTI BURN IN CYCLE'/>"
                     "</form><br>"
                     "<form name='btn_on' action='/save' method='GET' required >"
                     "<input type='hidden' value='factreset' name='factreset' />"
                     "<input type='submit' value='FACTORY RESET'/>"
                     "</form><br>";



    control_forms += "<br><hr><h3>LAST SYSTEM MESSAGE</h3><br>" + last_error;

    server.send(200, "text/html", phead_1 + WEBSITE_TITLE + phead_2 + pstart + control_forms + pend);
}



void handleNotFound()
{
    server.send(404, "text/html","<html><head>header('location: /'); </head></html>");
}



String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}


void test_digits(){
  pixels.clear();
    for(int j = 0; j < 5; j++){
    for(int i = 0; i < NUM_NEOPIXELS; i++){
      pixels.clear();
      pixels.setPixelColor(i, Wheel(j*40, 255));
      pixels.show();
      delay(50);
    }
    }
    pixels.clear();
  }



void display_ip(){
  const String ip = IpAddress2String(WiFi.localIP());
      Serial.println("_ip_" + ip + "_");
      for(int i =0; i < strlen(ip.c_str()); i++ ) {
        
        if(ip[i] == '.'){
          update_clock_display(0,0,0,128,64,false); //DISPLAY WIFI ERROR
        }else{
          update_clock_display(ip[i]-'0',0,0,0,255,false); //DISPLAY WIFI ERROR
         // Serial.println("_st_" + String(ip[i]-'0') + "_0_");
        }
        delay(1000);
      }
  }

  
void setup(void)
{ 


    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("_setup_started_");
    // START THE FILESYSTEM
    int fsok = SPIFFS.begin();
      if(!fsok){
        last_error = "__err_spiffs_init_error_try_to_format__";
        Serial.println(last_error);
        SPIFFS.format();
        fsok = SPIFFS.begin();
        if(!fsok){
          last_error = "__err_spiffs_format failed__";
          Serial.println(last_error);
         }
          
      }

    
    // LOAD SETTINGS
    restore_eeprom_values();

    //SETUP NEOPIXELS
    pixels.begin();
    pixels.clear();

  //  test_digits();
    
    // START WFIFIMANAGER FOR CAPTIVE PORTAL
    WiFiManager wifiManager;
    wifiManager.setDebugOutput(true);
    wifiManager.setTimeout(60);
    wifiManager.setConfigPortalTimeout(180);
   // wifiManager.setAPClientCheck(true);
    wifiManager.setBreakAfterConfig(true);
   // wifiManager.setClass("invert"); 
    //TRY TO CONNECT
    if(wifiManager.autoConnect(( ap_name +"_" + String(get_esp_chip_id())).c_str())){
     display_ip();
    }else{
      update_clock_display(42, 42, 42, 192, 255,false); //DISPLAY WIFI ERROR
    }


  delay(2000);


  // MDNS.begin((MDNS_NAME + String(get_esp_chip_id())).c_str());
    //WEBSERVER ROUTES
    delay(1000);
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.on("/index.html", handleRoot);
    server.onNotFound(handleNotFound);
    server.begin();
    /*
    //START OTA LIB
    ArduinoOTA.setHostname((MDNS_NAME + String(get_esp_chip_id())).c_str());
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        }
        else { // U_SPIFFS
            type = "filesystem";
        }
        SPIFFS.end();
    });
    ArduinoOTA.onEnd([]() {});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
    ArduinoOTA.onError([](ota_error_t error) {});
    ArduinoOTA.onEnd([]() {
        //if (SPIFFS.begin(true)) {
       //     restore_eeprom_values(); // RESTORE FILE SETTINGS
       // }
       ESP.restart();
    });
    ArduinoOTA.begin();
*/

    //SETUP  NTP CLIENT
    timeClient.setTimeOffset(timezone);
    timeClient.setPoolServerName(ntp_server_url.c_str());
    timeClient.begin();
    timeClient.setTimeOffset(timezone*3600);
    timeClient.forceUpdate();

   
    
  
  

  if (sync_mode == 1) {
    update_rtc_via_ntp();
  }

        
  Serial.println("_setup_complete_");
}





void loop(void)
{

    //HANDLE SERVER
    server.handleClient();

    


    
    //UPDATE NTP NTP
    if ((millis() - last) > 1000 * 60 * NTP_SEND_TIME_INTERVAL) {
        last = millis();   
        if (sync_mode == 1) {
            //HANDLE NTP
            timeClient.update();
            time_last = timeClient.getFormattedTime(); 
            update_rtc_via_ntp();
        }
    }

      

    
     //UPDATE RTC IF THE CLOCK IS PRESENT
     //ELSE SIMPLY USE MILLIS TO KEEP TRACK OF TIME
    if (is_rtc_present && sync_mode == 1) {
      
      DateTime now = rtc.now();
      rtc_hours = now.hour();
      rtc_hours_tmp = now.hour();
      rtc_mins = now.minute();
      rtc_secs = now.second();
     
      rtc_day = now.second();
      rtc_month = now.month();
      rtc_year = now.year();
     
     
     //CHECK SUMMERTIME IF ENABLED
      if(dalight_saving_enabled && summertime_EU(rtc_year, rtc_month, rtc_day, rtc_hours,0)){
       rtc_hours_tmp = rtc_hours + 1;
       if(rtc_hours_tmp >= 24){
         rtc_hours_tmp = (rtc_hours_tmp - 24);
       }
      }
     
     
    }else{
    
      timeNow = millis()/1000;
      rtc_secs = timeNow - timeLast;
      
      if (rtc_secs >= 60) {
        rtc_secs = 0;
        timeLast = timeNow;
        rtc_mins++;
      }
      
      if (rtc_mins >= 60){
        rtc_mins = 0;
        rtc_hours++;
      }
      if (rtc_hours >= 24){
        rtc_mins = 0;
        rtc_hours = 0;
      }
      rtc_hours_tmp = rtc_hours;
    }
     

    // SET DISPLAY CLOCK
    if (sync_mode == 1 && !abi_started) {
      int tmp_brightness = brightness;
      //APPLY DARKMODE
      if(rtc_hours_tmp >= DARKMODE_START_HOURS && rtc_hours_tmp < DARKMODE_STOP_HOURS){
        tmp_brightness = (int)(tmp_brightness * (1.0 - ((darkmode_dimming % 100) / 100.0)));
      }
      if(tmp_brightness < 10){
        tmp_brightness = 10;
       }
      update_clock_display(rtc_hours_tmp, rtc_mins, rtc_secs, map(rtc_secs,0,60,0,255), tmp_brightness, false);
  
      //OFF
    }else if(sync_mode == 0){
      update_clock_display(0, 0, 0, 0, 10, true);
    }else{
      //HANDLE ABI CYLCE
      if ((millis() - last_abi) > 1000) {
        last_abi = millis();   
        loop_abi();
      }       
    }
 
   
 
    //HANDLE OTA
    ArduinoOTA.handle();
   
 
    delay(70);
}
