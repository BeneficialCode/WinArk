#include "pch.h"
#include "VadHelpers.h"

NTSTATUS VadHelpers::GetVadCount (VadData* pData, PULONG pCount) {
	NTSTATUS status = STATUS_SUCCESS;
	ULONG nVads = 0;
	PEPROCESS Process = nullptr;
	status = PsLookupProcessByProcessId(UlongToHandle(pData->Pid), &Process);

	if (NT_SUCCESS(status)) {
		switch (pData->VadCountPos)
		{
			case VadCountPos::NumberGenericTableElements: 
			{
				PUCHAR pTable = (PUCHAR)Process + pData->EprocessOffsets.VadRoot;
				PUCHAR pNumberGenericElements = (PUCHAR)pTable + pData->TableOffsets.NumberGenericTableElements;
				PUCHAR readAddr = pNumberGenericElements;
				readAddr += (pData->TableOffsets.BitField.Position / 8);
				ULONG readSz = (pData->TableOffsets.BitField.Position % 8 + pData->TableOffsets.BitField.Size
					+ 7) / 8;
				ULONG64 readBits;
				ULONG64 bitCopy;

				RtlCopyMemory(&readBits, readAddr, readSz);

				readBits = readBits >> (pData->TableOffsets.BitField.Position % 8);
				bitCopy = ((ULONG64)1 << pData->TableOffsets.BitField.Size);
				bitCopy -= (ULONG64)1;
				readBits &= bitCopy;

				*pCount = readBits;
				break;
			}
				

			case VadCountPos::NumberOfVads:
			{
				PULONG pNumberOfVads = (PULONG)((PUCHAR)Process + pData->EprocessOffsets.NumberOfVads);
				*pCount = *pNumberOfVads;
				break;
			}
				

			case VadCountPos::VadCount:
			{
				PULONG pVadCount = (PULONG)((PUCHAR)Process + pData->EprocessOffsets.VadCount);
				*pCount = *pVadCount;
				break;
			}
				
			default:
				status = STATUS_UNKNOWN_REVISION;
				break;
		}
		ObDereferenceObject(Process);
	}

	return status;
}