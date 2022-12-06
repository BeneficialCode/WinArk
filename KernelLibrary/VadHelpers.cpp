#include "pch.h"
#include "VadHelpers.h"

NTSTATUS VadHelpers::GetVadCount(VadData* pData, PULONG pCount) {
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
				PULONG pNumberGenericElements = (PULONG)((PUCHAR)pTable + pData->TableOffsets.NumberGenericTableElements);
				*pCount = *pNumberGenericElements;
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