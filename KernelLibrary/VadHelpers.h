#pragma once

struct VadHelpers abstract final {
	static NTSTATUS GetVadCount(VadData* pData,PULONG pCount);
};