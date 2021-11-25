#include "stdafx.h"
#include "SymbolInfo.h"
#include <WinInet.h>
#include <filesystem>

#pragma comment(lib,"Wininet.lib")

SymbolInfo::SymbolInfo() {
	_dlg.ShowCancelButton(false);
}

bool SymbolInfo::SymDownloadSymbol(std::wstring localPath) {
	std::string url = "http://msdl.microsoft.com/download/symbols";

	if (url.back() != '/')
		url += '/';

	CString temp = _pdbFile + L"/" + _pdbSignature + L"/" + _pdbFile;
	std::wstring symbolUrl = temp.GetBuffer();
	url+= std::string(symbolUrl.begin(), symbolUrl.end());
	std::wstring fileName = localPath + L"\\" + _pdbFile.GetBuffer();
	bool isExist = std::filesystem::is_regular_file(fileName);
	if (isExist)
		return true;
	

	_dlg.SetMessageText(L"Starting download " + _pdbFile);

	wil::unique_handle hThread(::CreateThread(nullptr, 0, [](auto params)->DWORD {
		SymbolInfo* info = (SymbolInfo*)params;
		info->_dlg.DoModal();
		return 0;
		}, this, 0, nullptr));

	auto result = Download(url, fileName, "WinArk", 1000, 
		[](void* userdata,unsigned long long readBytes,unsigned long long totalBytes) {
			CProgressDlg* pDlg = (CProgressDlg*)userdata;
			if (totalBytes) {
				pDlg->UpdateProgress(readBytes);
			}
			return true;
		}, 
		(void*)&_dlg);
	_dlg.EndDialog(IDCANCEL);
	return result == downslib_error::ok ? true : false;
}

bool SymbolInfo::GetPdbSignature(ULONG_PTR imageBase,PIMAGE_DEBUG_DIRECTORY entry) {
	if (entry->SizeOfData < sizeof(CV_INFO_PDB20))
		return false;

	ULONG_PTR offset = 0;
	
	offset = entry->PointerToRawData;
	auto cvData = (unsigned char*)(imageBase + offset);
	auto signature = *(DWORD*)cvData;

	if (signature == '01BN') {
		auto cv = (CV_INFO_PDB20*)cvData;
		_pdbSignature.Format(L"%X%X", cv->Signature, cv->Age);
		std::string file((const char*)cv->PdbFileName, entry->SizeOfData - FIELD_OFFSET(CV_INFO_PDB20, PdbFileName));
		_pdbFile = std::wstring(file.begin(), file.end()).c_str();
		_pdbValidation.signature = cv->Signature;
		_pdbValidation.age = cv->Age;
	}
	else if (signature == 'SDSR') {
		auto cv = (CV_INFO_PDB70*)cvData;
		_pdbSignature.Format(L"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%X",
			cv->Signature.Data1, cv->Signature.Data2, cv->Signature.Data3,
			cv->Signature.Data4[0], cv->Signature.Data4[1], cv->Signature.Data4[2],
			cv->Signature.Data4[3], cv->Signature.Data4[4], cv->Signature.Data4[5],
			cv->Signature.Data4[6], cv->Signature.Data4[7], cv->Age);
		std::string file((const char*)cv->PdbFileName, entry->SizeOfData - FIELD_OFFSET(CV_INFO_PDB70, PdbFileName));
		_pdbFile = std::wstring(file.begin(), file.end()).c_str();
		memcpy(&_pdbValidation.guid, &cv->Signature, sizeof(GUID));
		_pdbValidation.signature = 0;
		_pdbValidation.age = cv->Age;
	}
	return true;
}

downslib_error SymbolInfo::Download(std::string url, std::wstring fileName, std::string userAgent, 
	unsigned int timeout,downslib_cb cb, void* userdata){
	HINTERNET hInternet = nullptr;
	HINTERNET hUrl = nullptr;
	HANDLE hFile = nullptr;

	Cleanup cleanup([&]()
	{
		DWORD lastError = ::GetLastError();
		if (hFile != INVALID_HANDLE_VALUE) {
			bool doDelete = false;
			LARGE_INTEGER fileSize;
			if (lastError != ERROR_SUCCESS || (::GetFileSizeEx(hFile, &fileSize) && fileSize.QuadPart == 0)) {
				doDelete = true;
			}
			::CloseHandle(hFile);
			if (doDelete)
				DeleteFile(fileName.c_str());
		}
		if (hUrl != nullptr)
			::InternetCloseHandle(hUrl);
		if (hInternet != NULL)
			::InternetCloseHandle(hInternet);
		::SetLastError(lastError);
	});

	hFile = ::CreateFile(fileName.c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return downslib_error::createfile;

	hInternet = ::InternetOpenA(userAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG,
		nullptr, nullptr, 0);

	if (!hInternet)
		return downslib_error::inetopen;

	::InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
	DWORD flags = INTERNET_FLAG_RELOAD;
	if (strncmp(url.c_str(), "https://", 8) == 0)
		flags |= INTERNET_FLAG_SECURE;

	hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, flags, 0);
	if (!hUrl)
		return downslib_error::openurl;

	// Get HTTP content length
	char buffer[1<<11];
	memset(buffer, 0, sizeof(buffer));
	DWORD len = sizeof(buffer);
	unsigned long long totalBytes = 0;
	if (::HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH, buffer, &len, 0)) {
		if (sscanf_s(buffer, "%llu", &totalBytes) != 1)
			totalBytes = 0;
	}

	_dlg.SetProgressRange(totalBytes);

	// Get HTTP status code
	len = sizeof(buffer);
	if (::HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE, buffer, &len, 0)) {
		int statusCode = 0;
		if (sscanf_s(buffer, "%d", &statusCode) != 1)
			statusCode = 500;
		if (statusCode != 200) {
			::SetLastError(statusCode);
			return downslib_error::statuscode;
		}
	}
	
	DWORD read = 0;
	DWORD written = 0;
	unsigned long long readBytes = 0;
	while (::InternetReadFile(hUrl, buffer, sizeof(buffer), &read)) {
		readBytes += read;

		// We are done if nothing more to read, so now we can report total size in our final cb call
		if (read == 0)
			totalBytes = readBytes;

		// Call the callback to report progress and cancellation
		if (cb && !cb(userdata, readBytes, totalBytes)) {
			::SetLastError(ERROR_OPERATION_ABORTED);
			return downslib_error::cancel;
		}

		// Exit if noting more read
		if (read == 0)
			break;

		::WriteFile(hFile, buffer, read, &written, nullptr);
	}

	if (totalBytes > 0 && readBytes != totalBytes) {
		::SetLastError(ERROR_IO_INCOMPLETE);
		return downslib_error::incomplete;
	}

	::SetLastError(ERROR_SUCCESS);
	return downslib_error::ok;
}

