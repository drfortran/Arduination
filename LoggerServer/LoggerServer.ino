// Basic Arduino Web Server version 0.1 modified to include logging
// http://startingelectronics.org/software/arduino/web-server/basic-01/
//
// More details and web files available from:
// http://startingelectronics.org/software/arduino/web-server/01-log-data/
//
// Date: 11 July 2015
//
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#ifndef NO_RTC
#include <DS3231M.h> // Include the DS3231M RTC library
#endif

// maximum length of file name including path
#define FILE_NAME_LEN  20
// HTTP request type
#define HTTP_invalid   0
#define HTTP_GET       1
#define HTTP_POST      2

// file types
#define FT_HTML       0
#define FT_ICON       1
#define FT_CSS        2
#define FT_JAVASCRIPT 3
#define FT_JPG        4
#define FT_PNG        5
#define FT_GIF        6
#define FT_TEXT       7
#define FT_INVALID    8

// pin used for Ethernet chip SPI chip select
#define PIN_ETH_SPI   10

//const uint32_t SERIAL_SPEED        = 115200; ///< Set the baud rate for Serial I/O
const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O

long log_time_ms = 15000; // how often to log data in milliseconds
long prev_log_time = 0;   // previous time log occurred

// the media access control (ethernet hardware) address for the shield:
const byte mac[] PROGMEM = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//the IP address for the shield:
const byte ip[] = { 192, 168, 9, 184 };  // does not work if this is put into Flash
// the router's gateway address:
const byte gateway[] PROGMEM = { 192, 168, 9, 1 };
// the subnet:
const byte subnet[] PROGMEM = { 255, 255, 255, 0 };
EthernetServer server(80);

#ifndef NO_RTC
DS3231M_Class DS3231M;                          ///< Create an instance of the DS3231M Class
#endif

void setup() {
  // deselect Ethernet chip on SPI bus
  pinMode(PIN_ETH_SPI, OUTPUT);
  digitalWrite(PIN_ETH_SPI, HIGH);

  // Open serial communications and wait for port to open:
  Serial.begin(SERIAL_SPEED);       // for debugging
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
  delay(3000);
#endif
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  if (!SD.begin(4)) {
    Serial.println(F("SD card initialization failed! Things to check:"));
    Serial.println(F("1. is a card inserted?"));
    Serial.println(F("2. is your wiring correct?"));
    Serial.println(F("3. did you change the chipSelect pin to match your shield or module?"));
    Serial.println(F("Note: press reset or reopen this serial monitor after fixing your issue!"));
    return;  // SD card initialization failed
  }

  Serial.println(F("Initialize Ethernet without DHCP:"));
  Ethernet.begin((uint8_t*)mac, ip, gateway, subnet);
  delay(1000);
  server.begin();  // start listening for clients
  Serial.print(F("server is at "));
  Serial.println(Ethernet.localIP());
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(F("."));
  }
  Serial.println();

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
  // if an incoming client connects, there will be bytes available to read:
  EthernetClient client = server.available();
  if (client) {
    Serial.println(F("new client"));
    while (client.connected()) {
      if (ServiceClient(&client)) {
        // received request from client and finished responding
        break;
      }
    }  // while (client.connected())
    delay(1);
    client.stop();
  }  // if (client)
  // log the data
  LogData();
}// void loop()

void LogData(void)
{
  unsigned long current_time;   // current millisecond time
  File logFile;                 // file to log data to

  current_time = millis();  // get the current time in milliseconds
  if ((current_time - prev_log_time) > log_time_ms) {
    prev_log_time = current_time;
#ifndef NO_RTC
    DateTime RTC_now = DS3231M.now(); // get the current time
    unsigned long RTC_Time = RTC_now.unixtime();
//    Serial.print((String)RTC_Time);
//    Serial.print(F(" ~ "));
//    Serial.print((String)NTP_Time);
//    Serial.println(F(" ; "));
#endif
    // the log time has elapsed, so log another set of data
    logFile = SD.open("log.txt", FILE_WRITE);
    if (logFile) {
      logFile.print(current_time);
      logFile.print(" ; ");
#ifndef NO_RTC
      logFile.print(RTC_Time);
      logFile.print(" ; ");
#endif
      logFile.println(analogRead(5)); // log the analog pin value
      logFile.close();
    }
  }
}

