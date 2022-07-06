#include <pch.h>
#include "PacketParser.h"

namespace sniffer
{
	PacketParser::PacketParser(const std::string& protoDirPath, const std::string& protoIDPath, bool CmdIdMode) 
	{
		m_ProtoParser.Load(protoIDPath, protoDirPath);
		
		if (CmdIdMode)
			SetCMDIDMode(CmdIdMode);
	}

	void PacketParser::SetProtoDir(const std::string& protoDir)
	{
		m_ProtoParser.LoadProtoDir(protoDir);
		UpdateUnionPacketIDs();
	}

	void PacketParser::SetProtoIDPath(const std::string& protoIDPath)
	{
		m_ProtoParser.LoadIDsFromFile(protoIDPath);
		UpdateUnionPacketIDs();
	}

	void PacketParser::SetCMDIDMode(bool enable)
	{
		if (enable)
			m_ProtoParser.LoadIDsFromCmdID();
		else
			m_ProtoParser.LoadIDsFromFile();

		UpdateUnionPacketIDs();
	}

	PacketParseResult PacketParser::Parse(const packet::RawPacketData& data)
	{
		PacketParseResult result = {};
		result.success = false;

		if (!m_ProtoParser.Parse("PacketHead", data.head, result.head))
			return result;

		if (!m_ProtoParser.Parse(data.messageID, data.content, result.content))
			return result;

		result.success = true;
		return result;
	}

	bool PacketParser::IsUnionPacket(const uint32_t messageID)
	{
		return m_UnionPacketIds.count(messageID) > 0;
	}

	std::vector<UnionPacketParseResult> PacketParser::ParseUnionPacket(ProtoMessage& content, const uint32_t messageID, bool recursive)
	{
		if (!IsUnionPacket(messageID))
			return {};

		auto parseFunction = m_UnionPacketIds[messageID];
		return (this->*parseFunction)(content, recursive);
	}

	std::vector<sniffer::UnpackedFieldInfo> PacketParser::GetUnpackedFields(ProtoNode& content)
	{
		auto it = s_UnionPacketFieldNames.find(content.type());
		if (it == s_UnionPacketFieldNames.end())
			return {};

		auto& [nodeType, fieldTrace] = *it;

		std::vector<sniffer::UnpackedFieldInfo> fieldInfos;

		std::vector<ProtoField*> currentFields;

		bool isList = false;
		size_t i = 0;
		for (auto it = fieldTrace.begin(); it != fieldTrace.end(); it++, i++)
		{
			auto& fieldName = *it;
			if (currentFields.empty())
			{
				currentFields.push_back(&content.field_at(fieldName));
				continue;
			}

			if (fieldName == "#")
			{
				isList = true;
				continue;
			}

			bool isLast = i == fieldTrace.size() - 1;
			if (isList)
			{
				std::vector<ProtoField*> newFields;
				for (auto& field : currentFields)
				{
					uint32_t i = 0;
					for (auto& value : field->value().to_list())
					{
						if (!value.to_node().has(fieldName))
							break;

						if (isLast)
						{
							if (!value.to_node().has(GetUnpackedFieldName(fieldName)))
								break;

							fieldInfos.push_back({ &field->value(), i, &value.to_node().field_at(fieldName), &value.to_node().field_at(GetUnpackedFieldName(fieldName)) });
						}
						else
							newFields.push_back(&value.to_node().field_at(fieldName));
						i++;
					}
				}

				if (newFields.empty())
					break;

				isList = false;
				currentFields = newFields;
				continue;
			}

			std::vector<ProtoField*> newFields;
			for (auto& field : currentFields)
			{
				if (!field->value().to_node().has(fieldName))
					continue;

				if (isLast)
				{
					if (!field->value().to_node().has(GetUnpackedFieldName(fieldName)))
						continue;

					fieldInfos.push_back({ nullptr, 0, &field->value().to_node().field_at(fieldName), &field->value().to_node().field_at(GetUnpackedFieldName(fieldName)) });
				}
				else
					newFields.push_back(&field->value().to_node().field_at(fieldName));
			}

		}
		return fieldInfos;
	}

	std::string PacketParser::GetUnpackedFieldName(const std::string& fieldName)
	{
		return fieldName + "_unpacked";
	}

