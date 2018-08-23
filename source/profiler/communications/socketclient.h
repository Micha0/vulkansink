/*
 * SocketClient
 *
 * A class for handling Socket connections.
 *
 * This class is designed for Shoot-and-forget non-blocking connections
 *
 * TODO: Needs to be a HOST!!
 */
#pragma once
#include <queue>
#include <thread>
#include <string>
#include <mutex>
#include <netinet/in.h>
#include "packets.h"
#include <vector>

class SocketConnection
{
public:
	int m_Sock;
	struct sockaddr_in m_Address;
	SocketConnection();
};

class SocketClient
{
private:
	enum RESPONSE_STATUS {
		ERROR = 0,
		OK = 1
	};

	enum CONNECTION_TYPE {
		NONE = 0,
		CLIENT = 1,
		SERVER = 2
	};
	/* in-thread vars and methods */
	int m_Sock;
	std::mutex m_MutexSock;
	std::string m_Address;
	int m_Port;
	std::mutex m_MutexAddressAndPort;
	struct sockaddr_in m_Server;
	std::vector<std::shared_ptr<SocketConnection> > m_Clients;
	std::mutex m_MutexClients;
	std::shared_ptr<PacketHandshake> m_HelloPacket;

	bool Prepare();
	bool Connect();
	bool Listen();
	bool SendPacket(const std::shared_ptr<Packet>& packet);
	RESPONSE_STATUS Receive(int);

	/* Separate listen thread */
	bool m_StopListenRequested;
	std::mutex m_MutexStopListenRequested;
	bool IsStopListenRequested();
	void SetStopListenRequested(bool);
	void ListenLoop();
	std::thread m_ListenThread;

	/* these vars needs to transfer */
	std::queue<std::shared_ptr<Packet> > m_Queue;
	std::mutex m_MutexQueue;

	bool m_IsConnected;
	std::mutex m_MutexIsConnected;

	bool m_ThreadIsRunning;
	std::mutex m_MutexThreadIsRunning;

	void SetIsConnected(bool);
	bool IsConnected();
	void AddToQueue(const std::shared_ptr<Packet>&);

	/* out-thread vars and methods */
	std::thread m_Thread;
	void StartThread();
	void RunThread(CONNECTION_TYPE);
	void FinishThread();
	CONNECTION_TYPE m_ConnectionType;

public:
	SocketClient(unsigned long long int time);
	~SocketClient();

	void SendPacketAsync(const std::shared_ptr<Packet>&);
	void ConnectAsync(const std::string&, const int&);
	void ListenAsync(const std::string&, const int&);
	virtual void HandleResponse(std::string&);
};