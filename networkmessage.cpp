#include "networkmessage.h"
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
//#include <iostream.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#ifndef WIN32
#include <sys/ioctl.h>
#endif

using namespace std;

NetworkMessage::NetworkMessage()
{
	Reset();
}

NetworkMessage::~NetworkMessage()
{
	//
}

bool NetworkMessage::ReadFromSocket(SOCKET socket)
{
	// just read the size to avoid reading 2 messages at once
	m_MsgSize = ::recv(socket, (char*)m_MsgBuf, 2, 0);
	
	// for now we expect 2 bytes at once, it should not be splitted
	int datasize = m_MsgBuf[0] | m_MsgBuf[1] << 8;
	if((m_MsgSize != 2) || (datasize > NETWORKMESSAGE_MAXSIZE-2)){
		int errnum;
		errnum = errno;
		Reset();
		return false;
	}

	// read the real data
	m_MsgSize += ::recv(socket, (char*)m_MsgBuf+2, datasize, 0);

	// we got something unexpected/incomplete
	if((m_MsgSize <= 2) || ((m_MsgBuf[0] | m_MsgBuf[1] << 8) != m_MsgSize-2)){
		Reset();
		return false;
	}

	m_ReadPos = 2;
	return true;
}

bool NetworkMessage::WriteToSocket(SOCKET socket)
{
	if(m_MsgSize == 0){
		return true;
	}
	m_MsgBuf[2] = (unsigned char)(m_MsgSize);
	m_MsgBuf[3] = (unsigned char)(m_MsgSize >> 8);
  
	bool ret = true;
	int sendBytes = 0;
	int flags;

	flags = 0; // MSG_CONFIRM in linux
	int start = 2;

	do{
		//int b = send(socket, (char*)m_MsgBuf+sendBytes+start, std::min(m_MsgSize-sendBytes+2, 1000), flags);
		int b = send(socket, (char*)m_MsgBuf+sendBytes+start, std::min(m_MsgSize-sendBytes+2, 1000), flags);
		if(b <= 0){
			ret = false;
			break;
		}
		sendBytes += b;
	}while(sendBytes < m_MsgSize+2);
	std::cout << "Wrote " << (int) m_MsgSize+2 << " bytes to socket." << std::endl;
	return ret;
}

unsigned char NetworkMessage::getByte()
{
	return m_MsgBuf[m_ReadPos++];
}

unsigned char NetworkMessage::InspectByte(int offset /*= 0*/)
{
	return m_MsgBuf[offset + m_ReadPos];
}

unsigned short NetworkMessage::getU16()
{
	unsigned char c1 = getByte();
	unsigned char c2 = getByte();

	unsigned short v = (c1 | (c2 << 8));
	return v;
}

unsigned short NetworkMessage::InspectU16()
{
	unsigned char c1 = m_MsgBuf[m_ReadPos];
	unsigned char c2 = m_MsgBuf[m_ReadPos + 1];

	unsigned short v = (c1 | (c2 << 8));
	return v;
}

unsigned int NetworkMessage::getU32()
{
  unsigned char c1 = getByte();
	unsigned char c2 = getByte();
	unsigned char c3 = getByte();
	unsigned char c4 = getByte();

	unsigned int n = ((c1) | (c2 << 8) | (c3 << 16) | (c4 << 24));
	return n;
}

unsigned int NetworkMessage::InspectU32()
{
  unsigned char c1 = m_MsgBuf[m_ReadPos];
	unsigned char c2 = m_MsgBuf[m_ReadPos + 1];
	unsigned char c3 = m_MsgBuf[m_ReadPos + 2];
	unsigned char c4 = m_MsgBuf[m_ReadPos + 3];

	unsigned int n = ((c1) | (c2 << 8) | (c3 << 16) | (c4 << 24));
	return n;
}

std::string NetworkMessage::InspectString()
{
  int stringlen = InspectU16();
	int tmpoffset = 1;

	std::string ret = "";
	int tmplen = stringlen;
	while(tmplen != 0) {
		ret += m_MsgBuf[m_ReadPos + tmpoffset];
		tmpoffset++;
		--tmplen;
	}

	return ret;
}

std::string NetworkMessage::getString()
{
  int stringlen = getU16();

	std::string ret = "";
	int tmplen = stringlen;
	while(tmplen != 0) {
		ret += getByte();
		--tmplen;
	}

	return ret;
}

void NetworkMessage::skipBytes(int count)
{
  m_ReadPos += count;
	//need to do some boundary checks
}

void NetworkMessage::addByte(unsigned char value)
{
	m_MsgBuf[m_ReadPos++] = value;
	m_MsgSize++;
}

void NetworkMessage::addHex(uint8_t value)
{
	m_MsgBuf[m_ReadPos++] = value;
	m_MsgSize++;
}

void NetworkMessage::addU16(unsigned short value)
{
	*(unsigned short*)(m_MsgBuf + m_ReadPos) = value;
	m_ReadPos += 2; m_MsgSize += 2;
}

void NetworkMessage::addU32(unsigned int value)
{
	*(unsigned long*)(m_MsgBuf + m_ReadPos) = value;
	m_ReadPos += 4; m_MsgSize += 4;
}

void NetworkMessage::addString(const char* value)
{
	unsigned long stringLen = (unsigned long)strlen(value);
	if(stringLen > 8192){
		return;
	}
	
	addU16(stringLen);
	strcpy((char*)(m_MsgBuf + m_ReadPos), value);
	m_ReadPos += stringLen;
	m_MsgSize += stringLen;
}
