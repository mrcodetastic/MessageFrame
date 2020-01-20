#include "OWMCurrentWeatherParser.h"
//#include <TimeLib.h>

// Refer to OpenWeatherMapParser.h for the class variables.
void OWMCurrentWeatherParser::setup()
{
  
  // Memory reservations 
  current_key.reserve(32);
  tmp_weather_summary.reserve(32);
  tmp_weather_description.reserve(32);
  tmp_city_name.reserve(32);
	
  tmp_result_string.reserve(256);
    
  // So we've started
  started = true;

}

void OWMCurrentWeatherParser::whitespace(char c) {

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("whitespace");
#endif

}

void OWMCurrentWeatherParser::startDocument() {

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("start document");
#endif

  /* Due to reading about Strings causing excessive fragmentation. Specifically:
        https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
        http://gammon.com.au/concat

     We will allocate memory to the specific JSON result variables, just in case, as to
     avoid excessive malloc activity when key:value pairs are different.

     We execute this function on first parse.
  */
  if (!started)
  {
    //setup();
    Sprintln("Setup has not been called for this listener instance!");
  }
}

void OWMCurrentWeatherParser::key(String key)
{

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("key: " + key);
#endif

  current_key = key;

}


void OWMCurrentWeatherParser::value(String value) 
{
	
  if ( current_key == "cod" ) // Server response
  {
	  if (value.equalsIgnoreCase("200"))
	  {
		  tmp_server_response_ok = true;
	  }
  }
  else if ( current_key == "name" ) 
  {
	  tmp_city_name = value;
  }
  else if ( current_key == "temp" )
  {
    // https://www.arduino.cc/reference/en/language/variables/data-types/string/functions/toint/
    tmp_temperature = value.toInt(); // convert string to int
  
  }
  else if ( current_key == "humidity" )
  {

    tmp_humidity = value.toInt(); // convert string to int

  }
  else if ( current_key == "description"  ) // there could be multiple of these, get the last one I guess.
  {
    tmp_weather_description = value;
    tmp_weather_description.trim(); // get rid of the whitespace
  }
  else if ( current_key == "main") // there could be multiple of these, get the last one I guess.
  {
    tmp_weather_summary = value;
    tmp_weather_summary.trim(); // get rid of the whitespace	  
	  
  }

  
#ifdef DEBUG_OUTPUT_JSON
  Sprintln("value: " + value);
#endif

}

void OWMCurrentWeatherParser::endArray() {

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("end array. ");
#endif

}

void OWMCurrentWeatherParser::endObject() {

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("end object. ");
#endif

}

void OWMCurrentWeatherParser::endDocument() {

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("end document. ");
#endif

process();

}

void OWMCurrentWeatherParser::startArray() {

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("start array. ");
#endif

}

void OWMCurrentWeatherParser::startObject() {

#ifdef DEBUG_OUTPUT_JSON
  Sprintln("start object. ");
#endif

}

void OWMCurrentWeatherParser::process()
{
  Sprintln("Processing current weather...  ");
  
  if (tmp_server_response_ok == false )
  {
    Sprintln("No weather to process.");
    //strcpy(char_str_buffer_pointer, "No Weather!");
	  return;
  }

  // otherwise all good.

  //https://arduino.stackexchange.com/questions/46828/how-to-show-the-%C2%BA-character-in-a-lcd
  //https://forum.arduino.cc/index.php?topic=456437.0
  tmp_result_string = tmp_weather_summary + " & " + tmp_temperature + "\xDF""C"; // need to append the °C elsewherev
  
  //memset(char_str_buffer_pointer, '\0', char_str_max_length); // empty  
  //tmp_result_string.toCharArray(char_str_buffer_pointer, char_str_max_length);
  //sprintf(curWeatherMessage, "Current Weather for %s \x10 %s, %d\xF7, %d%% humidity.", cityname,  weather0_main, main_temp, main_humidity);
  
  Sprint("Setting current weather to: ");
  Sprintln(tmp_result_string);
  
} // end process forecast


String OWMCurrentWeatherParser::getCurrentWeatherString()
{

  Sprint("Returning current weather: ");
  Sprintln(tmp_result_string);
    
  return tmp_result_string;
  
}
