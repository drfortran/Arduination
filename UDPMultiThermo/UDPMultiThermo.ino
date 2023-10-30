#include <SPI.h>
#include <EthernetUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>

// maximum length of file name including path
#define FILE_NAME_LEN  20

// pin used for Ethernet chip SPI chip select
#define PIN_ETH_SPI   10
// pin used for SD chip SPI chip select
#define PIN_SD_SPI     4
// pin used for water pressure measurement
#ifdef GARAGE
#define PIN_ANALOG_PRESSURE     A7
#else
#define PIN_ANALOG_PRESSURE     A0
#define PIN_ANALOG_PRESSURE_BOILER     A1
#endif

static constexpr uint32_t SERIAL_SPEED        = 9600U; ///< Set the baud rate for Serial I/O
// static constexpr uint8_t  SPRINTF_BUFFER_SIZE =  199U; ///< Buffer size for snprintf ()
// static constexpr char     format[] PROGMEM    = { "thermometer_wc,device=arduino02 current_uptime_s=%f,%s=%f,pressure_raw=%lu,pressure_bar=%f" };
static constexpr auto delay_between_measurement_ms = 1000U *
#ifdef DEBUG
  2U
#else
  60U
#endif
;

static uint32_t last_uptime_ms = 0U;

// ---- ETHERNET

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
static constexpr byte mac[] =
#ifdef GARAGE
  { 0xCE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static const IPAddress ip (192,168,9,188);
#define ONE_WIRE_BUS 9
#define PRESSURE_RANGE_PSI 30
static const String str_pressure_range PROGMEM = "pressure_range_psi: 30";
#else
  { 0xCD, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static const IPAddress ip (192,168,9,186);
#define ONE_WIRE_BUS 2
#define PRESSURE_RANGE_PSI 100
static const String str_pressure_range PROGMEM = "pressure_range_psi: 100";
#endif

EthernetUDP udp;
// the IP address of your InfluxDB host
// static constexpr byte host[] = {192, 168, 9, 23}; // serval.local
static constexpr byte host[] = {192, 168, 9, 4}; // cool.local
// the port that the UDP plugin is listening on; InfluxDB relies on 8089
// static constexpr size_t port = 8099; // dummy port
static constexpr size_t port = 8089; // influxDb port

#define TEMPERATURE_PRECISION 12

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire (ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors (&oneWire);

// arrays to hold device addresses
DeviceAddress devices[10];
static uint8_t devicesFound = 0U;

static constexpr float range_in_PSI = (float) PRESSURE_RANGE_PSI;

void printAddress (DeviceAddress deviceAddress);
String stringPrintAddress (DeviceAddress deviceAddress);
String printTemperature (DeviceAddress deviceAddress);

void LogData (const char* logLine);

void udp_shout (String message) {
  udp.beginPacket (host, port);
  udp.print (message); // don't use println!
  udp.endPacket ();
}

float pressure_converter_Pa (uint16_t sensorMeasure) {
  static constexpr float range_in_Volt = 5.0;
  static constexpr float offset_in_Volt = 0.47;
  static constexpr float max_Voltage = 4.5;
  static constexpr float PSI_per_bar = 14.503773773;
  static constexpr float PSI_per_Pa = PSI_per_bar / 100000.0;
  // const float PSI_per_Pa2 = 6894.7572932;
  static constexpr float conversion_Volt_to_PSI = range_in_PSI / (max_Voltage - offset_in_Volt);

  const float voltage = (sensorMeasure * range_in_Volt) / 1024;
  const float pressure_PSI = (conversion_Volt_to_PSI * (voltage - offset_in_Volt));
  // const float pressure_pascal = (3.0 * (voltage - 0.47)) * 1e6;
  const float pressure_pascal = pressure_PSI / PSI_per_Pa;
  return pressure_pascal;
}

void setup () {
  static const String str_compiler_version PROGMEM = F ("compiler_version:" __VERSION__);
#ifdef CPP_PATH
  static const String str_compiler_path PROGMEM = F ("compiler_path:" CPP_PATH);
#endif
#ifdef VERSION
  static const String str_version PROGMEM = F ("source_version:" VERSION);
#endif
#ifdef LASTDATE
  static const String str_lastdate PROGMEM = F ("source_lastdate:" LASTDATE);
#endif
#ifdef __TIMESTAMPISO__
  static const String str___timestampiso__ PROGMEM = F ("build_timestamp:" __TIMESTAMPISO__);
#endif
  static const String str_location PROGMEM = F ("location:"
#ifdef GARAGE
"chaufferie");
#else
"wc");
#endif
  // disable SD SPI
  pinMode (PIN_SD_SPI, OUTPUT);
  digitalWrite (PIN_SD_SPI, HIGH);
  pinMode (LED_BUILTIN, OUTPUT);

  // Open serial communications and wait for port to open:
  Serial.begin (SERIAL_SPEED);
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
  delay (3000);
#endif
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.print (F ("Multi thermal measurement speak to InfluxDb over UDP:"));
  Serial.println (F ("Starting program"));
  Serial.println (F ("- Built on " __DATE__ " at " __TIME__ " with"));
  Serial.println (str_compiler_version);

#ifdef CPP_PATH
  Serial.println (str_compiler_path);
#endif

#ifdef VERSION
  Serial.println (str_version);
#endif

#ifdef LASTDATE
  Serial.println (str_lastdate);
#endif

#ifdef __TIMESTAMPISO__
  Serial.println (str___timestampiso__);
#endif
  Serial.println (str_location);

  // --- THERMO ---
  // Start up the library
  sensors.begin ();

  // locate devices on the bus
  Serial.print (F ("Locating devices..."));
  Serial.print (F ("Found "));
  devicesFound = sensors.getDeviceCount ();
  Serial.print (devicesFound, DEC);
  Serial.println (F (" devices."));

  if (devicesFound == 0U)
  {
    Serial.println (F ("No DS18B20 was found.  Sorry, can't run without hardware. :("));
    while (true) {
      delay (1); // do nothing, no point running without Ethernet hardware
    }
  }
  // report parasite power requirements
  Serial.print (F ("Parasite power is: "));
  if (sensors.isParasitePowerMode ()) Serial.println ("ON");
  else Serial.println (F ("OFF"));

  for (uint8_t i = 0U; i < devicesFound; ++i)
    if (!sensors.getAddress (devices[i], i)) {
      Serial.print (F ("Unable to find address for Device"));
      Serial.println ((String) i);
    }

  // show the addresses we found on the bus
  for (uint8_t i = 0U; i < devicesFound; ++i)
  {
    Serial.print (F ("Device "));
    Serial.print ((String) i);
    Serial.print (F (" Address: "));
    printAddress (devices[i]);
    Serial.println ();
    Serial.println (stringPrintAddress (devices[i]));
    Serial.println ();
  }

  for (uint8_t i = 0U; i < devicesFound; ++i)
    sensors.setResolution (devices[i], TEMPERATURE_PRECISION);

  delay (1000);
  // start the Ethernet connection:
  Ethernet.init (PIN_ETH_SPI);  // Most Arduino shields
  if (Ethernet.begin (mac) == 0) {
     Serial.println (F ("Initialize Ethernet without DHCP:"));
     Ethernet.begin (mac, ip);
  }
  else
     Serial.println (F ("Initialize Ethernet with DHCP:"));
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus () == EthernetNoHardware) {
    Serial.println (F ("Ethernet shield was not found.  Sorry, can't run without hardware. :("));
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
  Serial.print (F ("IP is at "));
  Serial.println (Ethernet.localIP ());

  wdt_enable (WDTO_8S);
  Serial.println (F ("Setup ended"));
}

void printAddress (DeviceAddress deviceAddress)
{
  for (uint8_t i = 0U; i < 8; ++i)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print ("0");
    Serial.print (deviceAddress[i], HEX);
  }
}

String stringPrintAddress (DeviceAddress deviceAddress)
{
  String s ("");
  for (uint8_t i = 0U; i < 8; ++i)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16)
      s += "0";
      s += String (deviceAddress[i], HEX);
  }
  return s;
}

