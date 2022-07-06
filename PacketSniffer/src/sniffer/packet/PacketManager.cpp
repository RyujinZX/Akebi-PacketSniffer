#include "pch.h"
#include "PacketManager.h"
#include <sniffer/Config.h>
#include <sniffer/proto/ProtoWriter.h>
#include <sniffer/filter/FilterManager.h>

namespace sniffer::packet
{

	bool ModifySelectorFilter(const script::Script* script, std::string* message);

	void PacketManager::Run()
	{
		auto& config = sniffer::Config::instance();
		config.f_ProtoDirPath.FieldChangedEvent += FUNCTION_HANDLER(OnProtoDirChanged);
		config.f_ProtoIDsPath.FieldChangedEvent += FUNCTION_HANDLER(OnProtoIDPathChanged);
		config.f_ProtoIDMode.FieldChangedEvent += FUNCTION_HANDLER(OnProtoIDModeChanged);

		f_FavoritePackets = config::CreateField<decltype(f_FavoritePackets)::_ValueType>("FavoritePackets", 
			"favorite_packets", "sniffer::manager", false, 
			decltype(f_FavoritePackets)::_ValueType{});

		f_ModifySelectorProfiler = config::CreateField<Profiler<script::ScriptSelector>>(
			"ModifySelectorProfiler", "modify_selector_profiler", "sniffer::manager", true, 
			Profiler<script::ScriptSelector>("default", script::ScriptSelector()));

		for (auto& [name, selector] : f_ModifySelectorProfiler.value().profiles())
			selector.SetFilter(ModifySelectorFilter);
		
		f_ModifySelectorProfiler.value().ChangedEvent += FUNCTION_HANDLER(PacketManager::OnModifyProfilerChanged);
		f_ModifySelectorProfiler.value().NewProfileEvent += FUNCTION_HANDLER(PacketManager::OnModifyProfileAdded);
		for (auto& [name, selector] :f_ModifySelectorProfiler.value().profiles())
			selector.SelectChangedEvent += FUNCTION_HANDLER(PacketManager::OnScriptSelectChanged);

		s_Parser = new sniffer::PacketParser(config.f_ProtoDirPath, config.f_ProtoIDsPath,
			config.f_ProtoIDMode.value() == Config::ProtoIDMode::InCMDIDField);
		s_NetworkThread = new std::thread(PacketManager::NetworkLoop);
	}

	void PacketManager::Clear()
	{
		s_ChildMap.clear();
		s_UnpackedMap.clear();
		s_PacketMap.clear();
		s_RelatedPackets.clear();
		s_Packets.clear();
	}

	std::list<Packet> const& PacketManager::GetPackets()
	{
		return s_Packets;
	}

	std::vector<Packet*> PacketManager::GetRelatedPackets(uint32_t sequenceID)
	{
		auto it = s_RelatedPackets.find(sequenceID);
		if (it == s_RelatedPackets.end())
			return {};

		return it->second;
	}

	std::vector<Packet*> PacketManager::GetRelatedPackets(const Packet* packet)
	{
		if (!packet->head().has("client_sequence_id"))
			return {};

		auto& client_sequence_id_value = packet->head().field_at("client_sequence_id").value();
		if (!client_sequence_id_value.is_unsigned32())
			return {};

		auto& clientSeqID = client_sequence_id_value.to_unsigned32();

		return GetRelatedPackets(clientSeqID);
	}

	const Packet* PacketManager::GetPacket(uint64_t uniqueID)
	{
		if (s_PacketMap.count(uniqueID) == 0)
			return nullptr;

		return s_PacketMap[uniqueID];
	}

	void PacketManager::FillRelativities(Packet& packet)
	{
		if (packet.content().has_flag(ProtoMessage::Flag::IsUnpacked))
			return;

		if (!packet.head().has("client_sequence_id"))
			return;

		auto& client_sequence_id_value = packet.head().field_at("client_sequence_id").value();
		if (!client_sequence_id_value.is_unsigned32())
			return;

		auto& client_sequence_id = client_sequence_id_value.to_unsigned32();
		if (client_sequence_id == 0)
			return;

		s_RelatedPackets[client_sequence_id].push_back(&packet);
	}

