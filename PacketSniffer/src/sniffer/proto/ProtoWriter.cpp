#include "pch.h"
#include "ProtoWriter.h"

#include <google/protobuf/wire_format_lite.h>
#include <memory>

using namespace google::protobuf;
using namespace google::protobuf::internal;
namespace sniffer
{

	bool ProtoWriter::WriteMessage(const ProtoNode& message, std::vector<byte>& buffer)
	{
		m_Buffer.clear();
		m_Element = std::make_unique<ProtoWriterElement>();
		m_Adapter = std::make_unique<io::StringOutputStream>(&m_Buffer);
		m_Stream = std::make_unique<io::CodedOutputStream>(m_Adapter.get());

		if (!WriteNode(message))
		{
			m_Stream.reset();
			return false;
		}
		
		m_Stream.reset();

		int curr_pos = 0;
		const void* data;
		int length;
		std::string output_buffer;
		strings::StringByteSink output = strings::StringByteSink(&output_buffer);
		io::ArrayInputStream input_stream(m_Buffer.data(), m_Buffer.size());
		while (input_stream.Next(&data, &length)) {
			if (length == 0) continue;
			int num_bytes = length;
			// Write up to where we need to insert the size field.
			// The number of bytes we may write is the smaller of:
			//   - the current fragment size
			//   - the distance to the next position where a size field needs to be
			//     inserted.
			if (!m_SizeInsert.empty() &&
				m_SizeInsert.front().pos - curr_pos < num_bytes) {
				num_bytes = m_SizeInsert.front().pos - curr_pos;
			}
			output.Append(static_cast<const char*>(data), num_bytes);
			if (num_bytes < length) {
				input_stream.BackUp(length - num_bytes);
			}
			curr_pos += num_bytes;
			// Insert the size field.
			//   size_insert_.front():      the next <index, size> pair to be written.
			//   size_insert_.front().pos:  position of the size field.
			//   size_insert_.front().size: the size (integer) to be inserted.
			if (!m_SizeInsert.empty() && curr_pos == m_SizeInsert.front().pos) {
				// Varint32 occupies at most 10 bytes.
				uint8_t insert_buffer[10];
				uint8_t* insert_buffer_pos = io::CodedOutputStream::WriteVarint32ToArray(
					m_SizeInsert.front().size, insert_buffer);
				output.Append(reinterpret_cast<const char*>(insert_buffer),
					insert_buffer_pos - insert_buffer);
				m_SizeInsert.pop_front();
			}
		}
		buffer = std::vector<byte>(output_buffer.begin(), output_buffer.end());
		return true;
	}

	bool ProtoWriter::WriteNode(const ProtoNode& node)
	{
		for (auto& field : node.fields())
		{
			if (!WriteField(field))
				return false;
		}
		return true;
	}

	bool ProtoWriter::WriteField(const ProtoField& field)
	{
		if (field.has_flag(ProtoField::Flag::Unsetted) || field.has_flag(ProtoField::Flag::Custom))
			return true;

		return WriteValue(field.number(), field.value());
	}

	static bool NodeHasSettedFields(const ProtoNode& node)
	{
		for (auto& field : node.fields())
		{
			if (!field.has_flag(ProtoField::Flag::Unsetted) && !field.has_flag(ProtoField::Flag::Custom))
				return true;
		}
		return false;
	}

