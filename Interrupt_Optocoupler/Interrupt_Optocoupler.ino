#ifndef NO_RTC
#include <DS3231M.h> // Include the DS3231M RTC library
#endif

#ifndef NO_RTC
DS3231M_Class DS3231M;                          ///< Create an instance of the DS3231M Class
#endif

const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O
const uint8_t  SPRINTF_BUFFER_SIZE =   85; ///< Buffer size for snprintf ()
const char     format[] PROGMEM    = { "%1d ; %10lu ; %10lu ; %10lu ; %10ld ; %10ld ; %10ld" };

// pin definitions
int optoPin = 2;

// global variables
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

void setup () {
  Serial.begin (SERIAL_SPEED);
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
  delay (3000);
#endif
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.print (F ("Gestion des Interruptions: @ "));
  Serial.print (micros ());
  Serial.println (F (" µs"));
  Serial.println (F ("Starting program"));
  Serial.println (F ("- Built with C++ version " __VERSION__));
  Serial.println (F ("- On " __DATE__ " at " __TIME__));

  // setup pin mode
  pinMode (optoPin, INPUT);
  attachInterrupt (digitalPinToInterrupt (optoPin), ISR_button, CHANGE);

#ifndef NO_RTC
  // Initialize communications with the RTC
  while (!DS3231M.begin ())
    {
      Serial.println (F ("Unable to find DS3231MM. Checking again in 3s."));
      delay (3000);
    } // of loop until device is located
  Serial.println (F ("DS3231M initialized."));
  Stop_Time   = Start_Time  = DS3231M.now ().unixtime ();
#endif
  Stop_Micro  = Start_Micro = micros ();
  Stop_Milli  = Start_Milli = millis ();
}

void loop () {
  char          outputBuffer[SPRINTF_BUFFER_SIZE]; ///< Buffer for snprintf ()
  unsigned long Ard_Milli = millis ();
  long unsigned int delta = Ard_Milli - lastPress;
  if (delta > debounceTime && optoFlag) {
#ifndef NO_RTC
    DateTime RTC_now = DS3231M.now (); // get the current time
    optoFlag = 0;
    unsigned long RTC_Time = RTC_now.unixtime ();
#endif
    optoFlag = 0;
    long unsigned int deltaPress0= lastPress - nextToLastPress;
    nextToLastPress = lastPress;
    lastPress = millis ();   //update lastPress
    long unsigned int deltaPress= lastPress - nextToLastPress;
    if (digitalRead (optoPin) == 0 && lastOptoState == 1) {  // if button is pressed and was released last change
      Start_Time  = RTC_Time;
      Start_Milli = Ard_Milli;
      Start_Micro = lastOpto;

      // Transition to ON state
      snprintf_P (outputBuffer, SPRINTF_BUFFER_SIZE, format,
                1, Start_Time, Start_Milli, Start_Micro,
                (Start_Time-Stop_Time), (Start_Milli-Stop_Milli), (Start_Micro-Stop_Micro));
      Serial.println (outputBuffer);

      lastOptoState = 0;    //record the lastButtonState
    }
    else if (digitalRead (optoPin) == 1 && lastOptoState == 0) {  // if button is not pressed, and was pressed last change
      Stop_Time   = RTC_Time;
      Stop_Milli  = Ard_Milli;
      Stop_Micro  = lastOpto;

      // Transition to OFF state
      snprintf_P (outputBuffer, SPRINTF_BUFFER_SIZE, format,
                0, Stop_Time, Stop_Milli, Stop_Micro,
                (Stop_Time-Start_Time), (Stop_Milli-Start_Milli), (Stop_Micro-Start_Micro));
      Serial.println (outputBuffer);

      lastOptoState = 1;    //record the lastButtonState
    }
    optoFlag = 0;
  }
}

void ISR_button ()
{
  optoFlag = 1;
  nextToLastOpto = lastOpto;
  lastOpto = micros ();
}
