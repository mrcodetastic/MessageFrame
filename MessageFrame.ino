 // INFORMATION...
// Remotely Updated MessageBox for family overseas.
// Message can be updated from remote. Message is updated by device polling an external website with basic PHP script.

/*---------------------------------- INCLUDES -------------------------------------*/
// Include librarys
#include <ESP8266WiFi.h>        // ESP8266 library for all WiFi functions
#include <DNSServer.h>          // ESP8266 library used in AP mode (config mode) to connect direct to the web page 
#include <ESP8266WebServer.h>   // ESP8266 library used in AP mode (config mode)
#include <WiFiManager.h>        // Manage auto connect to WiFi an fall back to AP   //https://github.com/tzapu/WiFiManager
#include <TimeLib.h>            // For Time Management //http://www.arduino.cc/playground/Code/Time - https://github.com/PaulStoffregen/Time - Known issue with ESP8266 -> http://forum.arduino.cc/index.php?topic=96891.30
#include <LiquidCrystal_I2C.h>

// For the current weather
#include <JsonStreamingParser.h>    // Custom JSON Streaming Parser implementation for Open Weather Maps - Current Weather
#include "OWMCurrentWeatherParser.h"

/*------------------------------- CONFIGURATION ----------------------------------*/

// Open Weather Maps Host / Key
const char *weather_host            = "api.openweathermap.org";
const char *weather_key             = "...........";
const char *weather_language        = "en";
const char *weather_city_id_away    = "2643743"; // London UK

// Local and Remote Timezone
const char *timezone_away     = "Europe/London";  // As per PHP
char *display_name_away       = "London";

const char *timezone_home     = "Australia/Sydney"; // As per PHP
char *display_name_home       = "Sydney";

// Time and Message Host - Custom PHP Script
const char *our_host        = "192.168.0.100";
const char *message_url     = "/test/device_1_message.txt"; // Change for each MessageFrame as required
const char *time_url        = "/test/tz_time.php";


// LCD Change Frequency
int display_change_frequency_ms = 10000; // Every 10 seconds change what is displayed


#define LCD_NUM_COLUMNS 16
#define LCD_NUM_ROWS    2
// Set the LCD address to 0x27 for a 16 chars and 2 line display
// IMPORTANT: Use and I2C_Scanner to check that the address is correct, or you'll only ever get white bars showing on the LCD.
// Other values seen for these devices include: 0x3F
//#define LCD_I2C_ADDRESS 0x27
#define LCD_I2C_ADDRESS 0x3F 
#define LCD_DEGREES_CHAR "\xDF" //https://forum.arduino.cc/index.php?topic=456437.0
/*------------------------------- GLOBAL VARIABLES ----------------------------------*/
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_NUM_COLUMNS, LCD_NUM_ROWS);

// For when we can't connect
WiFiManager       wifiManager;

// Home and Away Timezone Stuff
time_t   tz_gmt_away_offset = 0;
time_t   tz_gmt_home_offset = 0; 

// Define global variables and const
time_t        message_last_updated      = 0;
String        http_response;
String        current_message;                       // The received server text message
char          current_message_cstr[256] = {'\0'};
String        current_weather;

// LCD character buffer
char lcd_line1[LCD_NUM_COLUMNS + 4]; // the string to print on the LCD /// need to be > 16 chars to store the \0 so c knows the string is terminated
char lcd_line2[LCD_NUM_COLUMNS + 4];

// For loop event scheduling / duration calcuation
unsigned long current_millisecond   = 0;
unsigned long previous_millisecond_time       = 0;
unsigned long previous_millisecond_weather    = 0;
unsigned long previous_millisecond_message    = 0;
unsigned long previous_millisecond_backlight  = 0;

// Device behaviour
int   displayState    = 0;
int   lcd_line        = 1;
bool  backlight_on    = true;