// function to print the temperature for a device
String printTemperature (DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC (deviceAddress);
  if (tempC < 10)
    return "0" + (String) tempC;
  else
    return (String) tempC;
}

// function to print a device's resolution
void printResolution (DeviceAddress deviceAddress)
{
  Serial.print (F ("Resolution: "));
  Serial.print (sensors.getResolution (deviceAddress));
  Serial.println ();
}

void loop () {
  // char          outputBuffer[SPRINTF_BUFFER_SIZE]; ///< Buffer for snprintf ()
  wdt_reset ();

  if (devicesFound == 0)
  {
     Serial.println (F ("No devices found."));
     return;
  }
  sensors.requestTemperatures ();

  unsigned long current_uptime_ms = millis ();
  if (current_uptime_ms - last_uptime_ms < delay_between_measurement_ms) return;
  {
#ifdef GARAGE
     String sent = "thermometer_chaufferie,device=arduino03 current_uptime_s=";
#else
     String sent = "thermometer_wc,device=arduino02 current_uptime_s=";
#endif
     sent += (String) (current_uptime_ms/1000) + '.' + (String) (current_uptime_ms%1000) + ',';
     for (uint8_t i = 0U; i < devicesFound; ++i)
     {
       sent += stringPrintAddress (devices[i]) + '=' + printTemperature (devices[i]);
       if (i != devicesFound - 1)
          sent += ',';
     }
     const uint16_t pressure_sensor_raw = (uint16_t) analogRead (PIN_ANALOG_PRESSURE);
     const float pressure_bar = pressure_converter_Pa (pressure_sensor_raw) / 100000.;
     sent += ",pressure_raw=" + (String) pressure_sensor_raw;
     sent += ",pressure_bar=" + (String) pressure_bar;

#ifndef GARAGE
     const uint16_t pressure_sensor_boiler_raw = (uint16_t) analogRead (PIN_ANALOG_PRESSURE_BOILER);
     const float pressure_boiler_bar = pressure_converter_Pa (pressure_sensor_boiler_raw) / 100000.;
     sent += ",pressure_boiler_raw=" + (String) pressure_sensor_boiler_raw;
     sent += ",pressure_boiler_bar=" + (String) pressure_boiler_bar;
#endif
#ifdef DEBUG
     Serial.println (sent);
#endif
     udp_shout (sent);

  //    const uint8_t  SPRINTF_BUFFER_SIZE =  199; ///< Buffer size for snprintf ()
  //    snprintf_P (outputBuffer, SPRINTF_BUFFER_SIZE, format,
  //                1, Start_Time, Start_Milli, Start_Micro,
  //                (Start_Time-Stop_Time), (Start_Milli-Stop_Milli), (Start_Micro-Stop_Micro));
  //       Serial.print (outputBuffer);
  //       udp_shout (String (outputBuffer));
  // give the web browser time to receive the data
     last_uptime_ms = current_uptime_ms;
  }
}
