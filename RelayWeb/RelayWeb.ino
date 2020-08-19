#include <Ethernet.h>
#include <SPI.h>
boolean reading;
boolean lChambre; // Etat du relais
boolean eBouton;  // Etat des interrupteurs
byte mac[] = { 0x90, 0xA2, 0xDA, 0x08, 0x00, 0x59 }; // L'adresse MAC de votre shield Ethernet (normalement il se trouve sous la carte
// byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE };
IPAddress ip (192,168,9,179);
// EthernetServer server = EthernetServer (80);
EthernetServer server (80);
int in1 = 7;
bool hasBeenDisplayed = false;

void setup (){
  Serial.begin (9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  pinMode (6, INPUT); // Lire l'état du bouton poussoir
  pinMode (in1, OUTPUT);// Controler le relais
  reading = false;
  lChambre = false;
  eBouton = false;
  // Ethernet.begin (mac);
  Serial.println ("Initialize Ethernet without DHCP:");
  Ethernet.begin (mac, ip);
  //    Ethernet.begin (mac);
  delay (1000);
  server.begin ();
  Serial.print ("Arduino connecte: ");
  Serial.print ("server is at ");
  Serial.println (Ethernet.localIP ());
}

void loop (){
  enAttente ();
  if (digitalRead (6) == LOW && eBouton){
    lChambre = !lChambre;
    digitalWrite (in1, lChambre);
    delay (300); // Ce délai est nécessaire sinon chaque fois vous appuyez, Arduino en compte 2 appuis
  }
}

void enAttente (){
  // Serial.print ("Arduino loop... ");
  EthernetClient client = server.available ();
  if (client){
    boolean currentLineIsBlank = true;
    boolean sentHeader = false;
    while (client.connected ()){
      if (client.available ()){
        if (!sentHeader){
          Serial.println ("HTTP/1.1 200 OK");
          client.println ("HTTP/1.1 200 OK");
          client.println ("Content-Type: text/html; charset=utf-8");
          client.println ();
          sentHeader = true;
        }
        char c = client.read ();
        if (reading && c == ' '){reading = false;}
        if (c == '?'){reading = true;}
        if (reading){
          Serial.print (c);
          switch (c){
          case '1':
            //                            lChambre = !lChambre;
            lChambre = 0;
            digitalWrite (in1, lChambre);
            break;
          case '0':
            //                            eBouton = !eBouton; // activer / désactiver les interrupteurs
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
        if (c == '\n' && currentLineIsBlank){break;}
        if (c == '\n'){
          currentLineIsBlank = true;
        }else if (c != '\r'){
          currentLineIsBlank = false;
        }
      }
    }
    delay (5); // Permettre au navigateur de tout recevoir
    client.stop ();
    hasBeenDisplayed = false;
  }
  delay (30); //Permettre au navigateur de tout recevoir
}