	void PacketManager::OnProtoDirChanged(std::string& protoDir)
	{
		s_Parser->SetProtoDir(protoDir);
	}

	void PacketManager::OnProtoIDPathChanged(std::string& protoIDPath)
	{
		s_Parser->SetProtoIDPath(protoIDPath);
	}

	void PacketManager::OnProtoIDModeChanged(config::Enum<Config::ProtoIDMode>& mode)
	{
		s_Parser->SetCMDIDMode(mode.value() == Config::ProtoIDMode::InCMDIDField);
	}

	void PacketManager::NetworkLoop()
	{
		while (true)
		{
			auto receivedData = s_Handler->Receive();
			s_ReceiveQueue.push(std::move(receivedData));
			if (s_Handler->IsModifyingEnabled())
			{
				while (s_ModifyResponse.load() == nullptr)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}

				assert(s_ReceiveQueue.size() == 0);

				auto& modifyResponse = *s_ModifyResponse.load();
				s_Handler->Response(modifyResponse.second, modifyResponse.first);

				delete s_ModifyResponse.load();
				s_ModifyResponse.store(nullptr);
			}
		}
	}

	void PacketManager::SetHandler(IPacketHandler* handler)
	{
		s_Handler = handler;
	}

	void PacketManager::Update()
	{
		if (s_ReceiveQueue.size() > 0)
		{
			auto rawData = s_ReceiveQueue.pop();
			if (rawData)
				ProcessRawData(std::move(*rawData));
		}
	}

	void PacketManager::ProcessRawData(RawPacketData&& raw)
	{
		auto result = s_Parser->Parse(raw);
		if (!result.success)
		{
			if (s_Handler->IsModifyingEnabled())
				s_ModifyResponse.store(new std::pair<ModifyType, RawPacketData>(ModifyType::Unchanged, std::move(raw)));
			return;
		}

		auto newPacket = Packet(std::move(raw), std::move(result.content),
			std::move(result.head), GenerateUniqueID());

		auto& config = Config::instance();
		if (config.f_PacketLevelFilter && !filter::FilterManager::Execute(newPacket))
		{
			if (s_Handler->IsModifyingEnabled())
				s_ModifyResponse.store(new std::pair<ModifyType, RawPacketData>(ModifyType::Unchanged, std::move(newPacket.raw())));

			return;
		}

		auto& newPacketRef = s_Packets.emplace_back(std::move(newPacket));
		ProcessPacket(newPacketRef);
	}

	void PacketManager::ProcessPacket(Packet& packet)
	{
		s_PacketMap[packet.uid()] = &packet;
		FillRelativities(packet);
		s_PacketReceiveEvent(&packet, 0);
		FillNestedMessages(packet);

		if (!packet.content().has_flag(ProtoMessage::Flag::IsUnpacked))
			ProcessModify(packet);
	}

	void PacketManager::ProcessModify(Packet& packet)
	{
		if (!s_Handler->IsModifyingEnabled())
			return;

		auto result = ExecuteModifyScripts(&packet);
		s_PacketModifiedEvent(&packet, result.first, result.second);
		s_ModifyResponse.store(new std::pair<ModifyType, RawPacketData>(std::move(result)));
	}

	void PacketManager::FillNestedMessages(Packet& packet)
	{
		if (!s_Parser->IsUnionPacket(packet.mid()))
			return;

		auto results = s_Parser->ParseUnionPacket(packet.content(), packet.mid(), false);
		
		std::vector<uint64_t> childs;
		for (auto& nested : results)
		{
			auto newRawData = RawPacketData();
			newRawData.direction = packet.direction();
			newRawData.messageID = nested.mid;
			newRawData.content = std::move(nested.rawContent);
			newRawData.timestamp = packet.timestamp();

			auto newPacket = Packet(std::move(newRawData),
				std::move(nested.content), ProtoMessage(), GenerateUniqueID());

			auto& config = Config::instance();
			if (config.f_PacketLevelFilter && !filter::FilterManager::Execute(newPacket))
				continue;

			auto& nestedPacket = s_Packets.emplace_back(std::move(newPacket));

			childs.emplace_back(nestedPacket.uid());
			s_UnpackedMap[nestedPacket.uid()] = { packet.uid() };

			nested.unpackedField->value().to_node().emplace_field(-1, "__nested_id", childs.size() - 1);

			ProcessPacket(nestedPacket);
		}

		s_ChildMap[packet.uid()] = std::move(childs);
	}

	std::pair<sniffer::packet::ModifyType, sniffer::packet::RawPacketData> PacketManager::ExecuteModifyScripts(const Packet* packet)
	{
		ModifyType modifyType = ModifyType::Unchanged;
		Packet* modifiedPacket = nullptr;
		auto& scripts = f_ModifySelectorProfiler.value().current().GetSelectedScripts();
		for (auto& script : scripts)
		{
			bool result = script->GetLua().CallModifyFunction("on_modify", packet, modifiedPacket, modifyType);
			if (!result)
				continue;

			if (modifyType != ModifyType::Unchanged)
				break;
		}

		auto childs = GetPacketChildren(packet->uid());
		if (modifyType == ModifyType::Blocked || 
			(childs.size() == 0 && modifiedPacket == nullptr && modifyType != ModifyType::Modified))
		{
			if (modifiedPacket != nullptr)
				delete modifiedPacket;

			return std::make_pair(modifyType, packet->raw());
		}

		if (modifiedPacket == nullptr)
			modifiedPacket = new Packet(*packet);

		PackNestedFields(&modifiedPacket->content());

		auto unpackedInfos = s_Parser->GetUnpackedFields(modifiedPacket->content());

		std::vector<std::pair<ProtoValue::list_type&, ProtoValue::list_type::iterator>> toRemove;
		for (auto& info : unpackedInfos)
		{
			if (!info.unpackedField->value().is_node())
				continue;

			auto& unpackedNode = info.unpackedField->value().to_node();
			if (!unpackedNode.has("__nested_id"))
				continue;

			auto& index = unpackedNode.field_at("__nested_id").value().to_unsigned64();
			assert(childs.size() > index);

			auto& nestedPacket = childs[index];
			auto [nestedModifyType, nestedRaw] = ExecuteModifyScripts(nestedPacket);
			s_PacketModifiedEvent(nestedPacket, nestedModifyType, nestedRaw);

			if (nestedModifyType == ModifyType::Unchanged)
				continue;

			if (nestedModifyType == ModifyType::Blocked)
			{
				if (info.list == nullptr)
					continue;

				modifyType = ModifyType::Modified;

				auto& list = info.list->to_list();
				auto it = list.begin();
				std::advance(it, info.index);
				toRemove.emplace_back(list, it);
				continue;
			}

			modifyType = ModifyType::Modified;
			info.binaryField->value().to_bytes() = nestedRaw.content;
		}

		if (modifyType == ModifyType::Unchanged)
		{
			delete modifiedPacket;
			return std::make_pair(modifyType, packet->raw());
		}

		for (auto& [list, it] : toRemove)
		{
			list.erase(it);
		}

		RawPacketData rawData = {};
		rawData.direction = modifiedPacket->direction();
		rawData.messageID = modifiedPacket->mid();
		rawData.timestamp = modifiedPacket->timestamp();

		ProtoWriter::WriteMessage(modifiedPacket->head(), rawData.head);
		ProtoWriter::WriteMessage(modifiedPacket->content(), rawData.content);

		delete modifiedPacket;

		if (rawData.content.empty())
			modifyType = ModifyType::Blocked;

		return std::make_pair(modifyType, rawData);
	}

	void PacketManager::PackNestedFields(ProtoNode* node)
	{
		auto unpackedInfos = s_Parser->GetUnpackedFields(*node);
		for (auto& info : unpackedInfos)
		{
			if (!info.unpackedField->value().is_node())
				continue;

			auto& unpackedNode = info.unpackedField->value().to_node();
			PackNestedFields(&unpackedNode);

			auto& encodedValue = info.binaryField->value().to_bytes();
			ProtoWriter::WriteMessage(unpackedNode, encodedValue);
		}
	}

	bool PacketManager::IsConnected()
	{
		return s_Handler->IsConnected();
	}

	uint64_t PacketManager::GenerateUniqueID()
	{
		static uint64_t sequence_global_id = 0;
		sequence_global_id += 1;
		return sequence_global_id;
	}

	Profiler<script::ScriptSelector>& PacketManager::GetModifySelectorProfiler()
	{
		return f_ModifySelectorProfiler.value();
	}

	std::vector<const Packet*> PacketManager::GetPacketChildren(const Packet* packedPacket)
	{
		return GetPacketChildren(packedPacket->uid());
	}

	std::vector<const Packet*> PacketManager::GetPacketChildren(uint64_t uniqueID)
	{
		auto it = s_ChildMap.find(uniqueID);
		if (it == s_ChildMap.end())
			return {};

		std::vector<const Packet*> packets;
		for (auto& uid : it->second)
			packets.push_back(s_PacketMap.at(uid));
		
		return packets;
	}

	const sniffer::packet::Packet* PacketManager::GetParentPacket(uint64_t uniqueID)
	{
		auto it = s_UnpackedMap.find(uniqueID);
		if (it == s_UnpackedMap.end())
			return nullptr;

		return s_PacketMap.at(it->second);
	}

	const sniffer::packet::Packet* PacketManager::GetParentPacket(const Packet* packedPacket)
	{
		return GetParentPacket(packedPacket->uid());
	}

	void PacketManager::OnModifyProfilerChanged(Profiler<script::ScriptSelector>* profiler)
	{
		f_ModifySelectorProfiler.FireChanged();
	}

	bool ModifySelectorFilter(const script::Script* script, std::string* message)
	{
		if (!script->GetLua().HasFunction("on_modify"))
		{
			if (message != nullptr)
				*message = "Script must have `on_modify` function.";

			return false;
		}
		return true;
	}

	void PacketManager::OnModifyProfileAdded(Profiler<script::ScriptSelector>* profiler, const std::string& name, script::ScriptSelector& selector)
	{
		selector.SelectChangedEvent += FUNCTION_HANDLER(PacketManager::OnScriptSelectChanged);
		selector.SetFilter(ModifySelectorFilter);
	}

	void PacketManager::OnScriptSelectChanged(script::Script* script, bool enabled)
	{
		f_ModifySelectorProfiler.FireChanged();
	}

	const std::list<sniffer::packet::Packet>& PacketManager::GetFavoritePackets()
	{
		return f_FavoritePackets.value();
	}

	Packet& PacketManager::EmplaceFavoritePacket(const Packet& packet)
	{
		auto& newElement = f_FavoritePackets.value().emplace_back(packet);
		f_FavoritePackets.FireChanged();
		s_FavoritePacketAdded(&newElement);

		return newElement;
	}

	void PacketManager::RemoveFavoritePacket(const Packet& packet)
	{
		auto it =
			std::find_if(f_FavoritePackets.value().begin(), f_FavoritePackets.value().end(),
				[&packet](const Packet& element) { return packet == element; });

		RemoveFavoritePacket(it);
	}

	void PacketManager::RemoveFavoritePacket(const Packet* packet)
	{
		auto it = 
			std::find_if(f_FavoritePackets.value().begin(), f_FavoritePackets.value().end(), 
				[packet](const Packet& element) { return packet == &element; });

		RemoveFavoritePacket(it);
	}

	void PacketManager::RemoveFavoritePacket(const std::list<Packet>::iterator& it)
	{
		if (it == f_FavoritePackets.value().end())
			return;

		s_FavoritePacketPreRemove(&*it);
		f_FavoritePackets.value().erase(it);

		f_FavoritePackets.FireChanged();
	}

	void PacketManager::ClearFavoritePackets()
	{
		for (auto& packet : f_FavoritePackets.value())
			s_FavoritePacketPreRemove(&packet);

		f_FavoritePackets.value().clear();
		f_FavoritePackets.FireChanged();
	}

	size_t PacketManager::GetPacketCount()
	{
		return s_Packets.size();
	}

	// To move
	/*
		static ImColor _colorPallete[] = {
		ImColor(72, 86, 150),
		ImColor(231, 231, 231),
		ImColor(249, 199, 132),
		ImColor(252, 122, 30),
		ImColor(242, 76, 0),
		ImColor(136, 73, 143),
		ImColor(119, 159, 161),
		ImColor(224, 203, 168),
		ImColor(255, 101, 66),
		ImColor(86, 65, 84),
		ImColor(205, 247, 246),
		ImColor(143, 184, 222),
		ImColor(154, 148, 188),
		ImColor(155, 80, 148),
		ImColor(234, 140, 85)
	};

	void PacketManager::FillRelativities(PacketData& packetData)
	{
		if (packetData.content.has_flag(ProtoMessage::Flag::IsUnpacked))
			return;

		if (!packetData.header.has("client_sequence_id"))
			return;

		auto& client_sequence_id_value = packetData.header.field_at("client_sequence_id").value();
		if (!client_sequence_id_value.is_unsigned32())
			return;

		auto& client_sequence_id = client_sequence_id_value.to_unsigned32();
		if (client_sequence_id == 0)
			return;

		auto it = s_RelatedPackets.find(client_sequence_id);
		if (it == s_RelatedPackets.end())
		{
			s_RelatedPackets[client_sequence_id].push_back(&packetData);
			return;
		}

		auto& related_packets = it->second;
		auto& first_packet = *related_packets.begin();

		if (first_packet->timestamp + 60000 < packetData.timestamp)
		{
			related_packets.clear();
			related_packets.push_back(&packetData);
			return;
		}

		ImColor color = _colorPallete[client_sequence_id % (sizeof(_colorPallete) / sizeof(_colorPallete[0]))];

		if (related_packets.size() == 1)
		{
			auto request_flag = PacketFlag("Req", fmt::format("Have a linked response with sequence id {}", packetData.sequenceID), color,
				{
					{ "rel", packetData.sequenceID }
				});
			first_packet->AddFlag(request_flag);

			auto responce_flag = PacketFlag("Rsp", fmt::format("Have a linked request with sequence id {}", first_packet->sequenceID), color,
				{
					{ "rel", first_packet->sequenceID }
				});
			packetData.AddFlag(responce_flag);
		}
		else
		{
			std::vector<int64_t> related_packets_ids;
			for (auto& packet : related_packets)
			{
				related_packets_ids.push_back(packet->sequenceID);
			}
			related_packets_ids.push_back(packetData.sequenceID);

			auto related_flag = PacketFlag("Rel", fmt::format("Have several packet with the same sequence id."), color,
				{
					{ "rel", std::move(related_packets_ids) }
				}
			);

			bool need_change_flag = related_packets.size() == 2;
			if (need_change_flag)
			{
				auto& second_packet = related_packets[1];
				first_packet->RemoveFlag("Req");
				second_packet->RemoveFlag("Rsp");
				first_packet->AddFlag(related_flag);
				second_packet->AddFlag(related_flag);
			}

			for (auto& packet : related_packets)
			{
				auto flag = packet->GetFlag("Rel");
				assert(flag != nullptr);

				auto& info = flag->info();
				auto rel_it = info.find("rel");
				if (rel_it == info.end())
					continue;

				rel_it->array().push_back(packetData.sequenceID);
			}

			packetData.AddFlag(related_flag);
		}

		related_packets.push_back(&packetData);
	}

	*/
}