/*
 * Copyright (c) 2010 by Blake Foster <blfoster@vassar.edu>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <utility/w5100.h>

#define REQ_DATASIZE 64
#define ICMP_ECHOREPLY 0
#define ICMP_ECHOREQ 8
#define ICMP_ECHOREP 0
#define PING_TIMEOUT 1000

typedef unsigned long time_t;

class ICMPHeader;
class ICMPPing;

typedef enum Status
{
    SUCCESS = 1,
    SEND_TIMEOUT = 2,
    NO_RESPONSE = 3,
    BAD_RESPONSE = 4
};


struct ICMPHeader
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
};

struct ICMPEcho
{
    ICMPEcho(uint8_t type, uint16_t _id, uint16_t _seq, uint8_t * _payload);
    ICMPEcho();
    ICMPHeader header;
    uint16_t id;
    uint16_t seq;
    time_t time;
    uint8_t payload [REQ_DATASIZE];
};

struct ICMPEchoReply
{
    ICMPEcho content;
    uint8_t ttl;
    Status status;
    uint8_t addr [4];
};

class ICMPPing
{
public:
    ICMPPing(SOCKET s, uint8_t id); // construct an ICMPPing object for socket s
    void operator()(byte * addr, int nRetries, ICMPEchoReply& result); // Ping addr, retrying nRetries times if no response is received.
    ICMPEchoReply operator()(byte * addr, int nRetries); // Ping addr, retrying nRetries times if no response is received. 
    // The respone is store in result.  The return value is true if a response is received, and false otherwise.
private:
    Status waitForEchoReply(); // wait for a response
    Status sendEchoRequest(byte * addr, const ICMPEcho& echoReq); // send an ICMP echo request
    void receiveEchoReply(ICMPEchoReply& echoReply); // read a resposne
    uint8_t _id;
    uint8_t _nextSeq;
    SOCKET _socket; // socket number to send ping
};

#pragma pack(1)
