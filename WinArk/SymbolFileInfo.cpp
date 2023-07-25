#include "stdafx.h"
#include "SymbolFileInfo.h"
#include <WinInet.h>
#include <filesystem>
#include <Helpers.h>

#pragma comment(lib,"Wininet.lib")

bool SymbolFileInfo::SymDownloadSymbol(std::wstring localPath) {
	std::string url = "http://msdl.microsoft.com/download/symbols";

	_dlg.ShowCancelButton(false);

	if (url.back() != '/')
		url += '/';

	CString temp = _pdbFile + L"/" + _pdbSignature + L"/" + _pdbFile;
	std::wstring symbolUrl = temp.GetBuffer();
	url += std::string(symbolUrl.begin(), symbolUrl.end());
	std::wstring oldFileName = _pdbFile.GetBuffer();
	std::string deleteFile(oldFileName.begin(), oldFileName.end());
	std::wstring fileName = localPath + L"\\" + _pdbSignature.GetBuffer() + L"_" + _pdbFile.GetBuffer();
	bool isExist = std::filesystem::is_regular_file(fileName);
	if (isExist) {
		// How to know the pdb file has downloaded completley since the last time?
		auto fileSize = std::filesystem::file_size(fileName);
		if (fileSize) {
			unsigned long long contentSize = GetPdbSize(url, fileName, "WinArk", 1000);
			if (contentSize == 0) {
				// Other error
				return true;
			}
			if (contentSize != fileSize) {
				int ret = AtlMessageBox(nullptr, L"The pdb size is not equal the content size from symbol server", L"Are you sure to run WinArk>", MB_OKCANCEL);
				if (ret == IDCANCEL) {
					return false;
				}
				return true;
			}
			else
				return true;
		}
	}

	for (auto& iter : std::filesystem::directory_iterator(localPath)) {
		auto filename = iter.path().filename().string();
		if (filename.find(deleteFile.c_str()) != std::string::npos) {
			std::filesystem::remove(iter.path());
			break;
		}
	}

	_dlg.SetMessageText(L"Starting download " + _pdbFile);

	wil::unique_handle hThread(::CreateThread(nullptr, 0, [](auto params)->DWORD {
		SymbolFileInfo* info = (SymbolFileInfo*)params;
		info->_dlg.DoModal();
		return 0;
		}, this, 0, nullptr));

	auto result = Download(url, fileName, "WinArk", 1000,
		[](void* userdata, unsigned long long readBytes, unsigned long long totalBytes) {
			CProgressDlg* pDlg = (CProgressDlg*)userdata;
			if (totalBytes) {
				pDlg->UpdateProgress(static_cast<int>(readBytes));
			}
			return true;
		},
		(void*)&_dlg);
	_dlg.EndDialog(IDCANCEL);
	return result == downslib_error::ok ? true : false;
}

bool SymbolFileInfo::GetPdbSignature(ULONG_PTR imageBase, PIMAGE_DEBUG_DIRECTORY entry) {
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
		_pdbFile = Helpers::StringToWstring(file).c_str();
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
		_pdbFile = Helpers::StringToWstring(file).c_str();
		memcpy(&_pdbValidation.guid, &cv->Signature, sizeof(GUID));
		_pdbValidation.signature = 0;
		_pdbValidation.age = cv->Age;
	}
	return true;
}

