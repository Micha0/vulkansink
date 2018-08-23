/**
	based on http://www.binarytides.com/code-a-simple-socket-client-class-in-c/
	C++ client/server example using sockets and threads
*/

#include <stdio.h> //printf
#include <string.h>    //strlen
#include <string>  //string
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <netdb.h> //hostent
#include <unistd.h>
#include <algorithm>
#include <cstring>

#include "socketclient.h"
#include "utils/log.h"
#include "utils/timing.h"

namespace {

	std::string GetPeerName(struct sockaddr_in *s)
	{
		char ipstr[INET6_ADDRSTRLEN];
		int port;

		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

		return ipstr;
	}

	std::string GetPeerName(struct sockaddr_in6 *s)
	{
		char ipstr[INET6_ADDRSTRLEN];
		int port;

		port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof(ipstr));

		return ipstr;
	}

	std::string GetPeerName(struct sockaddr_storage* addr)
	{
		if (addr->ss_family == AF_INET) {
			return GetPeerName((struct sockaddr_in *)&addr);
		} else { // AF_INET6
			return GetPeerName((struct sockaddr_in6 *)&addr);
		}
	}
}
/**
    TCP Client class
*/
SocketConnection::SocketConnection()
	: m_Sock(0)
	, m_Address()
{
	memset(&m_Address, 0 , sizeof(m_Address));
}

SocketClient::SocketClient(unsigned long long int time)
	: m_Thread()
	, m_Sock(-1)
	, m_IsConnected(false)
	, m_Port(0)
	, m_Address()
	, m_ThreadIsRunning(false)
	, m_ConnectionType(CONNECTION_TYPE::NONE)
	, m_StopListenRequested(false)
	, m_ListenThread()
	, m_HelloPacket(new PacketHandshake(time, *(long long int *)"Schwifty"))
{
}

SocketClient::~SocketClient()
{
	if (m_Thread.joinable())
	{
		FinishThread();

		m_Thread.join();
		m_ThreadIsRunning = false;
	}
	if (m_ListenThread.joinable())
	{
		SetStopListenRequested(true);
		m_ListenThread.join();

		for (unsigned int i = 0; i < m_Clients.size(); ++i)
		{
			close(m_Clients[i]->m_Sock);
		}
	}
	if (IsConnected())
	{
		close(m_Sock);
	}
}

