#pragma once

struct VadHelpers abstract final {
	static NTSTATUS GetVadCount(VadData* pData, PULONG pCount);
	static NTSTATUS DumpsAllVadsForProcess(VadData* pData, VadInfo* pInfo);
};