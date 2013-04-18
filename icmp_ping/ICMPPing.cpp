/*
 * Copyright (c) 2010 by Blake Foster <blfoster@vassar.edu>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "ICMPPing.h"

uint16_t _checksum(ICMPEcho * echo)
{
    // calculate the checksum of an ICMPEcho with all fields but icmpHeader.checksum populated
    int nleft = sizeof(ICMPEcho);
    uint16_t * w = (uint16_t *)echo;
    unsigned long sum = 0;
    while(nleft > 1)  
    {
        // we skip the checksum field itself when calculating the checksum.
        if (w != &echo->icmpHeader.checksum) sum += *w;
        w++;
        nleft -= 2;
    }
    // If the size of ICMPEcho in bytes is odd, we'll have one trailing byte left over to add.
    if(nleft)
    {
        uint16_t u = 0;
        *(uint8_t *)(&u) = *(uint8_t *)w;
        sum += u;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

ICMPEcho::ICMPEcho(uint8_t type, uint16_t _id, uint16_t _seq, uint8_t * _payload)
: seq(_seq), id(_id), time(millis())
{
    memcpy(payload, _payload, REQ_DATASIZE);
    icmpHeader.type = type;
    icmpHeader.code = 0;
    icmpHeader.checksum = _checksum(this);
}

ICMPEcho::ICMPEcho()
: seq(0), id(0), time(0)
{
    memset(payload, 0, sizeof(payload));
    icmpHeader.code = 0;
    icmpHeader.type = 0;
    icmpHeader.checksum = 0;
}

ICMPPing::ICMPPing(SOCKET socket, uint8_t id)
: _socket(socket), _id(id)
{}

void ICMPPing::operator()(byte * addr, int nRetries, ICMPEchoReply& result)
{
    W5100.execCmdSn(_socket, Sock_CLOSE);
    W5100.writeSnIR(_socket, 0xFF);
    W5100.writeSnMR(_socket, SnMR::IPRAW);
    W5100.writeSnPROTO(_socket, IPPROTO::ICMP);
    W5100.writeSnPORT(_socket, 0);
    W5100.execCmdSn(_socket, Sock_OPEN);

    static uint8_t payload [REQ_DATASIZE];
    memset(payload, 0x1A, REQ_DATASIZE);

    ICMPEcho echoReq(ICMP_ECHOREQ, _id, _nextSeq++, payload);

    for (int i=0; i<nRetries; ++i)
    {
        result.status = sendEchoRequest(addr, echoReq);
        if (result.status != SUCCESS) continue;

        result.status = waitForEchoReply();
        if (result.status == SUCCESS)
        {
            byte replyAddr [4];
            receiveEchoReply(result);
            break;
        }
    }
   
    W5100.execCmdSn(_socket, Sock_CLOSE);
    W5100.writeSnIR(_socket, 0xFF);
}

ICMPEchoReply ICMPPing::operator()(byte * addr, int nRetries)
{
    ICMPEchoReply reply;
    operator()(addr, nRetries, reply);
    return reply;
}

Status ICMPPing::waitForEchoReply()
{
    time_t start = millis();
    while (!W5100.getRXReceivedSize(_socket))
    {
        if (millis() - start > PING_TIMEOUT) return NO_RESPONSE;
    }
    return SUCCESS;
}

Status ICMPPing::sendEchoRequest(byte * addr, const ICMPEcho& echoReq)
{
    W5100.writeSnDIPR(_socket, addr);
    // The port isn't used, becuause ICMP is a network-layer protocol. So we
    // write zero. This probably isn't actually necessary.
    W5100.writeSnDPORT(_socket, 0);
    W5100.send_data_processing(_socket, (uint8_t *)&echoReq, sizeof(ICMPEcho));
    W5100.execCmdSn(_socket, Sock_SEND);
    while ((W5100.readSnIR(_socket) & SnIR::SEND_OK) != SnIR::SEND_OK) 
    {
        if (W5100.readSnIR(_socket) & SnIR::TIMEOUT)
        {
            W5100.writeSnIR(_socket, (SnIR::SEND_OK | SnIR::TIMEOUT));
            return SEND_TIMEOUT;
        }
    }
    W5100.writeSnIR(_socket, SnIR::SEND_OK);
    return SUCCESS;
}

void ICMPPing::receiveEchoReply(ICMPEchoReply& echoReply)
{
    uint8_t ipHeader [6];
    uint8_t buffer = W5100.readSnRX_RD(_socket);
    W5100.read_data(_socket, (uint8_t *)buffer, ipHeader, sizeof(ipHeader));
    buffer += sizeof(ipHeader);
    for (int i=0; i<4; ++i) echoReply.addr[i] = ipHeader[i];
    uint8_t dataLen = ipHeader[4];
    dataLen = (dataLen << 8) + ipHeader[5];
    if (dataLen > sizeof(ICMPEcho)) dataLen = sizeof(ICMPEcho);
    W5100.read_data(_socket, (uint8_t *)buffer, (uint8_t *)&echoReply.data, dataLen);
    buffer += dataLen;
    W5100.writeSnRX_RD(_socket, buffer);
    W5100.execCmdSn(_socket, Sock_RECV);
    echoReply.ttl = W5100.readSnTTL(_socket);
    echoReply.status = (echoReply.data.icmpHeader.type == ICMP_ECHOREP) ? SUCCESS : BAD_RESPONSE;
}