	std::vector<UnionPacketParseResult> PacketParser::ParseUnionCmdNotify(ProtoMessage& content, bool recursive)
	{
		std::vector<UnionPacketParseResult> results;

		if (!content.has("cmd_list"))
			return results;

		auto& cmd_list = content.field_at("cmd_list").value();
		if (!cmd_list.is_list())
			return results;

		for (auto& cmd : cmd_list.to_list())
		{
			if (!cmd.is_node())
				continue;

			auto& cmd_node = cmd.to_node();
			if (!cmd_node.has("body") || !cmd_node.has("message_id"))
				continue;
			
			auto& body = cmd_node.field_at("body").value();
			auto& messageId = cmd_node.field_at("message_id").value();

			if (!body.is_bytes() || !messageId.is_unsigned32())
				continue;

			auto& nested = results.emplace_back();
			nested.mid = messageId.to_unsigned32();
			nested.rawContent = body.to_bytes();

			auto parsed = m_ProtoParser.Parse(messageId.to_unsigned32(), body.to_bytes(), nested.content);
			if (parsed && recursive && IsUnionPacket(nested.mid))
			{
				auto nestedNestedPackets = ParseUnionPacket(nested.content, nested.mid);
				results.insert(results.end(), 
					std::make_move_iterator(nestedNestedPackets.begin()), 
					std::make_move_iterator(nestedNestedPackets.end()));
			}

			nested.content.set_flag(ProtoMessage::Flag::IsUnpacked);
			nested.unpackedField = &cmd_node.emplace_field(-1, GetUnpackedFieldName("body"), static_cast<ProtoNode>(nested.content));
		}

		return results;
	}

