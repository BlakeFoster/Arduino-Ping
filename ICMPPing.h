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

#define REQ_DATASIZE 32
#define ICMP_ECHOREPLY 0
#define ICMP_ECHOREQ 8
#define PING_TIMEOUT 1000

typedef unsigned long time_t;

class ICMPHeader;
template <int dataSize>
class ICMPMessage;

class ICMPPing
{
public:
	ICMPPing(SOCKET s); // construct an ICMPPing object for socket s
	bool operator()(int nRetries, byte * addr, char * result); // Ping addr, retrying nRetries times if no response is received. 
	// The respone is store in result.  The return value is true if a response is received, and false otherwise.
private:
	bool waitForEchoReply(); // wait for a response
	size_t sendEchoRequest(byte * addr); // send an ICMP echo request
	uint8_t receiveEchoReply(byte * addr, uint8_t& TTL, time_t& time); // read a respone
	SOCKET socket; // socket number to send ping
};


class ICMPHeader
{
	friend class ICMPPing;
public:
	ICMPHeader(uint8_t Type);
	ICMPHeader();
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t seq;
	static int lastSeq;
	static int lastId;
};

template <int dataSize>
class ICMPMessage
{
	friend class ICMPPing;
public:
	ICMPMessage(uint8_t type);
	ICMPMessage();
	void initChecksum();
	uint8_t& operator[](int i);
	const uint8_t& operator[](int i) const;
	ICMPHeader icmpHeader;
	time_t time;
private:
	uint8_t data [dataSize];
};

template <int dataSize>
inline uint8_t& ICMPMessage<dataSize>::operator[](int i)
{
	return data[i];
}

template <int dataSize>
inline const uint8_t& ICMPMessage<dataSize>::operator[](int i) const
{
	return data[i];
}

typedef ICMPMessage<REQ_DATASIZE> EchoRequest;
typedef ICMPMessage<REQ_DATASIZE> EchoReply;

#pragma pack(1)