bool ServiceClient(EthernetClient *client)
{
  static boolean currentLineIsBlank = true;
  char cl_char;
  File webFile;
  // file name from request including path + 1 of null terminator
  char file_name[FILE_NAME_LEN + 1] = {0};  // requested file name
  char http_req_type = 0;
  char req_file_type = FT_INVALID;
  const char *file_types[] = {"text/html", "image/x-icon", "text/css", "application/javascript", "image/jpeg", "image/png", "image/gif", "text/plain"};

  static char req_line_1[40] = {0};  // stores the first line of the HTTP request
  static unsigned char req_line_index = 0;
  static bool got_line_1 = false;

  if (client->available()) {   // client data available to read
    cl_char = client->read();

    if ((req_line_index < 39) && (got_line_1 == false)) {
      if ((cl_char != '\r') && (cl_char != '\n')) {
        req_line_1[req_line_index] = cl_char;
        req_line_index++;
      }
      else {
        got_line_1 = true;
        req_line_1[39] = 0;
      }
    }

    if ((cl_char == '\n') && currentLineIsBlank) {
      // get HTTP request type, file name and file extension type index
      http_req_type = GetRequestedHttpResource(req_line_1, file_name, &req_file_type);
      if (http_req_type == HTTP_GET) {         // HTTP GET request
        if (req_file_type < FT_INVALID) {      // valid file type
          webFile = SD.open(file_name);        // open requested file
          if (webFile) {
            // send a standard http response header
            client->println(F("HTTP/1.1 200 OK"));
            client->print(F("Content-Type: "));
            client->println(file_types[req_file_type]);
            client->println(F("Connection: close"));
            client->println();
            // send web page
            while(webFile.available()) {
              int num_bytes_read;
              char byte_buffer[64];
              // get bytes from requested file
              num_bytes_read = webFile.read(byte_buffer, 64);
              // send the file bytes to the client
              client->write(byte_buffer, num_bytes_read);
            }
            webFile.close();
          }
          else {
            // failed to open file
          }
        }
        else {
          // invalid file type
        }
      }
      else if (http_req_type == HTTP_POST) {
        // a POST HTTP request was received
      }
      else {
        // unsupported HTTP request received
      }
      req_line_1[0] = 0;
      req_line_index = 0;
      got_line_1 = false;
      // finished sending response and web page
      return 1;
    }
    if (cl_char == '\n') {
      currentLineIsBlank = true;
    }
    else if (cl_char != '\r') {
      currentLineIsBlank = false;
    }
  }  // if (client.available())
  return 0;
}

// extract file name from first line of HTTP request
char GetRequestedHttpResource(char *req_line, char *file_name, char *file_type)
{
  char request_type = HTTP_invalid;  // 1 = GET, 2 = POST. 0 = invalid
  char *str_token;

  *file_type = FT_INVALID;

  str_token =  strtok(req_line, " ");    // get the request type
  if (strcmp(str_token, "GET") == 0) {
    request_type = HTTP_GET;
    str_token =  strtok(NULL, " ");      // get the file name
    if (strcmp(str_token, "/") == 0) {
      strcpy(file_name, "index.htm");
      *file_type = FT_HTML;
    }
    else if (strlen(str_token) <= FILE_NAME_LEN) {
      // file name is within allowed length
      strcpy(file_name, str_token);
      // get the file extension
      str_token = strtok(str_token, ".");
      str_token = strtok(NULL, ".");

      if      (strcmp(str_token, "htm") == 0) {*file_type = 0;}
      else if (strcmp(str_token, "ico") == 0) {*file_type = 1;}
      else if (strcmp(str_token, "css") == 0) {*file_type = 2;}
      else if (strcmp(str_token, "js")  == 0) {*file_type = 3;}
      else if (strcmp(str_token, "jpg") == 0) {*file_type = 4;}
      else if (strcmp(str_token, "png") == 0) {*file_type = 5;}
      else if (strcmp(str_token, "gif") == 0) {*file_type = 6;}
      else if (strcmp(str_token, "txt") == 0) {*file_type = 7;}
      else {*file_type = 8;}
    }
    else {
      // file name too long
    }
  }
  else if (strcmp(str_token, "POST") == 0) {
    request_type = HTTP_POST;
  }

  return request_type;
}
