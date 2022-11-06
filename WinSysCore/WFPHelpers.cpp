#include "pch.h"
#include "WFPHelpers.h"
#include <mstcpip.h>

#pragma comment(lib,"Fwpuclnt.lib")

using namespace WinSys;

_Use_decl_annotations_
UINT32 WFPHelpers::HlprFwpmEngineOpen(HANDLE* pEngineHandle, const UINT32 sessionFlags){
	ASSERT(pEngineHandle);

	UINT32 status = NO_ERROR;
	FWPM_SESSION session{ 0 };

	session.displayData.name = const_cast <wchar_t*>(L"WinARK's User Mode Session");
	session.flags = sessionFlags;

	status = FwpmEngineOpen(0, RPC_C_AUTHN_WINNT, 0, &session, pEngineHandle);

	return status;
}

_Use_decl_annotations_
UINT32 WFPHelpers::HlprFwpmEngineClose(HANDLE* pEngineHandle){
	ASSERT(pEngineHandle);

	UINT32 status = NO_ERROR;

	if (*pEngineHandle) {
		status = FwpmEngineClose(*pEngineHandle);
		if (status != NOERROR)
			return status;

		*pEngineHandle = 0;
	}

	return status;
}

_Use_decl_annotations_
UINT32 WFPHelpers::HlprFwpmFilterEnum(const HANDLE engineHandle, const HANDLE enumHandle, 
	const UINT32 numEntriesRequested, FWPM_FILTER*** pppEntries, UINT32* pNumEntriesReturned){
	ASSERT(engineHandle);
	ASSERT(enumHandle);
	ASSERT(numEntriesRequested);
	ASSERT(pppEntries);
	ASSERT(pNumEntriesReturned);

	UINT32 status = NO_ERROR;

	status = FwpmFilterEnum(engineHandle,
		enumHandle,
		numEntriesRequested,
		pppEntries,
		pNumEntriesReturned);

	if (status != NO_ERROR && status != FWP_E_FILTER_NOT_FOUND
		&& status != FWP_E_NOT_FOUND) {
		*pppEntries = 0;
		*pNumEntriesReturned = 0;
		return status;
	}

	return status;
}

_Use_decl_annotations_
UINT32 WFPHelpers::HlprFwpmFilterCreateEnumHandle(const HANDLE engineHandle, 
	const FWPM_FILTER_ENUM_TEMPLATE* pEnumTemplate, HANDLE* pEnumHandle){
	ASSERT(engineHandle);
	ASSERT(pEnumHandle);

	UINT32 status = NO_ERROR;

	status = FwpmFilterCreateEnumHandle(engineHandle, pEnumTemplate,
		pEnumHandle);

	if (status != NO_ERROR) {
		*pEnumHandle = 0;
		return status;
	}

	return status;
}

_Use_decl_annotations_
UINT32 WFPHelpers::HlprFwpmFilterDestroyEnumHandle(const HANDLE engineHandle, const HANDLE enumHandle){
	ASSERT(engineHandle);
	ASSERT(enumHandle);

	UINT32 status = NO_ERROR;

	status = FwpmFilterDestroyEnumHandle(engineHandle, enumHandle);
	return status;
}

_Use_decl_annotations_
UINT32 WinSys::WFPHelpers::HlprFwpmFilterDeleteById(const HANDLE engineHandle, const UINT64 id){
	ASSERT(engineHandle);
	
	UINT32 status = NO_ERROR;
	status = FwpmFilterDeleteById(engineHandle, id);
	return status;
}

_Use_decl_annotations_
BOOLEAN WinSys::WFPHelpers::HlprFwpmLayerIsUserMode(const GUID* pLayerKey){
	ASSERT(pLayerKey);
	BOOLEAN isUserMode = FALSE;

	for (UINT32 layerIndex = 0;
		layerIndex < TOTAL_USER_MODE_LAYER_COUNT && isUserMode == FALSE;
		layerIndex++) {
		if (pLayerKey == ppUserModeLayerKeyArray[layerIndex])
			isUserMode = TRUE;
	}

	return isUserMode;
}