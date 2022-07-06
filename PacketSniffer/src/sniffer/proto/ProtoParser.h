#pragma once

#include <string>
#include <filesystem>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include "ProtoMessage.h"

namespace sniffer 
{
	class ProtoParser
	{
	public:
		ProtoParser();

		/**
		 * Parsing the raw message data (`data`) with proto by messageID.
		 * If the proto dir doesn't contain the specified proto, or parsing failed - empty result will be return.
		 *
		 * If parsing successfuly, return `true`, and fill `message`:
		 *   `data` - with proto fields data
		 *   `unknown_fields` - with unknown fields
		 *   `message` - out message.
		 */
		bool Parse(uint32_t id, const std::vector<byte>& data, ProtoMessage& message);

		/**
		 * Parsing the raw message data (`data`) with proto by it's `name`.
		 * Proto name must be without `.proto` part.
		 * If the proto dir doesn't contain the specified proto, or parsing failed - empty result will be return.
		 * 
		 * If parsing successfuly, return `true`, and fill `message`:
		 *   `data` - with proto fields data
		 *   `unknown_fields` - with unknown fields
		 *   `message` - out message.
		 */
		bool Parse(const std::string& name, const std::vector<byte>& data, ProtoMessage& message);
		
		/**
		 * Get proto file name by `messageID`.
		 * Returned name don't contain `.proto` part.
		 */
		std::optional<std::string> GetName(uint32_t id);

		/**
		 * Receive the `messageID` by .proto file `name`.
		 * Name must be without `.proto` part.
		 * If name wasn't found return zero(0).
		 */
		uint16_t GetId(const std::string& name);

		/**
		 * Load the proto files from `protoDir`, and ids from `idFilePath`.
		 * Also this method saving `idFilePath`, so, you can use LoadIDsFromFile() to update ids from this file lately.
		 */
		void Load(const std::string& protoDir, const std::string& idFilePath);

		/**
		 * Load the proto files from `protoDir`, and ids from `cmdID` field in proto files.
		 */
		void Load(const std::string& protoDir);
		
		/**
		 * Load ids from specified file.
		 * Also this method saving `idFilePath`, so, you can use LoadIDsFromFile() to update ids from this file lately.
		 */
		void LoadIDsFromFile(const std::string& filepath);

		/**
		 * Load ids from last specified file.
		 * Designed for updating IDs, when file was changed, or when was used another mode to load ids.
		 * File should be specified by one of these functions calling this method:
		 *   void Load(const std::string& protoDir, const std::string& idFilePath)
		 *   void LoadIDsFromFile(const std::string& filepath)
		 * If it doesn't, method just do nothing.
		 */
		void LoadIDsFromFile();

		/**
		 * Load ids from `cmdID` field in proto files.
		 * Should be used after `Load` or `LoadProtoDir` function for correct work.
		 */
		void LoadIDsFromCmdID();

		/**
		 * Load proto files from specified directory.
		 */
		void LoadProtoDir(const std::string& dirPath);

	private:
		std::mutex _mutex;

	    std::shared_ptr<google::protobuf::compiler::Importer> m_Importer;
		std::shared_ptr<google::protobuf::DynamicMessageFactory> m_Factory;
		std::shared_ptr<google::protobuf::compiler::DiskSourceTree> m_DiskTree;

		std::unordered_map<uint16_t, std::string> m_NameMap;
		std::unordered_map<std::string, uint16_t> m_IdMap;

		std::string m_IDFilePath;
		std::string m_ProtoDirPath;

		google::protobuf::Message* ParseMessage(const std::string& name, const std::vector<byte>& data);
	};
}


