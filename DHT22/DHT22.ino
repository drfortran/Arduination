#include "DHT.h" //including the dht22 library
// https://github.com/adafruit/DHT-sensor-library
#define DHTPIN 8 //Declaring pin 9 of arduino for the dht22
#define DHTTYPE DHT22 //Defining which type of dht22 we are using (DHT22 or DHT11)

const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O

DHT dht(DHTPIN, DHTTYPE); //Declaring a variable named dht

void setup() {
  Serial.begin (SERIAL_SPEED); //setting the baud rate at 9600
  #ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
    delay(3000);
  #endif
  Serial.print (F("\nStarting Set program\n"));
  Serial.print (F("- Compiled with c++ version "));
  Serial.print (F(__VERSION__));
  Serial.print (F("\n- On "));
  Serial.print (F(__DATE__));
  Serial.print (F(" at "));
  Serial.print (F(__TIME__));
  Serial.print (F("\n"));
  dht.begin (); //This command will start to receive the values from dht22
}

void loop() {
  float hum = dht.readHumidity ();
  float temp = dht.readTemperature (); // Reading the temperature as Celsius
  float fah = dht.readTemperature (true);
  if (isnan (hum) || isnan (temp) || isnan (fah)) {
    Serial.println ("Failed to read from DHT sensor!");
    return;
  }
  float heat_index = dht.computeHeatIndex (fah, hum);
  float heat_indexC = dht.convertFtoC (heat_index);
  Serial.print (millis ());
  Serial.print (" ms ");
  Serial.print ("Humidity: ");
  Serial.print (hum);
  Serial.print (" %\t");
  Serial.print ("Temperature: ");
  Serial.print (temp);
  Serial.print (" °C ");
  Serial.print (fah);
  Serial.print (" °F\t");
  Serial.print ("Heat index: ");
  Serial.print (heat_indexC);
  Serial.print (" °C ");
  Serial.print (heat_index);
  Serial.println (" °F ");
  delay (2000);
}
