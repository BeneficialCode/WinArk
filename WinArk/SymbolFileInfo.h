#pragma once
#include "ProgressDlg.h"

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
	explicit Cleanup(std::function<void()> fn) :fn(std::move(fn)) {}
	~Cleanup() { fn(); }
};

struct SymbolFileInfo {
	bool SymDownloadSymbol(std::wstring localFile);
	bool GetPdbSignature(ULONG_PTR imageBase, PIMAGE_DEBUG_DIRECTORY entry);
	downslib_error Download(std::string url, std::wstring fileName,
		std::string userAgent, unsigned int timeout, downslib_cb = nullptr, void* userdata = nullptr);

	unsigned long long GetPdbSize(std::string url, std::wstring fileName, std::string userAgent,
		unsigned int timeout);

	CString _pdbSignature;
	CString _pdbFile;
	PdbValidationData _pdbValidation;
	CProgressDlg _dlg;
};