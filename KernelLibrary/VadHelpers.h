#pragma once

struct VadHelpers abstract final {
	static NTSTATUS GetVadCount(VadData* pData,PULONG pCount);
	static NTSTATUS DumpsAllVadsForProcess(VadData* pData, VadInfo* pInfo);
	static ULONG64 ReadBitField(PUCHAR readAddr, BitField* pBitField);
	static ULONG64 ReadFieldValue(PUCHAR readAddr, ULONG readSize);
};