	std::optional<UnionPacketParseResult> PacketParser::ParseAbilityInvokeEntry(ProtoMessage& parent, ProtoNode& entry)
	{
		static std::unordered_map<std::string, std::string> ability_type_to_proto = 
		{
			{ "ABILITY_META_MODIFIER_CHANGE", "AbilityMetaModifierChange" },
			{ "ABILITY_META_COMMAND_MODIFIER_CHANGE_REQUEST", "AbilityMetaCommandModifierChangeRequest" },
			{ "ABILITY_META_SPECIAL_FLOAT_ARGUMENT", "AbilityMetaSpecialFloatArgument" },
			{ "ABILITY_META_OVERRIDE_PARAM", "AbilityMetaOverrideParam" },
			{ "ABILITY_META_CLEAR_OVERRIDE_PARAM", "AbilityMetaClearOverrideParam" },
			{ "ABILITY_META_REINIT_OVERRIDEMAP", "AbilityMetaReinitOverridemap" },
			{ "ABILITY_META_GLOBAL_FLOAT_VALUE", "AbilityMetaGlobalFloatValue" },
			{ "ABILITY_META_CLEAR_GLOBAL_FLOAT_VALUE", "AbilityMetaClearGlobalFloatValue" },
			{ "ABILITY_META_ABILITY_ELEMENT_STRENGTH", "AbilityMetaAbilityElementStrength" },
			{ "ABILITY_META_ADD_OR_GET_ABILITY_AND_TRIGGER", "AbilityMetaAddOrGetAbilityAndTrigger" },
			{ "ABILITY_META_SET_KILLED_SETATE", "AbilityMetaSetKilledSetate" },
			{ "ABILITY_META_SET_ABILITY_TRIGGER", "AbilityMetaSetAbilityTrigger" },
			{ "ABILITY_META_ADD_NEW_ABILITY", "AbilityMetaAddNewAbility" },
			{ "ABILITY_META_REMOVE_ABILITY", "AbilityMetaRemoveAbility" },
			{ "ABILITY_META_SET_MODIFIER_APPLY_ENTITY", "AbilityMetaSetModifierApplyEntity" },
			{ "ABILITY_META_MODIFIER_DURABILITY_CHANGE", "AbilityMetaModifierDurabilityChange" },
			{ "ABILITY_META_ELEMENT_REACTION_VISUAL", "AbilityMetaElementReactionVisual" },
			{ "ABILITY_META_SET_POSE_PARAMETER", "AbilityMetaSetPoseParameter" },
			{ "ABILITY_META_UPDATE_BASE_REACTION_DAMAGE", "AbilityMetaUpdateBaseReactionDamage" },
			{ "ABILITY_META_TRIGGER_ELEMENT_REACTION", "AbilityMetaTriggerElementReaction" },
			{ "ABILITY_META_LOSE_HP", "AbilityMetaLoseHp" },
			{ "ABILITY_ACTION_TRIGGER_ABILITY", "AbilityActionTriggerAbility" },
			{ "ABILITY_ACTION_SET_CRASH_DAMAGE", "AbilityActionSetCrashDamage" },
			{ "ABILITY_ACTION_EFFECT", "AbilityActionEffect" },
			{ "ABILITY_ACTION_SUMMON", "AbilityActionSummon" },
			{ "ABILITY_ACTION_BLINK", "AbilityActionBlink" },
			{ "ABILITY_ACTION_CREATE_GADGET", "AbilityActionCreateGadget" },
			{ "ABILITY_ACTION_APPLY_LEVEL_MODIFIER", "AbilityActionApplyLevelModifier" },
			{ "ABILITY_ACTION_GENERATE_ELEM_BALL", "AbilityActionGenerateElemBall" },
			{ "ABILITY_ACTION_SET_RANDOM_OVERRIDE_MAP_VALUE", "AbilityActionSetRandomOverrideMapValue" },
			{ "ABILITY_ACTION_SERVER_MONSTER_LOG", "AbilityActionServerMonsterLog" },
			{ "ABILITY_ACTION_CREATE_TILE", "AbilityActionCreateTile" },
			{ "ABILITY_ACTION_DESTROY_TILE", "AbilityActionDestroyTile" },
			{ "ABILITY_ACTION_FIRE_AFTER_IMAGE", "AbilityActionFireAfterImage" },
			{ "ABILITY_MIXIN_AVATAR_STEER_BY_CAMERA", "AbilityMixinAvatarSteerByCamera" },
			{ "ABILITY_MIXIN_MONSTER_DEFEND", "AbilityMixinMonsterDefend" },
			{ "ABILITY_MIXIN_WIND_ZONE", "AbilityMixinWindZone" },
			{ "ABILITY_MIXIN_COST_STAMINA", "AbilityMixinCostStamina" },
			{ "ABILITY_MIXIN_ELITE_SHIELD", "AbilityMixinEliteShield" },
			{ "ABILITY_MIXIN_ELEMENT_SHIELD", "AbilityMixinElementShield" },
			{ "ABILITY_MIXIN_GLOBAL_SHIELD", "AbilityMixinGlobalShield" },
			{ "ABILITY_MIXIN_SHIELD_BAR", "AbilityMixinShieldBar" },
			{ "ABILITY_MIXIN_WIND_SEED_SPAWNER", "AbilityMixinWindSeedSpawner" },
			{ "ABILITY_MIXIN_DO_ACTION_BY_ELEMENT_REACTION", "AbilityMixinDoActionByElementReaction" },
			{ "ABILITY_MIXIN_FIELD_ENTITY_COUNT_CHANGE", "AbilityMixinFieldEntityCountChange" },
			{ "ABILITY_MIXIN_SCENE_PROP_SYNC", "AbilityMixinScenePropSync" },
			{ "ABILITY_MIXIN_WIDGET_MP_SUPPORT", "AbilityMixinWidgetMpSupport" }
		};

		if (!entry.has("ability_data") || !entry.has("argument_type"))
			return {};

		auto& ability_data = entry.field_at("ability_data").value();
		auto& argument_type = entry.field_at("argument_type").value();

		if (!ability_data.is_bytes() || !argument_type.is_enum())
			return {};

		auto it = ability_type_to_proto.find(argument_type.to_enum().repr());
		if (it == ability_type_to_proto.end())
			return {};

		auto& ability_proto_name = it->second;

		UnionPacketParseResult result = {};
		result.mid = 0;
		m_ProtoParser.Parse(ability_proto_name, ability_data.to_bytes(), result.content);
		result.content.set_flag(ProtoMessage::Flag::IsUnpacked);
		result.rawContent = ability_data.to_bytes();

		result.unpackedField = &entry.emplace_field(-1, GetUnpackedFieldName("ability_data"), static_cast<ProtoNode>(result.content));
		return result;
	}

