#pragma once

#include <fwpmu.h>

#define FWPM_SESSION_FLAG_NONDYNAMIC 0

namespace WinSys {
	struct WFPHelpers {
		static UINT32 HlprFwpmEngineOpen(_Out_ HANDLE* pEngineHandle,
			_In_ const UINT32 sessionFlags = FWPM_SESSION_FLAG_NONDYNAMIC);
		static UINT32 HlprFwpmEngineClose(_Inout_ HANDLE* pEngineHandle);
		static UINT32 HlprFwpmFilterCreateEnumHandle(_In_ const HANDLE engineHandle,
			_In_opt_ const FWPM_FILTER_ENUM_TEMPLATE* pEnumTemplate,
			_Out_ HANDLE* pEnumHandle);
		static UINT32 HlprFwpmFilterEnum(_In_ const HANDLE engineHandle,
			_In_ const HANDLE enumHandle, _In_ const UINT32 numEntriesRequested,
			_Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_FILTER*** pppEntries,
			_Out_ UINT32* pNumEntriesReturned);
		static UINT32 HlprFwpmFilterDestroyEnumHandle(_In_ const HANDLE engineHandle,
			_In_ const HANDLE enumHandle);
		static UINT32 HlprFwpmFilterDeleteById(_In_ const HANDLE engineHandle, _In_ const UINT64 id);
	};
};