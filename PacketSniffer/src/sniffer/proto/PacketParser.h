#pragma once
#include "ProtoParser.h"
#include <sniffer/packet/RawPacketData.h>

namespace sniffer
{
	struct PacketParseResult
	{
		bool success;

		ProtoMessage head;
		ProtoMessage content;
	};

	struct UnionPacketParseResult
	{
		uint32_t mid;
		ProtoField* unpackedField;
		std::vector<uint8_t> rawContent;
		ProtoMessage content;
	};

	struct UnpackedFieldInfo
	{
		ProtoValue* list;
		uint32_t index;

		ProtoField* binaryField;
		ProtoField* unpackedField;
	};

	class PacketParser
	{
	public:
		/**
		 * Initiating parser.
		 * protoDirPath - path to directory that contains proto files.
		 * protoIDPath - path to file that contains name->id map
		 * CmdIDMode - determine is the ids will find by file (false) or by `CmdID` field in proto file (true)
		 * 
		 * Actually, the protoIDPath can seems unnecessary when we use CmdIDMode. 
		 * But it also needs for switching mode by `SetCMDIDMode` method.
		 */
		PacketParser(const std::string& protoDirPath, const std::string& protoIDPath, bool CmdIDMode);

		/**
		 * Setting new proto directory path.
		 */
		void SetProtoDir(const std::string& protoDir);

		/**
		 * Setting new proto ID path. If before used `CmdIDMode` it will automatically switched.
		 */
		void SetProtoIDPath(const std::string& protoIDPath);

		/**
		 * Changing search ID mode.
		 * If enabled - IDs will be find by `CmdID` field in proto files.
		 * Otherwise, the IDs will be find by file what was specified in `SetProtoIDPath` or in constructor.
		 */
		void SetCMDIDMode(bool enable);

		/**
		 * Parsing proto from packet data.
		 * Resulting the messages 
		 * Returns true when parse was a success.
		 */
		PacketParseResult Parse(const packet::RawPacketData& data);

		/**
		 * Return is the data contains the packet packets in itself.
		 * 
		 * Example of union packets: `UnionCmdNotify`, `CombatInvocationsNotify`, ...
		 */
		bool IsUnionPacket(const uint32_t messageID);

		/**
		 * Parsing the union packet. See `IsUnionPacket` info.
		 * Writing the unpacked data inside `content`.
		 * Returns all packed packets.
		 */
		std::vector<UnionPacketParseResult> ParseUnionPacket(ProtoMessage& content, const uint32_t messageID, bool recursive = true);

		std::vector<UnpackedFieldInfo> GetUnpackedFields(ProtoNode& content);
	private:
		static std::string GetUnpackedFieldName(const std::string& fieldName);
		std::vector<UnionPacketParseResult> ParseUnionCmdNotify(ProtoMessage& data, bool recursive);

		std::optional<UnionPacketParseResult> ParseAbilityInvokeEntry(ProtoMessage& parent, ProtoNode& entry);
		std::vector<UnionPacketParseResult> ParseAbilityInvocationsNotify(ProtoMessage& data, bool recursive);

		std::optional<UnionPacketParseResult> ParseCombatInvokeEntry(ProtoMessage& parent, ProtoNode& entry);
		std::vector<UnionPacketParseResult> ParseCombatInvocationsNotify(ProtoMessage& data, bool recursive);

		using UnionPacketParserFunc = std::vector<UnionPacketParseResult>(PacketParser::*)(ProtoMessage& data, bool recursive);
		inline static std::map<std::string, UnionPacketParserFunc> s_UnionPacketNames =
		{
			{ "UnionCmdNotify", &ParseUnionCmdNotify},

			{ "EntityAbilityInvokeEntry",      &ParseAbilityInvocationsNotify },
			{ "ClientAbilityInitFinishNotify", &ParseAbilityInvocationsNotify },
			{ "ClientAbilityChangeNotify",     &ParseAbilityInvocationsNotify },
			{ "AbilityInvocationsNotify",      &ParseAbilityInvocationsNotify },

			{ "CombatInvocationsNotify", &ParseCombatInvocationsNotify}
		};

		inline static std::unordered_map<std::string, std::vector<std::string>> s_UnionPacketFieldNames
		{
			{
				{ "UnionCmdNotify", { "cmd_list", "#", "body" } },
				{ "EntityAbilityInvokeEntry", { "invokes", "#", "ability_data" } },
				{ "ClientAbilityInitFinishNotify", { "invokes", "#", "ability_data"} },
				{ "ClientAbilityChangeNotify", {  "invokes", "#", "ability_data"} },
				{ "AbilityInvocationsNotify", { "invokes", "#", "ability_data"} },
				{ "CombatInvocationsNotify", { "invoke_list", "#", "combat_data"} }
			}
		};

		sniffer::ProtoParser m_ProtoParser;
		std::map<uint32_t, UnionPacketParserFunc> m_UnionPacketIds;

		void UpdateUnionPacketIDs();
	};
}