	std::vector<UnionPacketParseResult> PacketParser::ParseAbilityInvocationsNotify(ProtoMessage& content, bool /*recursive*/)
	{
		std::vector<UnionPacketParseResult> results = {};
		if (!content.has("invokes"))
			return results;

		auto& invokes = content.field_at("invokes").value();
		if (!invokes.is_list())
			return results;

		for (auto& invoke_entry : invokes.to_list())
		{
			if (!invoke_entry.is_node())
				continue;

			auto ability_packet_data = ParseAbilityInvokeEntry(content, invoke_entry.to_node());
			if (ability_packet_data)
				results.push_back(std::move(*ability_packet_data));
		}
		return results;
	}

	std::optional<UnionPacketParseResult> PacketParser::ParseCombatInvokeEntry(ProtoMessage& parent, ProtoNode& entry)
	{
		static std::unordered_map<std::string, std::string> combat_type_to_proto = {
			{ "ENTITY_MOVE", "EntityMoveInfo" },
			{ "COMBAT_EVT_BEING_HIT", "EvtBeingHitInfo" },
			{ "COMBAT_ANIMATOR_STATE_CHANGED", "EvtAnimatorStateChangedInfo" },
			{ "COMBAT_FACE_TO_DIR", "EvtFaceToDirInfo" },
			{ "COMBAT_SET_ATTACK_TARGET", "EvtSetAttackTargetInfo" },
			{ "COMBAT_RUSH_MOVE", "EvtRushMoveInfo" },
			{ "COMBAT_ANIMATOR_PARAMETER_CHANGED", "EvtAnimatorParameterInfo" },
			{ "SYNC_ENTITY_POSITION", "EvtSyncEntityPositionInfo" },
			{ "COMBAT_STEER_MOTION_INFO", "EvtCombatSteerMotionInfo" },
			{ "COMBAT_FORCE_SET_POSITION_INFO", "EvtCombatForceSetPosInfo" },
			{ "COMBAT_FORCE_SET_POS_INFO", "EvtCombatForceSetPosInfo" },
			{ "COMBAT_COMPENSATE_POS_DIFF", "EvtCompensatePosDiffInfo" },
			{ "COMBAT_MONSTER_DO_BLINK", "EvtMonsterDoBlink" },
			{ "COMBAT_FIXED_RUSH_MOVE", "EvtFixedRushMove" },
			{ "COMBAT_SYNC_TRANSFORM", "EvtSyncTransform" },
			{ "COMBAT_LIGHT_CORE_MOVE", "EvtLightCoreMove" }
		};

		if (!entry.has("argument_type") || !entry.has("combat_data"))
			return {};

		auto& argument_type = entry.field_at("argument_type").value();
		auto& combat_data = entry.field_at("combat_data").value();

		if (!argument_type.is_enum() || !combat_data.is_string())
			return {};

		auto it = combat_type_to_proto.find(argument_type.to_enum().repr());
		if (it == combat_type_to_proto.end())
			return {};

		auto& combat_proto_name = it->second;

		UnionPacketParseResult result = {};
		result.mid = 0;
		m_ProtoParser.Parse(combat_proto_name, combat_data.to_bytes(), result.content);
		result.content.set_flag(ProtoMessage::Flag::IsUnpacked);
		result.rawContent = combat_data.to_bytes();

		result.unpackedField = &entry.emplace_field(-1, GetUnpackedFieldName("combat_data"), static_cast<ProtoNode>(result.content));
		return result;
	}

	std::vector<UnionPacketParseResult> PacketParser::ParseCombatInvocationsNotify(ProtoMessage& content, bool /*recursive*/)
	{
		std::vector<UnionPacketParseResult> results = {};
		if (!content.has("invoke_list"))
			return results;

		auto& invoke_list = content.field_at("invoke_list").value();
		if (!invoke_list.is_list())
			return results;

		for (auto& invoke_entry : invoke_list.to_list())
		{
		
			if (!invoke_entry.is_node())
				continue;

			auto combatPacketData = ParseCombatInvokeEntry(content, invoke_entry.to_node());
			if (combatPacketData)
				results.push_back(std::move(*combatPacketData));
		}
		return results;
	}

	void PacketParser::UpdateUnionPacketIDs()
	{
		m_UnionPacketIds.clear();
		for (auto& [unionPacketName, parserFunc] : s_UnionPacketNames)
		{
			m_UnionPacketIds[m_ProtoParser.GetId(unionPacketName)] = parserFunc;
		}
	}
}