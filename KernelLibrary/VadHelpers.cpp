#include "pch.h"
#include "VadHelpers.h"
#include "Helpers.h"

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

NTSTATUS VadHelpers::DumpsAllVadsForProcess(VadData* pData, VadInfo* pInfo) {
	ULONG64 Next;
	ULONG64 VadToDump;
	ULONG64 ParentStored;
	ULONG64 First;
	ULONG64 Left;
	ULONG64 Prev;
	ULONG Flags;
	ULONG Done;
	ULONG Level = 0;
	ULONG Count = 0;
	ULONG AverageLevel = 0;
	ULONG MaxLevel = 0;
	ULONG VadFlagsPrivateMemory = 0, VadFlagsNoChange = 0;
	ULONG PhysicalMapping = 0, ImageMap = 0, NoChange = 0, LargePages = 0, MemCommit = 0,
		PrivateMemory = 0, Protection = 0;
	ULONG64 StartingVpn = 0, EndingVpn = 0, Parent = 0, LeftChild = 0, RightChild = 0;
	ULONG64 ControlArea = 0, FirstPrototypePte = 0, LastContiguousPte = 0, CommitCharge = 0;

	VadToDump = 0;
	Flags = 0;

	PEPROCESS Process = nullptr;
	NTSTATUS status = PsLookupProcessByProcessId(UlongToHandle(pData->Pid), &Process);
	if (NT_SUCCESS(status)) {
		do
		{
			VadToDump = (ULONG64)Process + pData->EprocessOffsets.VadRoot;
			if (VadToDump == 0) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			First = VadToDump;
			if (First == 0) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			PUCHAR readAddr = (PUCHAR)VadToDump + pData->VadFlagsOffsets.PrivateMemory;
			VadFlagsPrivateMemory = Helpers::ReadBitField(readAddr, &pData->VadFlagsOffsets.PrivateMemoryBitField);

			readAddr = (PUCHAR)VadToDump + pData->VadFlagsOffsets.NoChange;
			VadFlagsNoChange = Helpers::ReadBitField(readAddr, &pData->VadFlagsOffsets.NoChangeBitField);

			readAddr = (PUCHAR)VadToDump + pData->VadShortOffsets.StartingVpn;
			ULONG readSz = pData->VadShortOffsets.VpnSize;
			StartingVpn = Helpers::ReadFieldValue(readAddr, readSz);
			readAddr = (PUCHAR)VadToDump + pData->VadShortOffsets.EndingVpn;
			EndingVpn = Helpers::ReadFieldValue(readAddr, readSz);

			readAddr = (PUCHAR)VadToDump + pData->VadShortOffsets.LeftChild;
			readSz = pData->VadShortOffsets.ChildSize;
			LeftChild = Helpers::ReadFieldValue(readAddr, readSz);
			readAddr = (PUCHAR)VadToDump + pData->VadShortOffsets.RightChild;
			RightChild = Helpers::ReadFieldValue(readAddr, readSz);
			readAddr = (PUCHAR)VadToDump + pData->VadFlagsOffsets.CommitCharge;
			CommitCharge = Helpers::ReadBitField(readAddr, &pData->VadFlagsOffsets.CommitChargeBitField);

			Prev = First;

			while (LeftChild != 0) {
				Prev = First;
				First = LeftChild;
				Level += 1;
				if (Level > MaxLevel) {
					MaxLevel = Level;
				}

				readAddr = (PUCHAR)First + pData->VadShortOffsets.LeftChild;
				readSz = pData->VadShortOffsets.ChildSize;
				LeftChild = Helpers::ReadFieldValue(readAddr, readSz);
			}

			readAddr = (PUCHAR)First + pData->VadShortOffsets.StartingVpn;
			readSz = pData->VadShortOffsets.VpnSize;
			StartingVpn = Helpers::ReadFieldValue(readAddr, readSz);

		} while (false);

		ObDereferenceObject(Process);
	}

	return status;
}