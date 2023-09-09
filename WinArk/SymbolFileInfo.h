#pragma once
#include "ProgressDlg.h"
#include <Poco/URIStreamOpener.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <Poco/Net/HTTPStreamFactory.h>
#include "Poco/Net/HTTPSStreamFactory.h"
#include <Poco/Net/FTPStreamFactory.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/ConsoleCertificateHandler.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

struct CV_HEADER {
	DWORD Signature;
	DWORD Offset;
};

struct CV_INFO_PDB20 {
	CV_HEADER CvHeader;
	DWORD Signature;
	DWORD Age;
	BYTE PdbFileName[ANYSIZE_ARRAY];
};

struct CV_INFO_PDB70 {
	DWORD CvSignature;
	GUID Signature;
	DWORD Age;
	BYTE PdbFileName[ANYSIZE_ARRAY];
};

struct PdbValidationData {
	GUID guid{ 0 };
	DWORD signature = 0;
	DWORD age = 0;
};

using downslib_cb = bool (*)(void* userdata, unsigned long long readBytes, unsigned long long totalBytes);

enum class downslib_error {
	ok, createfile, inetopen, openurl, statuscode, cancel, incomplete
};

class Cleanup {
	std::function<void()> fn;
public:
	explicit Cleanup(std::function<void()> fn):fn(std::move(fn)){}
	~Cleanup() { fn(); }
};

struct SymbolFileInfo {
	SymbolFileInfo();
	~SymbolFileInfo();
	bool SymDownloadSymbol(std::wstring localFile);
	bool GetPdbSignature(ULONG_PTR imageBase,PIMAGE_DEBUG_DIRECTORY entry);
	downslib_error Download(std::string url, std::wstring fileName, 
		std::string userAgent, unsigned int timeout,downslib_cb = nullptr, void* userdata = nullptr);

	unsigned long long GetPdbSize(std::string url, std::wstring fileName, std::string userAgent,
		unsigned int timeout);

	downslib_error PdbDownLoader(Poco::URI& uri, std::ostream& ostr);

	CString _pdbSignature;
	CString _pdbFile;
	PdbValidationData _pdbValidation;
	CProgressDlg _dlg;
	CString _path;
};