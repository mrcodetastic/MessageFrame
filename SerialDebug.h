#ifndef SERIAL_DEBUG
#define SERIAL_DEBUG


  #ifdef DEBUG_MODE
  
      #define Sprintln(a) (Serial.println(a))
      #define SprintlnDEC(a, x) (Serial.println(a, x))
      
      #define Sprint(a) (Serial.print(a))
      #define SprintDEC(a, x) (Serial.print(a, x))
  
  #else
  
      #pragma message "COMPILING WITHOUT SERIAL DEBUGGING"
      #define Sprintln(a)
      #define SprintlnDEC(a, x)
      
      #define Sprint(a)
      #define SprintDEC(a, x)
      
  #endif



  






#endif
