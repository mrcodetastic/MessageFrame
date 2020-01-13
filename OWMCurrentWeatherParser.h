#pragma once

#include "JsonListener.h"

// #define DEBUG_OUTPUT      // Debug output of our open weather map state machine
// #define DEBUG_OUTPUT_JSON // If we want to see the debug output for the JSON tree / base parser

class OWMCurrentWeatherParser: public JsonListener
{

    // Arduino Strings (memory + memory reservation in setup() to avoid fragmentation)	
    String    current_key;
	
	// Stored elements from current forecast
	bool      tmp_server_response_ok = false;
    String    tmp_weather_summary;
	String    tmp_weather_description;
	String    tmp_city_name;
    int       tmp_temperature;
    int       tmp_humidity;

    // Started
    bool      started = false;
	
	// Output
	String tmp_result_string = "Loading weather..";

	
    /******************* Parser Class Functions *******************/
  private:
    void process();
    
  public:

    String getCurrentWeatherString();  

    void setup();
   
    virtual void whitespace(char c);

    virtual void startDocument();

    virtual void key(String key);

    virtual void value(String value);

    virtual void endArray();

    virtual void endObject();

    virtual void endDocument();

    virtual void startArray();

    virtual void startObject();
	
};