downslib_error SymbolFileInfo::Download(std::string url, std::wstring fileName, std::string userAgent,
	unsigned int timeout, downslib_cb cb, void* userdata) {
	HINTERNET hInternet = nullptr;
	HINTERNET hUrl = nullptr;
	HANDLE hFile = nullptr;
	HINTERNET hConnect = nullptr;

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
			if (hInternet != nullptr)
				::InternetCloseHandle(hInternet);
			if (hConnect != nullptr)
				::InternetCloseHandle(hConnect);
			::SetLastError(lastError);
		});

	hFile = ::CreateFile(fileName.c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return downslib_error::createfile;

	hInternet = ::InternetOpenA(userAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG,
		nullptr, nullptr, 0);


	if (!hInternet)
		return downslib_error::inetopen;

	DWORD flags;
	DWORD len = sizeof(flags);

	::InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
	flags = INTERNET_FLAG_RELOAD;
	if (strncmp(url.c_str(), "https://", 8) == 0)
		flags |= INTERNET_FLAG_SECURE;

	hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, flags, 0);
	DWORD error = ::GetLastError();
	if (error == ERROR_INTERNET_INVALID_CA) {
		std::string serviceName = "msdl.microsoft.com";
		// Connect to the http server
		hConnect = InternetConnectA(hInternet, serviceName.c_str(),
			INTERNET_DEFAULT_HTTP_PORT, nullptr,
			nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		int pos = static_cast<int>(url.find(".com") + 4);
		std::string objName(url.begin() + pos, url.end());
		hUrl = HttpOpenRequestA(hConnect, "GET",
			objName.c_str(), nullptr, nullptr, nullptr, INTERNET_FLAG_KEEP_CONNECTION, 0);

		HttpSendRequest(hUrl, nullptr, 0, nullptr, 0);
		error = ::GetLastError();
		if (error == ERROR_INTERNET_INVALID_CA) {
			// https://stackoverflow.com/questions/41357008/how-to-ignore-certificate-in-httppost-request-in-winapi
			flags = 0;
			InternetQueryOption(hUrl, INTERNET_OPTION_SECURITY_FLAGS, &flags, &len);
			flags |= SECURITY_SET_MASK;
			InternetSetOptionA(hUrl, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
			HttpSendRequest(hUrl, nullptr, 0, nullptr, 0);
		}
	}
	if (error == ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR) {
		url.insert(4, "s");
		flags |= INTERNET_FLAG_SECURE;
		hUrl = ::InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, flags, 0);
	}
	if (!hUrl)
		return downslib_error::openurl;

	// Get HTTP content length
	char buffer[1 << 11];
	memset(buffer, 0, sizeof(buffer));
	len = sizeof(buffer);
	unsigned long long totalBytes = 0;
	if (::HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH, buffer, &len, 0)) {
		if (sscanf_s(buffer, "%llu", &totalBytes) != 1)
			totalBytes = 0;
	}

	_dlg.SetProgressRange(static_cast<int>(totalBytes));

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

unsigned long long SymbolFileInfo::GetPdbSize(std::string url, std::wstring fileName, std::string userAgent,
	unsigned int timeout) {
	HINTERNET hInternet = nullptr;
	HINTERNET hUrl = nullptr;
	HINTERNET hConnect = nullptr;

	Cleanup cleanup([&]()
		{
			DWORD lastError = ::GetLastError();
			if (hUrl != nullptr)
				::InternetCloseHandle(hUrl);
			if (hInternet != nullptr)
				::InternetCloseHandle(hInternet);
			if (hConnect != nullptr)
				::InternetCloseHandle(hConnect);
			::SetLastError(lastError);
		});

	hInternet = ::InternetOpenA(userAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG,
		nullptr, nullptr, 0);

	if (!hInternet)
		return 0;

	DWORD flags;
	DWORD len = sizeof(flags);

	::InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
	flags = INTERNET_FLAG_RELOAD;
	if (strncmp(url.c_str(), "https://", 8) == 0)
		flags |= INTERNET_FLAG_SECURE;

	hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, flags, 0);
	DWORD error = ::GetLastError();
	if (error == ERROR_INTERNET_INVALID_CA) {
		std::string serviceName = "msdl.microsoft.com";
		// Connect to the http server
		hConnect = InternetConnectA(hInternet, serviceName.c_str(),
			INTERNET_DEFAULT_HTTP_PORT, nullptr,
			nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		int pos = static_cast<int>(url.find(".com") + 4);
		std::string objName(url.begin() + pos, url.end());
		hUrl = HttpOpenRequestA(hConnect, "GET",
			objName.c_str(), nullptr, nullptr, nullptr, INTERNET_FLAG_KEEP_CONNECTION, 0);

		HttpSendRequest(hUrl, nullptr, 0, nullptr, 0);
		error = ::GetLastError();
		if (error == ERROR_INTERNET_INVALID_CA) {
			// https://stackoverflow.com/questions/41357008/how-to-ignore-certificate-in-httppost-request-in-winapi
			flags = 0;
			InternetQueryOption(hUrl, INTERNET_OPTION_SECURITY_FLAGS, &flags, &len);
			flags |= SECURITY_SET_MASK;
			InternetSetOptionA(hUrl, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
			HttpSendRequest(hUrl, nullptr, 0, nullptr, 0);
		}
	}
	if (error == ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR) {
		url.insert(4, "s");
		flags |= INTERNET_FLAG_SECURE;
		hUrl = ::InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, flags, 0);
	}
	if (!hUrl)
		return 0;

	// Get HTTP content length
	char buffer[1 << 11];
	memset(buffer, 0, sizeof(buffer));
	len = sizeof(buffer);
	unsigned long long totalBytes = 0;
	if (::HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH, buffer, &len, 0)) {
		if (sscanf_s(buffer, "%llu", &totalBytes) != 1)
			totalBytes = 0;
	}

	return totalBytes;
}