#pragma once
#include <cheat-base/events/event.hpp>
#include <sniffer/packet/Packet.h>
#include <sniffer/packet/IPacketHandler.h>
#include <sniffer/proto/PacketParser.h>
#include <sniffer/Config.h>
#include <sniffer/Profiler.h>
#include <sniffer/script/ScriptSelector.h>

namespace sniffer::packet
{
	class PacketManager
	{
	public:
		static void Run();

		static const std::list<Packet>& GetPackets();
		static size_t GetPacketCount();

		static const Packet* GetPacket(uint64_t uniqueID);

		static std::vector<Packet*> GetRelatedPackets(const Packet* packetData);
		static std::vector<Packet*> GetRelatedPackets(uint32_t sequenceID);

		static const Packet* GetParentPacket(const Packet* packedPacket);
		static const Packet* GetParentPacket(uint64_t uniqueID);

		static std::vector<const Packet*> GetPacketChildren(const Packet* packedPacket);
		static std::vector<const Packet*> GetPacketChildren(uint64_t uniqueID);

		static const std::list<Packet>& GetFavoritePackets();
		static Packet& EmplaceFavoritePacket(const Packet& packet);
		static void RemoveFavoritePacket(const Packet& packet);
		static void RemoveFavoritePacket(const Packet* packet);
		static void ClearFavoritePackets();

		static Profiler<script::ScriptSelector>& GetModifySelectorProfiler();

		static void SetHandler(IPacketHandler* handler);
		static bool IsConnected();

		static void Update();
		static void Clear();

	private:

		static void ProcessRawData(RawPacketData&& raw);
		static void ProcessPacket(Packet& packet);

		static void FillNestedMessages(Packet& packet);
		static void FillRelativities(Packet& packet);

		static void ProcessModify(Packet& packet);
		static void PackNestedFields(ProtoNode* node);
		static std::pair<ModifyType, RawPacketData> ExecuteModifyScripts(const Packet* packet);

		static void RemoveFavoritePacket(const std::list<Packet>::iterator& it);

		static void NetworkLoop();

		static void OnScriptSelectChanged(script::Script* script, bool enabled);
		static void OnModifyProfilerChanged(Profiler<script::ScriptSelector>* profiler);
		static void OnModifyProfileAdded(Profiler<script::ScriptSelector>* profiler, const std::string& name, script::ScriptSelector& selector);

		static void OnProtoDirChanged(std::string& protoDir);
		static void OnProtoIDPathChanged(std::string& protoIDPath);
		static void OnProtoIDModeChanged(config::Enum<Config::ProtoIDMode>& mode);

		static uint64_t GenerateUniqueID();

		static inline PacketParser* s_Parser = nullptr;
		static inline IPacketHandler* s_Handler = nullptr;
		static inline std::thread* s_NetworkThread;

		static inline std::map<uint32_t, std::vector<Packet*>> s_RelatedPackets = {};
		static inline std::map<uint64_t, Packet*> s_PacketMap = {};
		static inline std::list<Packet> s_Packets = {};

		static inline config::Field<std::list<Packet>> f_FavoritePackets;
		static inline config::Field<Profiler<script::ScriptSelector>> f_ModifySelectorProfiler;

		static inline std::map<uint64_t, std::vector<uint64_t>> s_ChildMap;
		static inline std::map<uint64_t, uint64_t> s_UnpackedMap;

		static inline SafeQueue<RawPacketData> s_ReceiveQueue;
		static inline std::atomic<std::pair<ModifyType, RawPacketData>*> s_ModifyResponse;

		static inline TEvent<const Packet*, uint64_t> s_PacketReceiveEvent;
		static inline TEvent<const Packet*> s_NewSessionEvent;
		static inline TEvent<const Packet*, ModifyType, RawPacketData> s_PacketModifiedEvent;

		static inline TEvent<const Packet*> s_FavoritePacketAdded;
		static inline TEvent<const Packet*> s_FavoritePacketPreRemove;

	public:

		static inline IEvent<const Packet*, ModifyType, RawPacketData>& PacketModifiedEvent = s_PacketModifiedEvent;
		static inline IEvent<const Packet*, uint64_t>& PacketReceiveEvent = s_PacketReceiveEvent;
		static inline IEvent<const Packet*>& NewSessionEvent = s_NewSessionEvent;

		static inline IEvent<const Packet*>& FavoritePacketAdded = s_FavoritePacketAdded;
		static inline IEvent<const Packet*>& FavoritePacketPreRemove = s_FavoritePacketPreRemove;
	};
}

