#include <pch.h>
#include "PipeModifyData.h"

namespace sniffer::pipe::messages
{
	PipeModifyData::PipeModifyData() :
		modifyType(packet::ModifyType::Unchanged), head({}), content({}), messageID(0)
	{ }

	void PipeModifyData::Write(PipeTransfer* transfer)
	{
		PipeMessage::Write(transfer);

		transfer->Write(modifyType);
		if (modifyType == packet::ModifyType::Modified)
		{

			transfer->Write(messageID);
			transfer->Write(content);
			transfer->Write(head);
		}
	}

	void PipeModifyData::Read(PipeTransfer* transfer)
	{
		transfer->Read(modifyType);
		if (modifyType == packet::ModifyType::Modified)
		{

			transfer->Read(messageID);
			transfer->Read(content);
			transfer->Read(head);
		}
	}

}
