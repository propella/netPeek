/* -*- c -*-
  NetPeek: A simple web client for Aruino Ethernet
 
 This sketch connects to a website
 (http://192.168.0.1/~tyamamiya/p4peek.cgi) to monitor the Perforce
 server
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * LED output is 8 (external) and 9 (internal)
 
 By Takashi Yamamiya based on http://arduino.cc/en/Tutorial/WebClient
 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <string.h>

#define GOOD_MARKER "PERFORCE-LOOKS-GOOD-AND-SMOOTH"

#define MAX_BUFFER 1024


byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x29, 0x8E }; // My MAC address

IPAddress server(192,168,0,1); // Target IP

int index = 0;
char buffer[MAX_BUFFER];

// Initialize the Ethernet client library
EthernetClient client;

void setup() {

  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT); // a built-in LED for Arduino Ethernet
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    while (true) {}
  }
}

void connect() {
  Serial.println("connecting...");
  
  if (client.connect(server, 80)) {
    Serial.println("connected");
    client.println("GET /~tyamamiya/p4peek.cgi HTTP/1.0");
    client.println();
  } 
  else {
    Serial.println("connection failed");
  }
}

// blink n times
// External LED: ON if isOn is true.
// Built-in LED: OFF if isOn is true.

void blink(boolean isON, int n) {
  Serial.print("Status: ");
  Serial.println(isON);
  // Repeat blink 5 times if the connection found
  for (int i = 0; i < n; i++) {
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
    delay(1000);
    digitalWrite(8, isON ? HIGH : LOW);
    digitalWrite(9, isON ? LOW : HIGH);
    delay(1000);
  }
}

void loop()
{
  if (!client.connected() || index >= MAX_BUFFER - 1) {
    Serial.println("Connection finished.");
    buffer[index] = '\0';
    boolean found = strstr(buffer, GOOD_MARKER) != NULL;
    Serial.println(buffer);
    index = 0;
    Serial.println();
    Serial.println("reset connection.");
    client.stop();
    blink(!found, 5);

    connect(); // Connect for next timesstep.
  }

  // Read and record from the server
  if (client.available()) {
    char c = client.read();
    buffer[index] = c;
    index++;
  }
}
