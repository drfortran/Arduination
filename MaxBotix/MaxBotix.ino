/* Arduino example code for MaxBotix MB1240 XL-MaxSonar-EZ4 ultrasonic distance sensor: pulse width output. More info: www.www.makerguides.com */

#define sensorPin 2

const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O

void setup () {
  pinMode (sensorPin, INPUT);
  Serial.begin (SERIAL_SPEED);
  Serial.println (" Start");
  Serial.print (F ("Ultrasonic fuel level measurement:"));
  Serial.println (F ("Starting program"));
  Serial.println (F ("- Built on " __DATE__ " at " __TIME__ " with"));
  const String str_compiler_version PROGMEM = F ("compiler_version:" __VERSION__);
  Serial.println (str_compiler_version);

#ifdef CPP_PATH
  const String str_compiler_path PROGMEM = F ("compiler_path:" CPP_PATH);
  Serial.println (str_compiler_path);
#endif

#ifdef VERSION
  const String str_version PROGMEM = F ("source_version:" VERSION);
  Serial.println (str_version);
#endif

#ifdef LASTDATE
  const String str_lastdate PROGMEM = F ("source_lastdate:" LASTDATE);
  Serial.println (str_lastdate);
#endif

#ifdef __TIMESTAMPISO__
  const String str___timestampiso__ PROGMEM = F ("build_timestamp:" __TIMESTAMPISO__);
  Serial.println (str___timestampiso__);
#endif
}

long read_sensor() {
  long distance = pulseIn (sensorPin, HIGH);
  return distance;
}

void print_data (long distance) {
  Serial.print (F ("distance(㎜)= "));
  Serial.println (distance);
}

void loop() {
  print_data (read_sensor ());
  delay (1000);
}