	bool ProtoWriter::WriteValue(int field_number, const ProtoValue& field_value)
	{
		switch (field_value.type())
		{
		case ProtoValue::Type::Bool:
			WireFormatLite::WriteBool(field_number, field_value.to_bool(), m_Stream.get());
			return true;
		case ProtoValue::Type::Int32:
			WireFormatLite::WriteInt32(field_number, field_value.to_integer32(), m_Stream.get());
			return true;
		case ProtoValue::Type::Int64:
			WireFormatLite::WriteInt64(field_number, field_value.to_integer64(), m_Stream.get());
			return true;
		case ProtoValue::Type::UInt32:
			WireFormatLite::WriteUInt32(field_number, field_value.to_unsigned32(), m_Stream.get());
			return true;
		case ProtoValue::Type::UInt64:
			WireFormatLite::WriteUInt64(field_number, field_value.to_unsigned64(), m_Stream.get());
			return true;
		case ProtoValue::Type::Enum:
			WireFormatLite::WriteEnum(field_number, field_value.to_enum().value(), m_Stream.get());
			return true;
		case ProtoValue::Type::Float:
			WireFormatLite::WriteFloat(field_number, field_value.to_float(), m_Stream.get());
			return true;
		case ProtoValue::Type::Double:
			WireFormatLite::WriteDouble(field_number, field_value.to_double(), m_Stream.get());
			return true;
		case ProtoValue::Type::ByteSequence:
			WireFormatLite::WriteBytes(field_number, std::string(field_value.to_bytes().begin(), field_value.to_bytes().end()), m_Stream.get());
			return true;
		case ProtoValue::Type::String:
			WireFormatLite::WriteString(field_number, field_value.to_string(), m_Stream.get());
			return true;
		case ProtoValue::Type::List:
		{
			if (field_value.to_list().empty())
				return true;

			auto first_element_type = field_value.to_list().begin()->type();
			for (auto& element : field_value.to_list())
			{
				if (element.type() != first_element_type || !WriteValue(field_number, element))
					return false;
			}
			return true;
		}	
		case ProtoValue::Type::Map:
		{
			if (field_value.to_map().empty())
				return true;

			auto fe_it = field_value.to_map().begin();
			auto key_type = fe_it->first.type();
			auto value_type = fe_it->second.type();
			for (auto& [key, field_value] : field_value.to_map())
			{
				if (key.type() != key_type || field_value.type() != value_type)
					return false;

				// MapEntry is just message with 2 fields - key and value
				BeginObject(field_number);
				bool result = WriteValue(1, key);
				result |= WriteValue(2, field_value);
				EndObject();

				if (!result)
					return false;
			}
			return true;
		}
		case ProtoValue::Type::Node:
		{
			if (!NodeHasSettedFields(field_value.to_node()))
				return true;

			BeginObject(field_number);
			bool result = WriteNode(field_value.to_node());
			EndObject();
			return result;
		}
		default:
			return false;
		}
	}

	void ProtoWriter::BeginObject(int field_number)
	{
		m_Stream->WriteTag(WireFormatLite::MakeTag(field_number, WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
		m_Element = std::make_unique<ProtoWriterElement>(m_Element.release());
	}

	void ProtoWriter::EndObject()
	{
		m_Element.reset(m_Element->pop());
	}

	ProtoWriter::ProtoWriterElement::ProtoWriterElement()
		: m_Parent(nullptr),
		m_SizeIndex(-1)
	{ }

	ProtoWriter::ProtoWriterElement::ProtoWriterElement(ProtoWriterElement* parent)
		: m_Parent(parent),
		m_SizeIndex(ProtoWriter::m_SizeInsert.size())
	{
		int start_pos = ProtoWriter::m_Stream->ByteCount();
		SizeInfo info = { start_pos, -start_pos };
		ProtoWriter::m_SizeInsert.push_back(info);
	}

	ProtoWriter::ProtoWriterElement* ProtoWriter::ProtoWriterElement::parent()
	{
		return m_Parent.get();
	}

	ProtoWriter::ProtoWriterElement* ProtoWriter::ProtoWriterElement::pop()
	{
		ProtoWriter::m_SizeInsert[m_SizeIndex].size += ProtoWriter::m_Stream->ByteCount();
		int size = ProtoWriter::m_SizeInsert[m_SizeIndex].size;
		int length = io::CodedOutputStream::VarintSize32(size);
		for (ProtoWriterElement* e = parent(); e->parent() != nullptr; e = e->parent()) 
		{
			ProtoWriter::m_SizeInsert[e->m_SizeIndex].size += length;
		}
		return m_Parent.release();
	}

}