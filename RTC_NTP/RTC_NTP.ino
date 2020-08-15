#include <SPI.h>
#include <DS3231M.h> // Include the DS3231M RTC library
#include <Ethernet.h>
#include <NTPClient.h>

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF
};
IPAddress ip(192, 168, 9, 182);

EthernetServer server(80);

EthernetUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "europe.pool.ntp.org");
NTPClient timeClient(ntpUDP);
int didLastUpdateSucceed=1;

//const uint32_t SERIAL_SPEED        = 115200; ///< Set the baud rate for Serial I/O
const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O
const uint8_t  LED_PIN             =     13; ///< Arduino built-in LED pin number
const uint8_t  SPRINTF_BUFFER_SIZE =     32; ///< Buffer size for sprintf()

DS3231M_Class DS3231M;                          ///< Create an instance of the DS3231M Class
char          inputBuffer[SPRINTF_BUFFER_SIZE]; ///< Buffer for sprintf()/sscanf()

void setup() {
  // put your setup code here, to run once:
  // disable SD SPI
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  // Open serial communications and wait for port to open:
  Serial.begin(SERIAL_SPEED);
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
  delay(3000);
#endif
  Serial.print(F("\nStarting Set program\n"));
  Serial.print(F("- Compiled with c++ version "));
  Serial.print(F(__VERSION__));
  Serial.print(F("\n- On "));
  Serial.print(F(__DATE__));
  Serial.print(F(" at "));
  Serial.print(F(__TIME__));
  Serial.print(F("\n"));
  while (!DS3231M.begin()) // Initialize communications with the RTC
  {
    Serial.println(F("Unable to find DS3231MM. Checking again in 3s."));
    delay(3000);
  } // of loop until device is located
  Serial.println(F("DS3231M initialized."));

  Serial.println(F("Initialize Ethernet without DHCP:"));
  if (1 == 1)
    Ethernet.begin(mac, ip);
  delay(1000);
  // print your local IP address:
  server.begin();
  Serial.print(F("server is at "));
  Serial.println(Ethernet.localIP());

  // --- NTP
  timeClient.begin();
  delay(1000);
  timeClient.update();
  delay(1000);
  didLastUpdateSucceed=timeClient.update();
  if (!didLastUpdateSucceed) {
    Serial.print(F("*"));
    DateTime ref = DateTime(F(__DATE__), F(__TIME__));
    Serial.print(F("current_epoch_second "));
    Serial.println((String) ref.unixtime ());
    //DS3231M.adjust(DateTime((uint32_t) 1596064400+7200)); // Set to library compile Date/Time
    //DS3231M.adjust(); // Set to library compile Date/Time
    DS3231M.adjust(ref); // Set to library compile Date/Time
  } else {
    DS3231M.adjust(DateTime((uint32_t) timeClient.getEpochTime() + 7200)); // Set to library compile Date/Time
    Serial.print(F("Date/Time set to NTP time: "));
  };
  delay(1000);

  //DS3231M.adjust(); // Set to library compile Date/Time
  //DS3231M.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set to library compile Date/Time
  //DS3231M.adjust(DateTime((uint32_t) 1595760822+7200)); // Set to library compile Date/Time
  uint32_t current_epoch_second = timeClient.getEpochTime();
  // uint32_t current_epoch_second = timeClient.getEpochTime()+7200;
  Serial.print(F("current_epoch_second "));
  Serial.println((String) current_epoch_second);
  DateTime now = DS3231M.now(); // get the current time
  // Use sprintf() to pretty print the date/time with leading zeros
  sprintf(inputBuffer, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(),
          now.minute(), now.second());
  Serial.println(inputBuffer);
  Serial.print(F("DS3231M chip temperature is "));
  Serial.print(DS3231M.temperature() / 100.0, 1); // Value is in 100ths of a degree
  Serial.println("\xC2\xB0""C");
  Serial.println(F("Setting alarm to go off in 12 seconds."));
  DS3231M.setAlarm(secondsMinutesHoursDateMatch, now + TimeSpan(0, 0, 0, 12)); // Alarm goes off in 12 seconds
  Serial.println(F("Setting INT/SQW pin to toggle at 1Hz."));
  DS3231M.pinSquareWave(); // Make 1Hz signal on INT/SQW pin

}

void loop() {
  // put your main code here, to run repeatedly:
  static uint8_t secs;
  DateTime now = DS3231M.now(); // get the current time
  if (secs != now.second())     // Output if seconds have changed
  {
    sprintf(inputBuffer, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    Serial.println((String) inputBuffer + "." + (String) millis ()); // Display the current date/time
    secs = now.second();         // Set the counter variable
  } // of if the seconds have changed
  if (DS3231M.isAlarm()) // If the alarm bit is set
  {
    Serial.println("Alarm has gone off.");
    DS3231M.clearAlarm();
    // Alarm in 12 seconds. This will also reset the alarm state
    DS3231M.setAlarm(secondsMinutesHoursDateMatch, now + TimeSpan(0, 0, 0, 12));
  } // of if-then an alarm has triggered

}
