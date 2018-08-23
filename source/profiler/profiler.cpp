#include "profiler.h"
#include "utils/timing.h"
#include "communications/packets.h"
#include "communications/socketclient.h"
#include <map>

namespace Profiler {

	static SocketClient* socketClient;
	static Timewatcher startTime;

	void Initialize()
	{
		startTime = Timing::Start();

		socketClient = new SocketClient(startTime->GetNanoseconds());
		socketClient->ListenAsync("127.0.0.1", 5300);
	}

	struct ProfileData
	{
		Timewatcher m_TimeWatch;
		std::shared_ptr<Packet> m_Packet0;
		std::shared_ptr<Packet> m_Packet1;
		ProfileData(Timewatcher timewatch, std::shared_ptr<Packet> pack0, std::shared_ptr<Packet> pack1)
			: m_TimeWatch(timewatch)
			, m_Packet0(pack0)
			, m_Packet1(pack1)
		{
		}
	};

	std::map<int, std::shared_ptr<ProfileData> > profiles;
	static int mapCounter = 0;

	int StartScope(std::string scopeName)
	{
		int index = mapCounter++;
		profiles[index] = std::shared_ptr<Profiler::ProfileData>(new ProfileData(
				Timing::Start(),
				std::shared_ptr<PacketProfileScopeIn>(new PacketProfileScopeIn(
						startTime->GetNanoseconds(), scopeName.c_str())),
				std::shared_ptr<PacketProfileScopeOut>(new PacketProfileScopeOut(
						startTime->GetNanoseconds(), scopeName.c_str(), 0))
			));
		socketClient->SendPacketAsync(profiles[index]->m_Packet0);
		return index;
	}

	void EndScope(int index)
	{
		PacketProfileScopeOut* endPacket = (PacketProfileScopeOut*)profiles[index]->m_Packet1.get();
		endPacket->SetNanoseconds(profiles[index]->m_TimeWatch->GetNanoseconds());
		endPacket->SetTime(startTime->GetNanoseconds());

		socketClient->SendPacketAsync(profiles[index]->m_Packet1);
	}

	ScopeProfiler::ScopeProfiler(std::string scope)
	{
		m_ProfileId = StartScope(scope);
	}

	ScopeProfiler::~ScopeProfiler()
	{
		EndScope(m_ProfileId);
	}

	std::unique_ptr<ScopeProfiler> ProfileScope(std::string name)
	{
		return std::unique_ptr<ScopeProfiler>(new ScopeProfiler(name));
	}

	void Destroy()
	{
		delete socketClient;
		profiles.clear();
	}
}