/*
  Ping Example
 
 This example repeatedly sends ICMP pings and sends the result over the serial port.
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 created 30 Sep 2010
 by Blake Foster
 
 */

#include <SPI.h>         
#include <Ethernet.h>
#include <ICMPPing.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // max address for ethernet shield
byte ip[] = {192,168,2,177}; // ip address for ethernet shield
byte pingAddr[] = {192,168,2,1}; // ip address to ping

SOCKET pingSocket = 0;

char buffer [256];

void setup() 
{
  // start Ethernet
  Ethernet.begin(mac, ip);
  Serial.begin(9600);
}

void loop()
{
  ICMPPing ping(pingSocket);
  ping(4, pingAddr, buffer);
  Serial.println(buffer);
  delay(500);
}










