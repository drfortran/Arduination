#ifndef NO_RTC
#include <DS3231M.h> // Include the DS3231M RTC library
#endif

#ifndef NO_RTC
DS3231M_Class DS3231M;                          ///< Create an instance of the DS3231M Class
#endif

const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O

// pin definitions
int optoPin = 2;

// global variables
int toggleState;
int lastOptoState = 1;
long unsigned int lastPress, nextToLastPress;
volatile long unsigned int lastOpto;
volatile long unsigned int nextToLastOpto;
volatile int optoFlag;
int     unsigned long Start_Time = 0;
int     unsigned long Stop_Time = 0;
int     unsigned long Start_Micro = 0;
int     unsigned long Stop_Micro = 0;
int     unsigned long Start_Milli = 0;
int     unsigned long Stop_Milli = 0;

int debounceTime = 40;

void setup() {
  Serial.begin(SERIAL_SPEED);
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
  delay(3000);
#endif
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println ("Gestion des Interruptions: @ " + (String) micros () + " µs");

  // setup pin mode
  pinMode (optoPin, INPUT);
  attachInterrupt (digitalPinToInterrupt (optoPin), ISR_button, CHANGE);

#ifndef NO_RTC
  // Initialize communications with the RTC
  while (!DS3231M.begin())
  {
    Serial.println(F("Unable to find DS3231MM. Checking again in 3s."));
    delay(3000);
  } // of loop until device is located
  Serial.println(F("DS3231M initialized."));
#endif
}

void loop() {
  long unsigned int delta = millis() - lastPress;
  long unsigned int current_micro, delta_micro;
  if(delta > debounceTime && optoFlag)
  {
#ifndef NO_RTC
    DateTime RTC_now = DS3231M.now(); // get the current time
    optoFlag = 0;
    unsigned long RTC_Time = RTC_now.unixtime();
#endif
    optoFlag = 0;
    long unsigned int deltaPress0= lastPress - nextToLastPress;
    nextToLastPress = lastPress;
    lastPress = millis();   //update lastPress
    long unsigned int deltaPress= lastPress - nextToLastPress;
    if(digitalRead (optoPin) == 0 && lastOptoState == 1)    //if button is pressed and was released last change
    {
      toggleState =! toggleState;                 //toggle the LED state
      current_micro = micros ();
      delta_micro = (current_micro - lastOpto);
      delta_micro = (lastOpto - nextToLastOpto);
      Start_Time = RTC_Time;
      Start_Milli = millis();
      Start_Micro = lastOpto;

      Serial.print (F("ON" ));
      Serial.print (F(" ; "));
      Serial.print (Start_Time);
      Serial.print (F(" ; "));
      Serial.print (Start_Milli);
      Serial.print (F(" ; "));
      Serial.print (Start_Micro);
      Serial.print (F(" ; "));
      Serial.print ((String) (Start_Time-Stop_Time));
      Serial.print (F(" ; "));
      Serial.print ((String) (Start_Milli-Stop_Milli));
      Serial.print (F(" ; "));
      Serial.print ((String) (Start_Micro-Stop_Micro));
      Serial.println ();

      lastOptoState = 0;    //record the lastButtonState
    }

    else if(digitalRead(optoPin) == 1 && lastOptoState == 0)    //if button is not pressed, and was pressed last change
    {
      current_micro = micros ();
      delta_micro = (current_micro - lastOpto);
      delta_micro = (lastOpto - nextToLastOpto);
      Stop_Time  = RTC_Time;
      Stop_Milli = millis();
      Stop_Micro = lastOpto;

      Serial.print (F("OFF"));
      Serial.print (F(" ; "));
      Serial.print (Stop_Time);
      Serial.print (F(" ; "));
      Serial.print (Stop_Milli);
      Serial.print (F(" ; "));
      Serial.print (Stop_Micro);
      Serial.print (F(" ; "));
      Serial.print ((String) (Stop_Time-Start_Time));
      Serial.print (F(" ; "));
      Serial.print ((String) (Stop_Milli-Start_Milli));
      Serial.print (F(" ; "));
      Serial.print ((String) (Stop_Micro-Start_Micro));
      Serial.println ();

      lastOptoState = 1;    //record the lastButtonState
    }
    optoFlag = 0;
  }
}

void ISR_button()
{
  optoFlag = 1;
  nextToLastOpto = lastOpto;
  lastOpto = micros ();
}
