#include "stdafx.h"
#include "SymbolFileInfo.h"
#include <filesystem>
#include <Helpers.h>
#include <fstream>
#include <curlcpp/curl_easy.h>
#include <curlcpp/curl_exception.h>
#include <curlcpp/curl_ios.h>

using curl::curl_easy;
using curl::curl_ios;
using curl::curl_easy_exception;
using curl::curlcpp_traceback;

#pragma comment(lib,"Ws2_32")
#pragma comment(lib,"Iphlpapi")
#pragma comment(lib,"Crypt32")


bool SymbolFileInfo::SymDownloadSymbol(std::wstring localPath) {
	std::wstring path = localPath + L"\\" + _path.GetString();
	std::filesystem::create_directories(path);

	std::string url = "https://msdl.microsoft.com/download/symbols";

	if (url.back() != '/')
		url += '/';

	CString temp = _pdbFile + L"/" + _pdbSignature + L"/" + _pdbFile;
	std::wstring symbolUrl = temp.GetBuffer();
	url += Helpers::WstringToString(symbolUrl);
	std::wstring fileName = path + L"\\" + _pdbFile.GetString();
	bool isExist = std::filesystem::is_regular_file(fileName);
	
	if (isExist) {
		// If the symbols pdb download by SDM.exe, we just skip the check.
		std::wstring sdm = localPath + L"\\sdm.json";
		isExist = std::filesystem::is_regular_file(sdm);
		if (isExist) {
			return true;
		}

		// How to know the pdb file has downloaded completley since the last time?
		auto fileSize = std::filesystem::file_size(fileName);
		if (fileSize) {
			return true;
		}
	}

	wil::unique_handle hThread(::CreateThread(nullptr, 0, [](auto params)->DWORD {
		SymbolFileInfo* info = (SymbolFileInfo*)params;
		return 0;
		}, this, 0, nullptr));

	auto result = Download(url, fileName, "WinArk", 1000,
		[](void* userdata, unsigned long long readBytes, unsigned long long totalBytes) {

			return true;
		},
		nullptr);
	
	bool bOk = result == downslib_error::ok ? true : false;
	if (!bOk) {
		MessageBox(NULL, L"Failed init symbols,\r\nWinArk will exit...\r\n", L"WinArk", MB_ICONERROR);
		std::filesystem::remove_all(path);
	}
	return bOk;
}

bool SymbolFileInfo::GetPdbSignature(ULONG_PTR imageBase,PIMAGE_DEBUG_DIRECTORY entry) {
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
	_path = _pdbFile + L"\\" + _pdbSignature;
	return true;
}

downslib_error SymbolFileInfo::Download(std::string url, std::wstring fileName, std::string userAgent,
	unsigned int timeout, downslib_cb cb, void* userdata) {
	std::ofstream fileStream(fileName, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!fileStream) {
		return downslib_error::incomplete;
	}

	curl_ios<std::ostream> writer(fileStream);

	curl_easy easy;

	easy.add<CURLOPT_URL>(url.c_str());
	easy.add<CURLOPT_SSL_VERIFYPEER>(1L);
	easy.add<CURLOPT_SSL_VERIFYHOST>(2L);

	easy.add<CURLOPT_FOLLOWLOCATION>(1L);

	easy.add<CURLOPT_WRITEFUNCTION>(writer.get_function());
	easy.add<CURLOPT_WRITEDATA>(writer.get_stream());

	easy.perform();

	auto code = easy.get_info<CURLINFO_RESPONSE_CODE>();

	if (code.get() != 200) {
		return downslib_error::incomplete;
	}

    if (cb) {
        cb(userdata, 100, 100);
    }
    fileStream.close();
    return downslib_error::ok;
}

SymbolFileInfo::SymbolFileInfo() {
}

SymbolFileInfo::~SymbolFileInfo() {
}