/*---------------------------------- SETUP ----------------------------------*/
void setup()
{
  http_response.reserve(2048);
  current_weather.reserve(1024);
  current_message.reserve(1024);
  
  // Setup the Serial
  Serial.begin(115200);
  Serial.println("Starting Setup...");

  // Print loading message to LCD.
  //lcd.begin(D3, D4);
  lcd.begin();
  lcd.cursor();
  clearLCDbuffers();
  lcd.print(" Connecting...");
//  lcd.print(F(" \xDF""C. Humidity: "));
  lcd.blink();

  // Setup and start WiFi-Manager
  wifiManager.setCaptivePortalEnable(true);
  wifiManager.setAPCallback(configModeCallback);   // Call routine to show infos when in Config Mode
  wifiManager.autoConnect("MessageFrame");          // Name of the buil-in accesspoint
  // ^ Loop here until connection with Wifi established

  delay(1000);
  lcd.noCursor();
  lcd.noBlink();
  
}


/*---------------------------------- LOOP ----------------------------------*/
void loop()
{
  // Get the current time for scheduling stuff
  current_millisecond = millis(); //


  // TASK SCHEDULING: Time (every week) ## https://www.baldengineer.com/arduino-how-do-you-regetCoinMarketCapPricesset-millis.html
  if ( ((unsigned long)(current_millisecond - previous_millisecond_time) >= (7 * 24 * 60 * 60 * 1000)) || (previous_millisecond_time == 0)  ) // perform this every eight hours or so...
  {
    Serial.println("SCHEDULE: It's time to do a weather update...");
    getTimeFromServer(timezone_away, false);
    getTimeFromServer(timezone_home, true);    
    previous_millisecond_time = current_millisecond;
  }


  // TASK SCHEDULING: Weather (every 8 hours) ## https://www.baldengineer.com/arduino-how-do-you-regetCoinMarketCapPricesset-millis.html
  if ( ((unsigned long)(current_millisecond - previous_millisecond_weather) >= (8 * 60 * 60 * 1000)) || (previous_millisecond_weather == 0)  ) // perform this every eight hours or so...
  {
    Serial.println("SCHEDULE: It's time to do a weather update...");
    getWeatherCurrentData();
    previous_millisecond_weather = current_millisecond;
  }

  // TASK SCHEDULING: Message (once a day)
  if ( ((unsigned long)(current_millisecond - previous_millisecond_message) >= (23 * 60 * 60 * 1000)) || (previous_millisecond_message == 0)  ) // perform this every eight hours or so...
  {
    Serial.println("SCHEDULE: It's time to do a message check...");
    getServerTextMessage();
    previous_millisecond_message = current_millisecond;
  }


  // Turn off screen at night
  if ( ((unsigned long)(current_millisecond - previous_millisecond_backlight) >= (250000)) || (previous_millisecond_backlight == 0)  ) // perform this every couple of minutes
  {
    Serial.println("SCHEDULE: It's time to see if we should turn the backlight off..");

    int current_hour = hour((now() + tz_gmt_home_offset));
    Serial.println("Current home hour is: " + String(current_hour));

    if ( (current_hour > 22) && (current_hour < 7)  ) // Turn off blacklight between 10 PM and 7 AM
    {
      if (backlight_on == true)
      {
        backlight_on = false;
        lcd.noBacklight();
        Serial.println("Turning backlight off.");
      }
    }
    else if ( (backlight_on == false) ) // else we're outside of this and the backlight is on!
    {
      backlight_on = true;
      lcd.backlight();
      Serial.println("Turning backlight on.");

    }

    // Use the snapshot to set track time until next event
    previous_millisecond_backlight = current_millisecond;
  }


  Serial.println("Current displayState: " + String(displayState));

  clearLCDbuffers();
  switch (displayState)
  {
    case 0: // Show the IP address / configuration, this only happens once
      DisplayWiFiConnectionDetails();
      delay(5000);
      displayState++;
      break;

    case 1: // Show HOME location time
      Serial.println("Showing current HOME timezone time.");
      snprintf(lcd_line1, LCD_NUM_COLUMNS, "Time in %s", display_name_home);
      getFormattedTimeToCharBuffer(lcd_line2, (now() + tz_gmt_home_offset));
      printLCDbuffers();
      delay(display_change_frequency_ms);
      displayState++;
      break;

    case 2: // Show AWAY location time
      Serial.println("Showing current AWAY timezone time.");    
      snprintf(lcd_line1, LCD_NUM_COLUMNS, "Time in %s", display_name_away);
      getFormattedTimeToCharBuffer(lcd_line2, now() + tz_gmt_away_offset    );
      printLCDbuffers();
      delay(display_change_frequency_ms);
      displayState++;
      break;

    case 3: // Current Away Location Weather
      snprintf(lcd_line1, LCD_NUM_COLUMNS, "%s weather", display_name_away);
      //strcat(lcd_line1, display_name_away);  strcat(lcd_line1, " weather:");
      current_weather.toCharArray(lcd_line2, 16);
      //strcat(lcd_line2,  "\0xDF !");
      printLCDbuffers();
      delay(display_change_frequency_ms);
      displayState++;
      break;

    case 4: //  Message

      if (current_message.length() <= 4) // We don't have a valid message 
      {
        displayState++; // get outta here
      }
      

      // need to copy or it gets destroyed
      memset(current_message_cstr, '\0', sizeof(current_message_cstr));
      current_message.toCharArray(current_message_cstr, sizeof(current_message_cstr));

      Serial.println("Displaying message:");
      Serial.println(current_message);

      // http://www.cplusplus.com/reference/cstring/strtok/
      char * pch;
      printf ("Splitting string \"%s\" into lines:\n", current_message_cstr);
      pch = strtok (current_message_cstr, "\n"); // \r or \n are the delimiters
      while (pch != NULL) // loop through newlines
      {
        if ( lcd_line++ % 2 == 0 )
        {
          Serial.print("LCD Line 2: ");
          snprintf (lcd_line2, LCD_NUM_COLUMNS+1, "%s", pch);
          Serial.println(pch);

          Serial.print("LCD Line 2 (in line buffer): ");
          Serial.println(lcd_line2);
          Serial.println("Str len is: " + String (strlen(lcd_line2)));

          lcd.setCursor(0, 1);
          lcd.print(lcd_line2);

          delay(5000);
        }
        else
        {
          if ( lcd_line > 1)
          {
            lcd.clear();
          }
          Serial.print("LCD Line 1: ");
          lcd.setCursor(0, 0);
          snprintf (lcd_line1, LCD_NUM_COLUMNS+1, "%s", pch);
          Serial.println(pch);
          //printStringSeq(lcd_line1, 0);
          lcd.print(lcd_line1);
          Serial.print("LCD Line 1 (in line buffer): ");
          Serial.println(lcd_line1);
          Serial.println("Str len is: " + String (strlen(lcd_line1)));

          //printStringSeq(lcd_line1, 16, 0);
          delay(5000);
        }


        pch = strtok (NULL, "\r\n");
      }

      delay(display_change_frequency_ms);
      displayState++;

      break;



    // Default Case should catch things here.
    default:
      Serial.println("END of valid displayStates.");
      displayState = 1;
      lcd_line     = 1; // go back to zero for the next LCD line
      break;

  }

} // end loop




