/*
  Web Multi Thermo
  Version 1.1
    - suppression des messages snack bar
  Version 1.2
    - client NTP

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 */

#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <avr/wdt.h>
#ifndef NO_RTC
#include <DS3231M.h> // Include the DS3231M RTC library
#endif

const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O

// ---- ETHERNET

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,9,182);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

#ifndef NO_RTC
DS3231M_Class DS3231M;                          ///< Create an instance of the DS3231M Class
#endif

// ---- NTP
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// ---- ONE WIRE

/* Broche du bus 1-Wire */
#define ONE_WIRE_BUS 2

#define TEMPERATURE_PRECISION 12

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress devices[10];
int devicesFound = 0;

void setup() {
  // disable SD SPI
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  // Open serial communications and wait for port to open:
  Serial.begin(SERIAL_SPEED);
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
  delay(3000);
#endif
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  //Serial.print("Startup reason:");Serial.println(ESP.getResetReason());

  // start the Ethernet connection and the server:
  // Ethernet.begin(mac, ip);
  // start the Ethernet connection:
  // DHCP version
  Serial.println(F("Initialize Ethernet with DHCP:"));
  if (1 == 1)
      Ethernet.begin(mac, ip);
  else
    if (Ethernet.begin(mac) == 0) {
      Serial.println(F("Failed to configure Ethernet using DHCP"));
      Ethernet.begin(mac, ip);
      //if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      //  Serial.println(F("Ethernet shield was not found.  Sorry, can't run without hardware. :("));
      //}// else if (Ethernet.linkStatus() == LinkOFF) {
      //  Serial.println(F("Ethernet cable is not connected."));
      //}
      // no point in carrying on, so do nothing forevermore:
      //while (true) {
      //  delay(1);
      //}
    //}
    };
  delay(1000);
  // print your local IP address:
  server.begin();
  Serial.print(F("server is at "));
  Serial.println(Ethernet.localIP());

#ifndef NO_RTC
  // Initialize communications with the RTC
  while (!DS3231M.begin())
  {
    Serial.println(F("Unable to find DS3231MM. Checking again in 3s."));
    delay(3000);
  } // of loop until device is located
  Serial.println(F("DS3231M initialized."));
#endif

  // --- NTP
  timeClient.begin();

  // --- THERMO ---
  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print(F("Locating devices..."));
  Serial.print(F("Found "));
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(F(" devices."));

  devicesFound = sensors.getDeviceCount();

  // report parasite power requirements
  Serial.print(F("Parasite power is: "));
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println(F("OFF"));

  for (int i = 0; i < devicesFound; i++)
    if (!sensors.getAddress(devices[i], i)) {
      Serial.print(F("Unable to find address for Device"));
      Serial.println((String) i);
    }

  // show the addresses we found on the bus
  for (int i = 0; i < devicesFound; i++)
  {
    Serial.print(F("Device "));
    Serial.print((String)i);
    Serial.print(F(" Address: "));
    printAddress(devices[i]);
    Serial.println();
    Serial.println(stringPrintAddress(devices[i]));
    Serial.println();
  }

  for (int i = 0; i < devicesFound; i++)
    sensors.setResolution(devices[i], TEMPERATURE_PRECISION);

  delay(1000);
  timeClient.update();
  timeClient.setUpdateInterval(3600);
  delay(100);
#ifndef NO_RTC
    DS3231M.adjust(DateTime((uint32_t) timeClient.getEpochTime())); // Set to library compile Date/Time
    Serial.print(F("Date/Time set to NTP time: "));
#endif
  wdt_enable(WDTO_8S);
  Serial.println(F("Setup ended"));
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print a device address
String stringPrintAddress(DeviceAddress deviceAddress)
{
  String s("");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16)
      s += "0";
      s += String(deviceAddress[i], HEX);
  }
  return s;
}

// function to print the temperature for a device
String printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC < 10)
    return "0" + (String)tempC;
  else
    return (String)tempC;
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print(F("Resolution: "));
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}


// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  server.print(stringPrintAddress(deviceAddress));
  server.print(",");
  server.print(printTemperature(deviceAddress));
  server.print(",");
}


void loop() {
  wdt_reset();

  // listen for incoming clients
  EthernetClient client = server.available();


  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          client.println();
          client.println(F("<!DOCTYPE HTML>"));
          client.println(F("<html>"));
          // output the value of each analog input pin
          /*
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          */
          // ---- BEGIN Handle Thermo  -----------------------------------
          if (devicesFound == 0)
          {
             client.println(F("No devices found."));
             //return;
          }
          sensors.requestTemperatures();

          String sent = "";

          unsigned long NTP_Time = timeClient.getEpochTime();
#ifndef NO_RTC
          DateTime RTC_now = DS3231M.now(); // get the current time
          unsigned long RTC_Time = RTC_now.unixtime();
          Serial.print((String)RTC_Time);
          Serial.print(F(" ~ "));
          Serial.print((String)NTP_Time);
          Serial.println(F(" ; "));

          sent += (String)RTC_Time;
          sent += " ; ";
#endif

          // print the device information
          sent += (String)NTP_Time;
          sent += " ; ";
          sent += (String)millis();
          sent += " ; ";
          sent += timeClient.getFormattedTime();
          sent += " ; ";
          for (int i = 0; i < devicesFound; i++)
          {
            sent += stringPrintAddress(devices[i]);
            sent += " ; ";
            sent += printTemperature(devices[i]);
            if (i != devicesFound - 1)
              sent += " ; ";
          }

          client.println(sent);
          if (devicesFound == 0)
          {
             delay(1);
             // close the connection:
             client.stop();
             return;
          }

          // ---- END Handle Thermo -----------------------------------

          client.println(F("</html>"));
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println(F("client disconnected"));
  }
}
