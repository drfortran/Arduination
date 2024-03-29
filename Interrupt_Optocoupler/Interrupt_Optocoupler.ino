#include <SPI.h>
#ifdef NO_ETHERNET
#include <SD.h>
#else
#include <EthernetUdp.h>
#endif
#ifndef NO_RTC
#include <DS3231M.h> // Include the DS3231M RTC library
#endif

#ifndef NO_RTC
DS3231M_Class DS3231M;                          ///< Create an instance of the DS3231M Class
#endif

// maximum length of file name including path
#define FILE_NAME_LEN  20

// pin used for Ethernet chip SPI chip select
#define PIN_ETH_SPI   10
// pin used for SD chip SPI chip select
#define PIN_SD_SPI     4

const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O
const uint8_t  SPRINTF_BUFFER_SIZE =  199; ///< Buffer size for snprintf ()
const char     format[] PROGMEM    = { "optoburner,device=arduino05 current_status=%d,stop_second=%lu,stop_milli=%lu,stop_micro=%lu,duration_second=%ld,duration_milli=%ld,duration_micro=%ld\n" };

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

const   unsigned int debounceTime = 20; // in ms
const   unsigned long Duration_Milli_Min = 1;

// ---- ETHERNET
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xAB, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip (192,168,9,184);

EthernetUDP udp;
// the IP address of your InfluxDB host
byte host[] = {192, 168, 9, 4}; // volumio.local
// the port that the UDP plugin is listening on; InfluxDB relies on 8089
int port = 8089;

void LogData (const char* logLine);

void ISR_button ()
{
  optoFlag = 1;
  nextToLastOpto = lastOpto;
  lastOpto = micros ();
}

void udp_shout (String s) {
  udp.beginPacket (host, port);
  udp.print (s);
  udp.endPacket ();
}

void setup () {
#ifdef NO_ETHERNET
  // deselect Ethernet chip on SPI bus
  pinMode (PIN_ETH_SPI, OUTPUT);
  digitalWrite (PIN_ETH_SPI, HIGH);
#else
  // disable SD SPI
  pinMode (PIN_SD_SPI, OUTPUT);
  digitalWrite (PIN_SD_SPI, HIGH);
#endif
  pinMode (LED_BUILTIN, OUTPUT);

  // Open serial communications and wait for port to open:
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
  Serial.println (F ("- Built on " __DATE__ " at " __TIME__ " with"));
  const String str_compiler_version PROGMEM = F ("compiler_version:" __VERSION__);
  Serial.println(str_compiler_version);

#ifdef CPP_PATH
  const String str_compiler_path PROGMEM = F ("compiler_path:" CPP_PATH);
  Serial.println(str_compiler_path);
#endif

#ifdef VERSION
  const String str_version PROGMEM = F ("source_version:" VERSION);
  Serial.println(str_version);
#endif

#ifdef LASTDATE
  const String str_lastdate PROGMEM = F ("source_lastdate:" LASTDATE);
  Serial.println(str_lastdate);
#endif

#ifdef __TIMESTAMPISO__
  const String str___timestampiso__ PROGMEM = F ("build_timestamp:" __TIMESTAMPISO__);
  Serial.println(str___timestampiso__);
#endif

  pinMode (optoPin, INPUT);
  lastOptoState = digitalRead (optoPin);
  attachInterrupt (digitalPinToInterrupt (optoPin), ISR_button, CHANGE);

#ifdef NO_ETHERNET
  if (!SD.begin (PIN_SD_SPI)) {
    Serial.println (F ("SD card initialization failed! Things to check:"));
    Serial.println (F ("1. is a card inserted?"));
    Serial.println (F ("2. is your wiring correct?"));
    Serial.println (F ("3. did you change the chipSelect pin to match your shield or module?"));
    Serial.println (F ("Note: press reset or reopen this serial monitor after fixing your issue!"));
    return;  // SD card initialization failed
  }
#else
  Ethernet.init (PIN_ETH_SPI);  // Most Arduino shields
  // start the Ethernet
  Ethernet.begin (mac, ip);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus () == EthernetNoHardware) {
    Serial.println ("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay (1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus () == LinkOFF) {
    Serial.println ("Ethernet cable is not connected.");
  }

  // start UDP
  udp.begin (port);
  Serial.print (F ("Arduino connecte: "));
  Serial.print (F ("server is at "));
  Serial.println (Ethernet.localIP ());
#endif

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
  uint8_t pinStatus = digitalRead (optoPin);
  digitalWrite(LED_BUILTIN, pinStatus);   // turn the LED on (HIGH is the voltage level)
  //Serial.println (pinStatus);   // turn the LED on (HIGH is the voltage level)
  if (delta > debounceTime && (optoFlag == 1)) {
#ifndef NO_RTC
    DateTime RTC_now = DS3231M.now (); // get the current time
    unsigned long RTC_Time = RTC_now.unixtime ();
#endif
    optoFlag = 0;
    long unsigned int deltaPress0= lastPress - nextToLastPress;
    nextToLastPress = lastPress;
    lastPress = millis ();   //update lastPress
    long unsigned int deltaPress= lastPress - nextToLastPress;
    if (digitalRead (optoPin) == 0 && lastOptoState == 1) {  // if button is pressed and was released last change
#ifndef NO_RTC
      Start_Time  = RTC_Time;
#endif
      Start_Milli = Ard_Milli;
      Start_Micro = lastOpto;

      // Transition to ON state
      snprintf_P (outputBuffer, SPRINTF_BUFFER_SIZE, format,
                1, Start_Time, Start_Milli, Start_Micro,
                (Start_Time-Stop_Time), (Start_Milli-Stop_Milli), (Start_Micro-Stop_Micro));
      if (abs (Start_Milli-Stop_Milli) > Duration_Milli_Min)
      {
      Serial.print (outputBuffer);
#ifdef NO_ETHERNET
        LogData (outputBuffer);
#else
        udp_shout (String (outputBuffer));
#endif
      }

      lastOptoState = 0;    //record the lastButtonState
    }
    else if (digitalRead (optoPin) == 1 && lastOptoState == 0) {  // if button is not pressed, and was pressed last change
#ifndef NO_RTC
      Stop_Time   = RTC_Time;
#endif
      Stop_Milli  = Ard_Milli;
      Stop_Micro  = lastOpto;

      // Transition to OFF state
      snprintf_P (outputBuffer, SPRINTF_BUFFER_SIZE, format,
                0, Stop_Time, Stop_Milli, Stop_Micro,
                (Stop_Time-Start_Time), (Stop_Milli-Start_Milli), (Stop_Micro-Start_Micro));
      if (abs (Start_Milli-Stop_Milli) > Duration_Milli_Min)
      {
      Serial.println (outputBuffer);
#ifdef NO_ETHERNET
        LogData (outputBuffer);
#else
        udp_shout (outputBuffer);
#endif
      }

      lastOptoState = 1;    //record the lastButtonState
    }
    optoFlag = 0;
  }
}

void LogData (const char* logLine)
{
#ifdef NO_ETHERNET
  File logFile = SD.open ("log.txt", FILE_WRITE);
  if (logFile) {
    logFile.print (logLine);
    logFile.close ();
  }
#endif
}
