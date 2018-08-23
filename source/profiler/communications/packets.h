#pragma once
#include <string>

class Packet
{
	static const unsigned char ID = 0x00;

	unsigned char m_Type;
	double m_Time;

public:
	virtual unsigned int GetSize();
	virtual void ToBuffer(unsigned char*);
	void SetTime(double);

	Packet(unsigned char type, double time);
private:
	Packet();
};

class PacketHandshake : public Packet
{
	static const unsigned char ID = 0x01;

	int m_Magic0;
	int m_Magic1;
public:
	virtual unsigned int GetSize();
	virtual void ToBuffer(unsigned char*);

	PacketHandshake(double time, long long int);
	PacketHandshake(double time, char m0, char m1, char m2, char m3, char m4, char m5, char m6, char m7);
};

class PacketProfileScopeIn : public Packet
{
	static const unsigned char ID = 0x10;

	unsigned char m_Size;
	char m_Scope[255];

public:
	virtual unsigned int GetSize();
	virtual void ToBuffer(unsigned char*);

	PacketProfileScopeIn(double time, std::string scope);
	PacketProfileScopeIn(unsigned char id, double time, std::string scope);
};

class PacketProfileScopeOut : public PacketProfileScopeIn
{
	static const unsigned char ID = 0x11;

	double m_Nanoseconds;

public:
	virtual unsigned int GetSize();
	virtual void ToBuffer(unsigned char*);

	void SetNanoseconds(double);

	PacketProfileScopeOut(double time, std::string scope, unsigned int useconds);
};