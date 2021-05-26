#include "stdafx.h"
#include "resource.h"
#include "RegHelper.h"
#include "Internals.h"
#include "RegistryManager.h"

HANDLE RegHelper::OpenKey(const CString& path, ACCESS_MASK access) {
	HANDLE hKey = nullptr;
	if (path.Left(8) == L"REGISTRY") {
		CString fullpath = L"\\" + path;
		// native open
		OBJECT_ATTRIBUTES attr;
		UNICODE_STRING name;
		RtlInitUnicodeString(&name, fullpath);
		InitializeObjectAttributes(&attr, &name, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
		::NtOpenKey(&hKey, access, &attr);
	}
	else {
		// standard open
		CString hive, subkey;
		RegistryManager::Get().GetHiveAndPath(path, hive, subkey);
		auto node = RegistryManager::Get().GetHiveNode(hive);
		CRegKey key;
		if (ERROR_SUCCESS == key.Open(*node->GetKey(), subkey, access))
			hKey = key.Detach();

	}
	return hKey;
}
