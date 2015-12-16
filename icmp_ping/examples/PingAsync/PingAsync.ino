/*
 * PingAsync.cpp
 * This example uses the asynchronous methods of ICMPPing to send a ping,
 * do "other stuff" and check back for results periodically.
 *
 *
 * See the basic "Ping" example, on which this is based, for simpler
 * (but synchronous--meaning code will be frozen while awaiting responses)
 * usage.
 *
 * Setup: Configure the various defines in the Configuration section, below,
 * to select whether to use DHCP or a static IP and to choose the remote host
 * to ping.
 *
 * Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * OR other wiznet 5100-based board (tested on the WIZ811MJ, for example),
 * suitably connected.
 *
 *  Created on: Dec 12, 2015
 *      Author: Pat Deegan
 *      Part of the ICMPPing Project
 *      Copyright (C) 2015 Pat Deegan, http://psychogenic.com
 * 
 * This file is free software; you can redistribute it and/or modify it under the terms 
 * of either the GNU General Public License version 2 or the GNU Lesser General Public 
 * License version 2.1, both as published by the Free Software Foundation.
 */
#include <SPI.h>
#include <Ethernet.h>
#include <ICMPPing.h>


/* ************ Configuration ************** */


// NOTE: Use *COMMAS* (,) between IP values, as we're
// using these to init the constructors

// PING_REMOTE_IP -- remote host to ping (see NOTE above)
// so, e.g., 162.220.162.142 becomes 162, 220, 162, 142
#define PING_REMOTE_IP      162, 220, 162, 142


// TEST_USING_STATIC_IP -- (see NOTE above)
// leave undefined (commented out)
// to use DHCP instead.
// #define TEST_USING_STATIC_IP  192, 168, 2, 177




// PING_REQUEST_TIMEOUT_MS -- timeout in ms.  between 1 and 65000 or so
// save values: 1000 to 5000, say.
#define PING_REQUEST_TIMEOUT_MS     2500

#define LOCAL_MAC_ADDRESS     0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED

#ifndef ICMPPING_ASYNCH_ENABLE
#error "Asynchronous functions only available if ICMPPING_ASYNCH_ENABLE is defined -- see ICMPPing.h"
#endif



byte mac[] = {LOCAL_MAC_ADDRESS}; // max address for ethernet shield

#ifdef TEST_USING_STATIC_IP
byte ip[] = {TEST_USING_STATIC_IP}; // ip address for ethernet shield
#endif

IPAddress pingAddr(PING_REMOTE_IP); // ip address to ping

SOCKET pingSocket = 0;

char buffer [256];
ICMPPing ping(pingSocket, 1);

void dieWithMessage(const char * msg)
{

  for (;;)
  {
    Serial.println(msg);
    delay(500);
  }
}

void setup()
{
  // start Ethernet
  Serial.begin(115200);

  Serial.println("PingAsync booted...");
  Serial.print("Configuring ethernet with ");

#ifdef TEST_USING_STATIC_IP

  Serial.println("static ip");
  Ethernet.begin(mac, ip);
#else

  Serial.print("DHCP...");
  if (! Ethernet.begin(mac) )
  {

    Serial.println("FAILURE");
    dieWithMessage("Couldn't init ethernet using DHCP?!");
  }

  Serial.println("Success!");
#endif

  // increase the default time-out, if needed, assuming a bad
  // connection or whatever.
  ICMPPing::setTimeout(PING_REQUEST_TIMEOUT_MS);

}


// lastPingSucceeded -- just a flag so we don't drown in
// output when things are going wrong...
bool lastPingSucceeded = false;

// someCriticalStuffThatCantWait is just a toy function
// to represent the *reason* you want to use asynchronous
// calls rather than wait around for pings to come home.
// Here, we just print out some chars...
void someCriticalStuffThatCantWait()
{
  for (int i = 0; i < 10; i++)
  {
    if (lastPingSucceeded) {
      Serial.print('.');
    }
  }
  Serial.print('_');
}

void loop()
{

  lastPingSucceeded = false;
  ICMPEchoReply echoResult;  // we'll get the status here
  Serial.println("Starting async ping.");

  // asynchStart will return false if we couldn't
  // even send a ping, though you could also check the
  // echoResult.status with should ==
  if (! ping.asyncStart(pingAddr, 3, echoResult))
  {
    Serial.print("Couldn't even send our ping request?? Status: ");
    Serial.println((int)echoResult.status);
    delay(500);
    return;

  }

  // ok the ping started out well...
  Serial.println("Ping sent ");
  while (! ping.asyncComplete(echoResult))
  {
    // we have critical stuff we wish to handle
    // while we wait for ping to come back
    someCriticalStuffThatCantWait();
  }

  // async is done!  let's see how it worked out...
  if (echoResult.status != SUCCESS)
  {
    // failure... but whyyyy?
    sprintf(buffer, "Echo request failed; %d", echoResult.status);
  } else {
    // huzzah
    lastPingSucceeded = true;
    sprintf(buffer,
            "Reply[%d] from: %d.%d.%d.%d: bytes=%d time=%ldms TTL=%d",
            echoResult.data.seq,
            echoResult.addr[0],
            echoResult.addr[1],
            echoResult.addr[2],
            echoResult.addr[3],
            REQ_DATASIZE,
            millis() - echoResult.data.time,
            echoResult.ttl);
  }

  Serial.println(buffer);
  delay(500);
}

