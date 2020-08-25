#include <Ethernet.h>
#include <SPI.h>

const uint32_t SERIAL_SPEED        = 9600; ///< Set the baud rate for Serial I/O

// ---- ETHERNET

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x08, 0x00, 0x59 }; // L'adresse MAC de votre shield Ethernet (normalement il se trouve sous la carte
IPAddress ip (192,168,9,179);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server (80);

boolean reading;
boolean lChambre; // État du relais

int in1 = 7;
bool hasBeenDisplayed = false;

void enAttente ();

void setup () {
  // Open serial communications and wait for port to open:
  Serial.begin (SERIAL_SPEED);
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait for the serial interface to initialize
  delay(3000);
#endif
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.print (F ("Relay control over the Web:"));
  Serial.println (F ("Starting program"));
  Serial.println (F ("- Built on " __DATE__ " at " __TIME__ " with"));
  const String str_compiler_version PROGMEM = F ("compiler_version:" __VERSION__);
  Serial.print(str_compiler_version);

#ifdef CPP_PATH
  const String str_compiler_path PROGMEM = F ("compiler_path:" CPP_PATH);
  Serial.print(str_compiler_path);
#endif

#ifdef VERSION
  const String str_version PROGMEM = F ("source_version:" VERSION);
  Serial.print(str_version);
#endif

#ifdef LASTDATE
  const String str_lastdate PROGMEM = F ("source_lastdate:" LASTDATE);
  Serial.print(str_lastdate);
#endif

#ifdef __TIMESTAMPISO__
  const String str___timestampiso__ PROGMEM = F ("build_timestamp:" __TIMESTAMPISO__);
  Serial.print(str___timestampiso__);
#endif

  pinMode (6, INPUT);    // Lire l'état du bouton poussoir
  pinMode (in1, OUTPUT); // Contrôler le relais
  reading = false;
  lChambre = false;
  Serial.println (F ("Initialize Ethernet without DHCP:"));
  Ethernet.begin (mac, ip);
  delay (1000);
  server.begin ();
  Serial.print (F ("Arduino connecte: "));
  Serial.print (F ("server is at "));
  Serial.println (Ethernet.localIP ());
}

void loop () {
  enAttente ();
}

void enAttente () {
  EthernetClient client = server.available ();
  if (client) {
    boolean currentLineIsBlank = true;
    boolean sentHeader = false;
    while (client.connected ()) {
      if (client.available ()) {
        if (!sentHeader) {
          Serial.println (F ("HTTP/1.1 200 OK"));
          client.println (F ("HTTP/1.1 200 OK"));
          client.println (F ("Content-Type: text/html; charset=utf-8"));
          client.println ();
          sentHeader = true;
        }
        char c = client.read ();
        if (reading && c == ' ') {reading = false;}
        if (c == '?') {reading = true;}
        if (reading) {
          Serial.print (c);
          switch (c) {
          case '1':
            lChambre = 0;
            digitalWrite (in1, lChambre);
            break;
          case '0':
            lChambre = 1;
            digitalWrite (in1, lChambre);
            break;
          }
          if (((c == '0') || (c == '1')) && !hasBeenDisplayed) {
            client.println (F ("<!DOCTYPE html>"));
            client.println (F ("<html>"));
            client.println (F ("  <head>"));
            client.println (F ("      <title>Contrôle chaudière</title>"));
            client.println (F ("    <style>"));
            client.println (F ("      * {"));
            client.println (F ("      font-size: 36px;"));
            client.println (F ("      }"));
            client.println (F ("      .button {"));
            client.println (F ("      background-color: green;"));
            client.println (F ("      color: white;"));
            client.println (F ("      padding: 15px 32px;"));
            client.println (F ("      text-align: center;"));
            client.println (F ("      display: inline-block;"));
            client.println (F ("      margin: 14px 12px;"));
            client.println (F ("      cursor: pointer;"));
            client.println (F ("    }"));
            client.println (F ("    </style>"));
            client.println (F ("  </head>"));
            client.println (F (" <body>"));
            client.println (F ("    <button class=\"button\" onclick=\"window.location.href = 'http://192.168.9.179/?0';\">ETEINDRE</button>"));
            client.println (F ("    <button class=\"button\" onclick=\"window.location.href = 'http://192.168.9.179/?1';\">ALLUMER</button>"));
            client.println (F ("<div>   Etat</div>"));

            if (lChambre == 0) {
              client.println (F ("<div> Allumé</div>"));
            }
            else {
              client.println (F ("<div> Éteint</div>"));
            };

            client.println (F (" </body></html>"));
            hasBeenDisplayed = true;
          }
        }
        if (c == '\n' && currentLineIsBlank) {break;}
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay (5); // Permettre au navigateur de tout recevoir
    client.stop ();
    hasBeenDisplayed = false;
  }
  delay (30); // Permettre au navigateur de tout recevoir
}
