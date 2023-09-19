#include "stdafx.h"
#include "SymbolFileInfo.h"
#include <filesystem>
#include <Helpers.h>
#include <fstream>


using Poco::URIStreamOpener;
using Poco::StreamCopier;
using Poco::Path;
using Poco::Exception;
using Poco::Net::HTTPStreamFactory;
using Poco::Net::FTPStreamFactory;
using Poco::SharedPtr;
using Poco::Net::HTTPSStreamFactory;
using Poco::Net::SSLManager;
using Poco::Net::Context;
using Poco::Net::InvalidCertificateHandler;
using Poco::Net::ConsoleCertificateHandler;
using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPMessage;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPClientSession;

#pragma comment(lib,"Ws2_32")
#pragma comment(lib,"Iphlpapi")
#pragma comment(lib,"Crypt32")

bool SymbolFileInfo::SymDownloadSymbol(std::wstring localPath) {
	std::wstring path = localPath + L"\\" + _path.GetString();
	std::filesystem::create_directories(path);

	std::string url = "https://msdl.microsoft.com/download/symbols";

	_dlg.ShowCancelButton(false);

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

	_dlg.SetMessageText(L"Starting download " + _pdbFile);

	wil::unique_handle hThread(::CreateThread(nullptr, 0, [](auto params)->DWORD {
		SymbolFileInfo* info = (SymbolFileInfo*)params;
		info->_dlg.DoModal();
		return 0;
		}, this, 0, nullptr));

	auto result = Download(url, fileName, "WinArk", 1000, 
		[](void* userdata,unsigned long long readBytes,unsigned long long totalBytes) {
			CProgressDlg* pDlg = (CProgressDlg*)userdata;
			if (totalBytes) {
				pDlg->UpdateProgress(static_cast<int>(readBytes));
			}
			return true;
		}, 
		(void*)&_dlg);
	_dlg.EndDialog(IDCANCEL);
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
	unsigned int timeout,downslib_cb cb, void* userdata){
	std::ofstream fileStream;
	fileStream.open(fileName, std::ios::out | std::ios::trunc | std::ios::binary);

	SharedPtr<InvalidCertificateHandler> pCertHandler = new ConsoleCertificateHandler(false); // ask the user via console
	Context::Ptr pContext = new Context(Context::CLIENT_USE, "", Context::VerificationMode::VERIFY_NONE);
	SSLManager::instance().initializeClient(0, pCertHandler, pContext);
	Poco::URI uri(url);
	if (uri.getScheme() == "http") {
		try
		{
			std::string path(uri.getPathAndQuery());
			if (path.empty()) path = "/";
			HTTPClientSession session(uri.getHost(), uri.getPort());
			HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
			session.sendRequest(request);
			HTTPResponse response;
			session.peekResponse(response);
			if (response.getStatus() == HTTPResponse::HTTPStatus::HTTP_FOUND) {
				uri = response.get("Location");
			}
		}
		catch (Poco::Exception& exc)
		{
			fileStream << exc.displayText() << std::endl;
			return downslib_error::incomplete;
		}
	}
	else if (uri.getScheme() == "https") {
		try
		{
			HTTPSClientSession session(uri.getHost(), uri.getPort());
			std::string path(uri.getPathAndQuery());
			if (path.empty()) path = "/";
			HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
			session.sendRequest(request);
			HTTPResponse response;
			session.peekResponse(response);
			if (response.getStatus() == HTTPResponse::HTTPStatus::HTTP_FOUND) {
				uri = response.get("Location");
			}
		}
		catch (Poco::Exception& exc)
		{
			fileStream << exc.displayText() << std::endl;
			return downslib_error::incomplete;
		}
	
	}
	downslib_error ret = PdbDownLoader(uri, fileStream);
	_dlg.SetProgressRange(100);
	cb(userdata, 100, 100);
	fileStream.close();
	return ret;
}

unsigned long long SymbolFileInfo::GetPdbSize(std::string url, std::wstring fileName, std::string userAgent,
	unsigned int timeout) {
	

	return 0;
}

SymbolFileInfo::SymbolFileInfo() {
	Poco::Net::initializeSSL();
	HTTPStreamFactory::registerFactory();
	HTTPSStreamFactory::registerFactory();
	FTPStreamFactory::registerFactory();
}

SymbolFileInfo::~SymbolFileInfo() {
	FTPStreamFactory::unregisterFactory();
	HTTPSStreamFactory::unregisterFactory();
	HTTPStreamFactory::unregisterFactory();
	Poco::Net::uninitializeSSL();
}

downslib_error SymbolFileInfo::PdbDownLoader(Poco::URI& uri, std::ostream& ostr) {
	try {
		std::unique_ptr<std::istream> pStr(URIStreamOpener::defaultOpener().open(uri));
		StreamCopier::copyStream(*pStr.get(), ostr);
		return downslib_error::ok;
	}
	catch (Exception& exc) {
		ostr << exc.displayText() << std::endl;
		return downslib_error::incomplete;
	}
}