/**
    Connect to a host on a certain port number
*/
bool SocketClient::Prepare()
{
	//create socket if it is not already created
	if (m_Sock == -1)
	{
		//Create socket
		m_Sock = socket(AF_INET , SOCK_STREAM , 0);
		if (m_Sock == -1)
		{
			LOGE("Could not create socket");
		}
		int optval=1;
		if (setsockopt(m_Sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0)
		{
			LOGE("Could not set SO_REUSEADDR to socket");
		}
		if (setsockopt(m_Sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval) < 0)
		{
			LOGE("Could not set SO_REUSEPORT to socket");
		}

		LOGI("Socket created");
	}
	else    {   /* OK , nothing */  }

	//setup address structure
	if (inet_addr(m_Address.c_str()) == -1)
	{
		struct hostent *he;
		struct in_addr **addr_list;

		//resolve the hostname, its not an ip address
		if ( (he = gethostbyname( m_Address.c_str() ) ) == NULL)
		{
			//gethostbyname failed
			LOGE("gethostbyname");
			LOGI("Failed to resolve hostname");

			return false;
		}

		//Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
		addr_list = (struct in_addr **) he->h_addr_list;

		for (int i = 0; addr_list[i] != NULL; i++)
		{
			//strcpy(ip , inet_ntoa(*addr_list[i]) );
			m_Server.sin_addr = *addr_list[i];

			LOGI("%s resolved to %s", m_Address.c_str(), inet_ntoa(*addr_list[i]));

			break;
		}
	}

		//plain ip address
	else
	{
		m_Server.sin_addr.s_addr = inet_addr( m_Address.c_str() );
	}

	m_Server.sin_family = AF_INET;
	m_Server.sin_port = htons(m_Port);

	return true;
}

bool SocketClient::Connect()
{
	if (m_ConnectionType != CONNECTION_TYPE::SERVER && Prepare())
	{
		//Connect to remote server
		if (connect(m_Sock, (struct sockaddr *) &m_Server, sizeof(m_Server)) < 0)
		{
			LOGE("connect failed. Error");
			return false;
		}

		LOGI("Connected");

		SetIsConnected(true);

		return true;
	}
	else
	{
		return false;
	}
}

bool SocketClient::Listen()
{
	if (m_ConnectionType != CONNECTION_TYPE::CLIENT && Prepare())
	{
		//Connect to remote server
		if (bind(m_Sock, (struct sockaddr *) &m_Server, sizeof(m_Server)) < 0)
		{
			LOGE("Bind failed. Error");
			return false;
		}

		if (listen(m_Sock, m_Port) < 0)
		{
			LOGE("Listen failed. Error");
			return false;
		}

		LOGI("Connected");

		m_ListenThread = std::thread(&SocketClient::ListenLoop, this);

		SetIsConnected(true);

		return true;
	}
	else
	{
		return false;
	}
}


void SocketClient::ListenLoop()
{
	std::shared_ptr<SocketConnection> tempClientConnection(new SocketConnection());
	while (!IsStopListenRequested())
	{
		//socklen_t socklen = sizeof(tempClientConnection->m_Address);
		LOGI("Accepting connections");
		tempClientConnection->m_Sock = accept(m_Sock, 0, 0);

		LOGI("Socket number = %d", tempClientConnection->m_Sock);
		if (tempClientConnection->m_Sock == -1)
		{
			usleep(100);
			continue;
		}

		std::string clientPeerName = GetPeerName(&tempClientConnection->m_Address);

		LOGI("Client detected: %s", clientPeerName.c_str());

		{
			const int bufferSize = m_HelloPacket->GetSize();
			unsigned char buffer[bufferSize];
			std::memset(buffer, 0, bufferSize);
			m_HelloPacket->ToBuffer(buffer);

			LOGI("Sending Handshake: %s", ToHex((char*)buffer, m_HelloPacket->GetSize()).c_str());
			if (send(tempClientConnection->m_Sock, buffer, m_HelloPacket->GetSize(), 0) < 0)
			{
				LOGE("Failed to send Handshake to client %s", clientPeerName.c_str());
			}
		}

		{
			m_MutexClients.lock();

			m_Clients.push_back(tempClientConnection);

			m_MutexClients.unlock();
		}

		tempClientConnection.reset(new SocketConnection());
	}

	LOGI("Listenloop quit");
}

bool SocketClient::IsStopListenRequested()
{
	bool result = false;
	m_MutexStopListenRequested.lock();
	result = m_StopListenRequested;
	m_MutexStopListenRequested.unlock();
	return result;
}

void SocketClient::SetStopListenRequested(bool value)
{
	m_MutexStopListenRequested.lock();
	m_StopListenRequested = value;
	m_MutexStopListenRequested.unlock();
}

bool SocketClient::SendPacket(const std::shared_ptr<Packet>& packet)
{
	const int bufferSize = packet->GetSize();
	unsigned char buffer[bufferSize];
	std::memset(buffer, 0, bufferSize);
	packet->ToBuffer(buffer);

	LOGI("Sending: %s", ToHex((char*)buffer, packet->GetSize()).c_str());

	if (m_ConnectionType == CONNECTION_TYPE::CLIENT)
	{
		if (send(m_Sock, buffer, packet->GetSize(), 0))
		{
			return false;
		}
	}
	else if (m_ConnectionType == CONNECTION_TYPE::SERVER)
	{
		m_MutexClients.lock();

		std::vector<SocketConnection*> deletes;
		for (unsigned int i = 0; i < m_Clients.size(); ++i)
		{
			int sendResult = send(m_Clients[i]->m_Sock, buffer, packet->GetSize(), 0);
			if (sendResult < 0)
			{
				deletes.push_back(m_Clients[i].get()); //you snooze, you loose
				LOGI("Removing client %d from list: %d", i, sendResult);
			}
//			else
//			{
//				LOGI("Packet sent to client %d", i);
//			}
		}

		if (deletes.size() > 0)
		{
			LOGI("Going to delete %zu clients", deletes.size());
			m_Clients.erase(
				std::remove_if(
					m_Clients.begin(), m_Clients.end(),
					[deletes](const std::shared_ptr<SocketConnection> client)
					{
						return std::find_if(
							deletes.begin(), deletes.end(),
							[client](const SocketConnection* deleteThis)
							{
								return deleteThis == client.get();
							}
						) != deletes.end();
					}
				),
				m_Clients.end()
			);
		}

		m_MutexClients.unlock();
	}

	return true;
}

/**
    Receive data from the connected host
*/
SocketClient::RESPONSE_STATUS SocketClient::Receive(int size=512)
{
	char buffer[size];
	std::string reply;

	//Receive a reply from the server
	if (recv(m_Sock, buffer, sizeof(buffer), 0) < 0)
	{
		return RESPONSE_STATUS::ERROR;
	}

	reply = buffer;

	HandleResponse(reply);
	//TODO: error handling with the HandleResponse function would be nice ..

	return RESPONSE_STATUS::OK;
}

void SocketClient::HandleResponse(std::string &)
{
/* do nothing*/
}

void SocketClient::SetIsConnected(bool value)
{
	m_MutexIsConnected.lock();

	m_IsConnected = value;

	m_MutexIsConnected.unlock();
}

bool SocketClient::IsConnected()
{
	bool isConnected = false;
	m_MutexIsConnected.lock();

	isConnected = m_IsConnected;

	m_MutexIsConnected.unlock();
	return isConnected;
}

void SocketClient::AddToQueue(const std::shared_ptr<Packet>& packet)
{
	m_MutexQueue.lock();

	m_Queue.push(packet);

//	LOGI("Added packet to queue (%d)", m_Queue.size());

	m_MutexQueue.unlock();
}

void SocketClient::RunThread(CONNECTION_TYPE runAs) {
	if (!IsConnected())
	{
		if (runAs == CONNECTION_TYPE::CLIENT)
		{
			LOGI("Connect as client");
			if (!Connect())
			{
				FinishThread();
				return;
			}
		}
		else if (runAs == CONNECTION_TYPE::SERVER)
		{
			LOGI("Listen as server");
			if (!Listen())
			{
				FinishThread();
				return;
			}
		}
	}

	while(m_ThreadIsRunning)
	{
		if (m_Queue.size() > 0)
		{
			m_MutexQueue.lock();

			std::shared_ptr<Packet> &packet = m_Queue.front();
			m_Queue.pop();

			m_MutexQueue.unlock();

			SendPacket(packet);
		}
		else
		{
			usleep(10);
		}
	}

	FinishThread();
}

void SocketClient::FinishThread()
{
	m_MutexThreadIsRunning.lock();
	m_ThreadIsRunning = false;
	m_MutexThreadIsRunning.unlock();

	LOGI("Thread finished");
}

void SocketClient::SendPacketAsync(const std::shared_ptr<Packet> &packet)
{
	if (m_ConnectionType == CONNECTION_TYPE::NONE)
	{
		return;
	}

	AddToQueue(packet);

	m_MutexThreadIsRunning.lock();
	if (!m_ThreadIsRunning)
	{
		m_MutexQueue.lock();
		LOGI("Recreating the thread: queue size: %u", m_Queue.size());
		m_MutexQueue.unlock();

		m_Thread = std::thread(&SocketClient::RunThread, this, m_ConnectionType);
		m_ThreadIsRunning = true;
	}
	m_MutexThreadIsRunning.unlock();
}

void SocketClient::ConnectAsync(const std::string& address, const int& port)
{
	if (m_ConnectionType == CONNECTION_TYPE::NONE)
	{
		m_MutexAddressAndPort.lock();

		m_Address = address;
		m_Port = port;
		m_ConnectionType = CONNECTION_TYPE::CLIENT;

		m_MutexAddressAndPort.unlock();
	}
	else
	{
		LOGE("Can't ConnectAsync: already set a connection type!");
	}
}

void SocketClient::ListenAsync(const std::string& address, const int& port)
{
	if (m_ConnectionType == CONNECTION_TYPE::NONE)
	{
		m_MutexAddressAndPort.lock();

		m_Address = address;
		m_Port = port;
		m_ConnectionType = CONNECTION_TYPE::SERVER;

		m_MutexAddressAndPort.unlock();
	}
	else
	{
		LOGE("Can't ListenAsync: already set a connection type!");
	}
}
