#include "stdafx.h"
#include "FileVersionInfoHelper.h"

std::wstring FileVersionInfoHelpers::GetCompanyName(std::wstring path) {
	BYTE buffer[1 << 12];
	WCHAR* companyName = nullptr;
	CString filePath = path.c_str();
	if (filePath.Mid(1, 2) == "??")
		filePath = filePath.Mid(4);
	if (::GetFileVersionInfo(filePath, 0, sizeof(buffer), buffer)) {
		WORD* langAndCodePage;
		UINT len;
		if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
			CString text;
			text.Format(L"\\StringFileInfo\\%04x%04x\\CompanyName", langAndCodePage[0], langAndCodePage[1]);

			if (::VerQueryValue(buffer, text, (void**)&companyName, &len))
				return companyName;
		}
	}
	return L"";
}