void getServerTextMessage() {

  current_message = ""; 

  // Use WiFiClient class to create TCP connections
  //String url              = "/iot/message/?action=get_message&device=" + String(device_id); // The URI
  int32_t eom_position    = -1; // End of Server Text  Token - 'EOM'
  int32_t som_position    = 0;   // Start of message position

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(our_host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // This will send the request to the server
  client.print(String("GET ") + String(message_url) + " HTTP/1.1\r\n" +
               "Host: " + String(our_host) + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println(String("GET ") + String(message_url) + " HTTP/1.1\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server
  while (client.available())
  {
    http_response  = client.readString(); // Get everything from the Server

  Serial.println("***************** MESSAGE RESPONSE *****************");
  Serial.println(http_response);
  Serial.println("***************************************************");    

    // Text Message
    current_message   = getResponseValue("<MESSAGE>", "</MESSAGE>", http_response);
    
    if ( current_message.length() <= 2)
    {
      Serial.println("There is no message to load from the server!");
    }
    else
    {
      Serial.println("Got server message: " + current_message);
    }


  } // end client available check

  client.stop();
}


void getWeatherCurrentData()
{
  WiFiClient client;

  client.setTimeout(5000);
  client.flush();

  // Based on code from: https://www.youtube.com/watch?v=PUOhUyLCIU8
  Serial.print("connecting to "); Serial.println(weather_host);

  // Get the current weather
  if (client.connect(weather_host, 80)) {
    client.println(String("GET /data/2.5/weather?id=") + String(weather_city_id_away) + "&units=metric&appid=" + weather_key + "&lang=" + weather_language + "\r\n" +
                   "Host: " + weather_host + "\r\nUser-Agent: ArduinoWiFi/1.1\r\n" +
                   "Connection: close\r\n\r\n");
  } else {
    Serial.println("connection failed");
    return;
  }

  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    Serial.println("w.");
    repeatCounter++;
  }

  // Bitcoin Price index Streaming Parser
  JsonStreamingParser jsp_current_weather;
  OWMCurrentWeatherParser CWP;

  // Setup the parser for Coinbase BPI
  CWP.setup();
  jsp_current_weather.setListener(&CWP); // add custom parser to JSON streaming service

  char c = 0;
  int counter = 0;
  while (client.available())
  {
    c = (char) client.read();

    // Note: Need to use the STREAMING JSON Parser for memory reasons
    jsp_current_weather.parse(c);
    delayMicroseconds(100); // need to do this to stop crashes for some reason
    counter++;
  }

  current_weather = CWP.getCurrentWeatherString();

  client.stop();  // stop listening

  if (counter == 0)
  {
    Serial.println("Recieved nothing for some reason.");
  }

} // get current weather data

boolean getTimeFromServer(const char *timezone, boolean is_home_tz)
{

  Serial.println("Getting time for timezone: " + String(timezone));
  Serial.print("Is home timezone: ");
  Serial.println(is_home_tz, BIN);

  // Get the time from our PHP script
  WiFiClient client;

  if (!client.connect(our_host, 80)) {
    Serial.println("connection failed");
    return false;
  }

  // http:// n/iot/ticker/?action=time&timezone=Europe/London - Get the London time.
  client.print(String("GET "    + String(time_url) + "?timezone=" + String(timezone) + " HTTP/1.1\r\n") +
               String("Host: "  + String(our_host) + "\r\n") +
               String("Connection: close\r\n\r\n"));

  Serial.print(String("GET "    + String(time_url) + "?timezone=" + String(timezone) + " HTTP/1.1\r\n") +
               String("Host: "  + String(our_host) + "\r\n") +
               String("Connection: close\r\n\r\n"));

  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10)
  {
    delay(500);
    repeatCounter++;
  }

  Serial.println("Parsing time server response...");

  while (client.connected() || client.available())
  {

    if ( !client.available() )
    {
      yield();  // wait here.
      continue;
    }

    http_response = client.readString(); // read everything
    http_response.toUpperCase();

  Serial.println("***************** TIME RESPONSE *****************");
  Serial.println(http_response);
  Serial.println("***************************************************");    

    // Get the GMT UNIX timestamp (Epoc basically)...
    String timestamp = getResponseValue(F("TIMESTAMP:"), "\n", http_response);
    Serial.println("The timestamp is: " + timestamp);

    // Get the offset for the requested timezone.
    String offset = getResponseValue(F("TIMESTAMP_OFFSET:"), "\n", http_response);
    Serial.println("The timestamp offset is: " + offset);

    if ( timestamp.equals(F("UNKNOWN")) )
    {
      Serial.println("Could not get time!!");
      return false;
    }

    time_t timestamp_int = timestamp.toInt();
    time_t offset_int    = offset.toInt();

    if (is_home_tz)
    {
      tz_gmt_home_offset = offset_int;
    }
    else
    {
      tz_gmt_away_offset = offset_int;
    }


    // Set the global time via TimeLib.h
    // Set it to GMT 0
    setTime(timestamp_int);

    // Set the offset
    //adjustTime(offset_int);
  } // end while (client.connected() || client.available())
 
  client.stop();
  
} // getTimeFromServer


// Get the value from a String line,
// i.e. A server string contains 'BLAH\nKEY:VALUE\nBLAH3';
// ... and we want VALUE from KEY:
String getResponseValue(String key, String terminator, String &line)
{
  Serial.println("Searching for START KEY: " + key);
  int start_pos = line.indexOf(key);
  if (start_pos == -1)
  {
    return "UNKNOWN";
  }

  Serial.println("Start of key is at: " + String(start_pos));
  
  Serial.println("Searching for TERMINATOR KEY:" + terminator);
  int end_pos   = line.indexOf(terminator, start_pos);
  Serial.println("End terminator is at: " + String(end_pos));


  String line2 = line.substring(start_pos + key.length(), end_pos);
  Serial.print("The substring is: "); Serial.println(line2);

  return line2;
}


/****************************** WIFI DISPLAY UTILS **********************************/

// What we run when we can't connect to an AP
void configModeCallback (WiFiManager *myWiFiManager)
{ // Show infos to log in with build in accesspoint. (Config Mode) When not possible to connect to WiFi

  lcd.setCursor(0, 0);
  lcd.print("No Internet!!");
  lcd.setCursor(0, 1);
  lcd.print("Please configure!");
  delay(5000);
  clearLCDbuffers();

  lcd.setCursor(0, 0);
  lcd.print("Connect to Wifi:");
  lcd.setCursor(0, 1);
  lcd.print("MessageFrame");
  delay(5000);
  clearLCDbuffers();

  lcd.setCursor(0, 0);
  lcd.print("Go to URL:");
  lcd.setCursor(0, 1);
  sprintf(lcd_line2, "%d.%d.%d.%d", WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
  lcd.print(lcd_line2);
}

void DisplayWiFiConnectionDetails()
{ // Connected screen

  clearLCDbuffers();
  strcpy (lcd_line1, "Device IP:");
  sprintf(lcd_line2, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  printLCDbuffers();
}


/****************************** LCD DISPLAY UTILS  **********************************/

void clearLCDbuffers()
{
  memset(lcd_line1, '\0', sizeof(lcd_line1));
  memset(lcd_line2, '\0', sizeof(lcd_line2));
  lcd.clear();
}

void printLCDbuffers()
{
  // but print these in the centre
  int len                    = strlen(lcd_line1);
  int start_cursor_position  = 0;
  Serial.println("The length of line 1 is: " + String(len));
  if (len <= (LCD_NUM_COLUMNS - 1))
  {
    start_cursor_position = (LCD_NUM_COLUMNS - len) / 2;
  }

  Serial.println("Start pos is: " + String(start_cursor_position));
  lcd.setCursor(start_cursor_position, 0);
  lcd.print(lcd_line1);

  // but print these in the centre
  len                    = strlen(lcd_line2);
  start_cursor_position  = 0;
  Serial.println("The length of line 2 is: " + String(len));
  if (len <= (LCD_NUM_COLUMNS - 1))
  {
    start_cursor_position = (LCD_NUM_COLUMNS - len) / 2;
  }

  Serial.println("Start pos is: " + String(start_cursor_position));
  lcd.setCursor(start_cursor_position, 1);
  lcd.print(lcd_line2);


}


/********************************** DATE/TIME HELPER UTILS  *************************************/
// NOTE: Ensure destination buffer at least 12 characters.
void getFormattedTimeToCharBuffer(char * parolaBuffer, time_t timestamp)
{
  /*
    unsigned long rawTime = now();
    int hours             = (rawTime % 86400L) / 3600;
    int minutes           = (rawTime % 3600)   / 60;
  */
  char am_or_pm[3];

  // Convert to 12 hour time
  if (isAM(timestamp) == false)
  {
    strcpy(am_or_pm, "PM");
  }
  else
  {
    strcpy(am_or_pm, "AM");
  }

  sprintf((char *)parolaBuffer, "%02d:%02d %s",  hourFormat12(timestamp), minute(timestamp), am_or_pm );

}


// NOTE: Ensure destination buffer at least 12 characters.
void getFormattedDateToCharBuffer(char * parolaBuffer)
{

  // Weird duplication issue when using the Time library character pointers!?
  // Answer: Because these functions return just a pointer to the same buffer, and sprintf
  // runs both then attempts to substitute. Unfortunately only the last operation is then in
  // the buffer, so we need to duplicate.
  // https://forum.arduino.cc/index.php?topic=367763.0

  char weekday_str[6] = { '\0' };
  strcpy(weekday_str, dayShortStr(weekday()));
  sprintf((char *)parolaBuffer, "%s %d %s",  weekday_str, day(),  monthShortStr(month()) );

}
