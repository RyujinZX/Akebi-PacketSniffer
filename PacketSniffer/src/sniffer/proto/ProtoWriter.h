#pragma once
#include <google/protobuf/io/coded_stream.h>
#include "ProtoMessage.h"

namespace sniffer
{
	// Modified version of <google/protobuf/util/internal/proto_writer.h>
	// Many checkers was removed.
	// It's kinda unsafe implementation, designed for correct use of types.
	// It don't check proto message types, just fills fields by specified indexes and types.
	// So it fills also unknown fields as well. And skip fields with ProtoField::Default flag.
	class ProtoWriter
	{
	public:
		static bool WriteMessage(const ProtoNode& message, std::vector<byte>& buffer);

	private:
		
		static bool WriteNode(const ProtoNode& node);
		static bool WriteField(const ProtoField& field);
		static bool WriteValue(int field_number, const ProtoValue& value);
		
		struct SizeInfo 
		{
			const int pos;
			int size;
		};

		inline static std::deque<SizeInfo> m_SizeInsert;
		inline static std::string m_Buffer;
		inline static std::unique_ptr<google::protobuf::io::StringOutputStream> m_Adapter;
		inline static std::unique_ptr<google::protobuf::io::CodedOutputStream> m_Stream;

		static void BeginObject(int field_number);
		static void EndObject();

		class ProtoWriterElement
		{
		public:
			ProtoWriterElement();
			ProtoWriterElement(ProtoWriterElement* parent);
			ProtoWriterElement* parent();
			ProtoWriterElement* pop();

		private:
			std::unique_ptr<ProtoWriterElement> m_Parent;
			int m_SizeIndex;
			ProtoWriter* m_Writer;
		};

		inline static std::unique_ptr<ProtoWriterElement> m_Element;
	public:
		ProtoWriter() = delete;
		ProtoWriter(const ProtoWriter& other) = delete;
		ProtoWriter(ProtoWriter&& other) = delete;

		ProtoWriter& operator=(const ProtoWriter& other) = delete;
		ProtoWriter& operator=(ProtoWriter&& other) = delete;
	};

}