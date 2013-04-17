/*
 * Copyright (c) 2010 by Blake Foster <blfoster@vassar.edu>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "ICMPPing.h"

ICMPEcho::ICMPEcho(uint8_t type, uint16_t _id, uint16_t _seq, uint8_t * _payload)
: seq(_seq), id(_id), time(millis())
{
    memcpy(payload, _payload, REQ_DATASIZE);
    header.type = type;
    header.code = 0;
    header.checksum = 0;

    int nleft = sizeof(ICMPEcho);
    uint16_t * w = (uint16_t *)this;
    unsigned long sum = 0;
    while(nleft > 1)  
    {
        sum += *w++;
        nleft -= 2;
    }
    if(nleft)
    {
        uint16_t u = 0;
        *(uint8_t *)(&u) = *(uint8_t *)w;
        sum += u;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    header.checksum = ~sum;
}


ICMPEcho::ICMPEcho()
: seq(0), id(0), time(0)
{
    memset(payload, 0, sizeof(payload));
    header.code = 0;
    header.type = 0;
    header.checksum = 0;
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
    uint16_t port = 0;
    uint8_t header [6];
    uint8_t buffer = W5100.readSnRX_RD(_socket);
    W5100.read_data(_socket, (uint8_t *)buffer, header, sizeof(header));
    buffer += sizeof(header);
    for (int i=0; i<4; ++i) echoReply.addr[i] = header[i];
    uint8_t dataLen = header[4];
    dataLen = (dataLen << 8) + header[5];
    if (dataLen > sizeof(ICMPEcho)) dataLen = sizeof(ICMPEcho);
    W5100.read_data(_socket, (uint8_t *)buffer, (uint8_t *)&echoReply.content, dataLen);
    buffer += dataLen;
    W5100.writeSnRX_RD(_socket, buffer);
    W5100.execCmdSn(_socket, Sock_RECV);
    echoReply.ttl = W5100.readSnTTL(_socket);
    echoReply.status = (echoReply.content.header.type == ICMP_ECHOREP) ? SUCCESS : BAD_RESPONSE;
}
