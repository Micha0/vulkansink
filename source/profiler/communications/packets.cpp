#include "packets.h"
#include "utils/log.h"
#include <cstring>

namespace {
	union Convert {
		unsigned long long L;
		unsigned int I[2];
		unsigned char C[8];
		long long sL;
		int sI[2];
		char sC[8];
	};

	unsigned long long intsToLong(unsigned int a, unsigned int b)
	{
		Convert conv;

		conv.I[0] = a;
		conv.I[1] = b;

		return conv.L;
	}

	int* longToInts(long long int a)
	{
		Convert conv;

		conv.sL = a;

		return conv.sI;
	}

	char* intsToChars(int a, int b)
	{
		Convert conv;

		conv.sI[0] = a;
		conv.sI[1] = b;

		return conv.sC;
	}

	unsigned char* longToChars(unsigned long long int a)
	{
		Convert conv;

		conv.L = a;

		return conv.C;
	}

	int* charsToInts(char m0, char m1, char m2, char m3, char m4, char m5, char m6, char m7)
	{
		Convert conv;

		conv.sC[0] = m0;
		conv.sC[1] = m1;
		conv.sC[2] = m2;
		conv.sC[3] = m3;
		conv.sC[4] = m4;
		conv.sC[5] = m5;
		conv.sC[6] = m6;
		conv.sC[7] = m7;

		return conv.sI;
	}
}

Packet::Packet()
	: m_Type(Packet::ID)
	, m_Time(0)
{
}

Packet::Packet(unsigned char type, double time)
	: m_Type(type)
	, m_Time(time)
{
}

unsigned int Packet::GetSize()
{
	return sizeof(m_Type) + sizeof(m_Time);
}

void Packet::ToBuffer(unsigned char* buffer)
{
	buffer[0] = m_Type;
	std::memcpy(buffer + 1, (unsigned char*)&m_Time, sizeof(m_Time));
}

void Packet::SetTime(double time)
{
	m_Time = time;
}


PacketHandshake::PacketHandshake(double time, long long int magic)
		: Packet(PacketHandshake::ID, time)
{
	int* ints = longToInts(magic);
	m_Magic0 = ints[0];
	m_Magic1 = ints[1];

	char* magicStrRaw = (char*)longToChars(magic);
	std::string magicStr(magicStrRaw, 8);

	LOGI("Set Magic: %s", magicStr.c_str());
	LOGI("Set Magic: %d %d %d %d %d %d %d %d", magicStrRaw[0], magicStrRaw[1], magicStrRaw[2], magicStrRaw[3], magicStrRaw[4], magicStrRaw[5], magicStrRaw[6], magicStrRaw[7]);
}

PacketHandshake::PacketHandshake(double time, char m0, char m1, char m2, char m3, char m4, char m5, char m6, char m7)
		: Packet(PacketHandshake::ID, time)
{
	int* ints = charsToInts(m0, m1, m2, m3, m4, m5, m6, m7);
	m_Magic0 = ints[0];
	m_Magic1 = ints[1];

	char* magicStrRaw = intsToChars(m_Magic0, m_Magic1);
	std::string magicStr(magicStrRaw, 8);

	LOGI("Set Magic: %s", magicStr.c_str());
	LOGI("Set Magic: %d %d %d %d %d %d %d %d", magicStrRaw[0], magicStrRaw[1], magicStrRaw[2], magicStrRaw[3], magicStrRaw[4], magicStrRaw[5], magicStrRaw[6], magicStrRaw[7]);
}

unsigned int PacketHandshake::GetSize()
{
	return Packet::GetSize() + sizeof(m_Magic0) + sizeof(m_Magic1);
}

void PacketHandshake::ToBuffer(unsigned char* buffer)
{
	Packet::ToBuffer(buffer);

	unsigned int bufferIndexOffset = Packet::GetSize();
	unsigned char* restBuffer = buffer + bufferIndexOffset;
	std::memcpy(restBuffer, (unsigned char*)&m_Magic0, sizeof(m_Magic0));
	std::memcpy(restBuffer + sizeof(m_Magic0), (unsigned char*)&m_Magic1, sizeof(m_Magic1));
}

PacketProfileScopeIn::PacketProfileScopeIn(double time, std::string _scope)
		: Packet(PacketProfileScopeIn::ID, time)
		, m_Scope()
		, m_Size(_scope.size())
{
	std::memcpy(m_Scope, _scope.c_str(), m_Size);
}

PacketProfileScopeIn::PacketProfileScopeIn(unsigned char id, double time, std::string _scope)
		: Packet(id, time)
		, m_Scope()
		, m_Size(_scope.size())
{
	std::memcpy(m_Scope, _scope.c_str(), m_Size);
}

unsigned int PacketProfileScopeIn::GetSize()
{
	return Packet::GetSize() + 1 + m_Size;
}

void PacketProfileScopeIn::ToBuffer(unsigned char* buffer)
{
	Packet::ToBuffer(buffer);

	unsigned int bufferIndexOffset = Packet::GetSize();
	unsigned char* restBuffer = buffer + bufferIndexOffset;
	restBuffer[0] = (char)m_Size;
	std::memcpy(restBuffer + 1, m_Scope, (size_t)m_Size);
}

PacketProfileScopeOut::PacketProfileScopeOut(double time, std::string scope, unsigned int useconds)
		: PacketProfileScopeIn(PacketProfileScopeOut::ID, time, scope)
		, m_Nanoseconds(useconds)
{
}

unsigned int PacketProfileScopeOut::GetSize()
{
	return PacketProfileScopeIn::GetSize() + sizeof(m_Nanoseconds);
}

void PacketProfileScopeOut::ToBuffer(unsigned char* buffer)
{
	PacketProfileScopeIn::ToBuffer(buffer);

	unsigned int bufferIndexOffset = PacketProfileScopeIn::GetSize();
	unsigned char* restBuffer = buffer + bufferIndexOffset;
	unsigned char* msChar = (unsigned char*)&m_Nanoseconds;
	std::memcpy(restBuffer, msChar, sizeof(m_Nanoseconds));
}

void PacketProfileScopeOut::SetNanoseconds(double usec)
{
	m_Nanoseconds = usec;
}
