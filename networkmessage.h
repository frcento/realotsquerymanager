#ifndef __NETWORKMESSAGE__
#define __NETWORKMESSAGE__

#include "definitions.h"

#include <vector>
#include <string>
#include <cmath>

#define NETWORKMESSAGE_MAXSIZE 665535

class NetworkMessage{
public:
	NetworkMessage();
	~NetworkMessage();

	void Reset(){
		m_MsgSize = 0;
		m_ReadPos = 4;
	}

	bool ReadFromSocket(SOCKET socket);
	bool WriteToSocket(SOCKET socket);

	unsigned char getByte();
	unsigned char InspectByte(int offset = 0);

	unsigned short getU16();
	unsigned short InspectU16();
	std::string InspectString();

	unsigned int getU32();
	unsigned int InspectU32();
	
	void addByte(unsigned char value);
	void addHex(uint8_t value);
	void addU16(unsigned short value);
	void addU32(unsigned int value);
	void addString(const char* value);

	int getMessageLength(){ return m_MsgSize; }

	std::string getString();
	std::string GetRaw();

	void skipBytes(int count);

private:
	int m_MsgSize;
	int m_ReadPos;
	unsigned char m_MsgBuf[NETWORKMESSAGE_MAXSIZE];
};

#endif
