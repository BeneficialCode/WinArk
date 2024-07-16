#include "stdafx.h"
#include "Processes.h"
#include "ProcessInfo.h"
#include "ProcessManager.h"
#include "Helpers.h"
#include "ProcessInfoEx.h"
#include "DriverHelper.h"

#pragma comment(lib,"Version.lib")

ProcessInfoEx::ProcessInfoEx(WinSys::ProcessInfo* pi) :_pi(pi) {
	auto hProcess = DriverHelper::OpenProcess(_pi->Id,PROCESS_QUERY_INFORMATION|PROCESS_VM_READ);
	if(!hProcess)
		hProcess = DriverHelper::OpenProcess(_pi->Id,PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ);
	if (hProcess)
		_process.reset(new WinSys::Process(hProcess));
}

WinSys::ProcessInfo* ProcessInfoEx::GetProcessInfo() const {
	return _pi;
}

ProcessAttributes ProcessInfoEx::GetAttributes(const WinSys::ProcessManager& pm) const {
	if (_attributes == ProcessAttributes::NotComputed) {
		_attributes = ProcessAttributes::None;
		if (_process) {
			if (_process->IsManaged())
				_attributes |= ProcessAttributes::Managed;
			if (_process->IsProtected())
				_attributes |= ProcessAttributes::Protected;
			if (_process->IsImmersive())
				_attributes |= ProcessAttributes::Immersive;
			if (_process->IsSecure())
				_attributes |= ProcessAttributes::Secure;
			if (_process->IsInJob())
				_attributes |= ProcessAttributes::InJob;
			auto parent = pm.GetProcessById(_pi->ParentId);
			if (parent && ::_wcsicmp(parent->GetImageName().c_str(), L"services.exe") == 0)
				_attributes |= ProcessAttributes::Service;
			if (_process->IsWow64Process())
				_attributes |= ProcessAttributes::Wow64;
		}
	}
	return _attributes;
}

const std::wstring& ProcessInfoEx::GetExecutablePath() const {
	if (_executablePath.empty() && _pi->Id != 0) {
		auto path = _pi->GetNativeImagePath();
		if (path.empty() && _process) {
			path = _process->GetFullImageName();
		}
		if (path[0] == L'\\') {
			_executablePath = Helpers::GetDosNameFromNtName(path.c_str());
		}
		else {
			_executablePath = path;
		}
	}
	return _executablePath;
}

int ProcessInfoEx::GetMemoryPriority() const {
	return _process ? _process->GetMemoryPriority() : -1;
}

WinSys::ProcessPriorityClass ProcessInfoEx::GetPriorityClass() {
	return _process ? _process->GetPriorityClass() : WinSys::ProcessPriorityClass::Unknown;
}

const std::wstring& ProcessInfoEx::GetCmdLine() const {
	if (_commandLine.empty() && _pi->Id > 4) {
		if (_process)
			_commandLine = _process->GetCmdLine();
	}
	return _commandLine;
}

//bool ProcessInfoEx::IsElevated() const {
//	if (!_elevatedChecked) {
//		_elevatedChecked = true;
//		if (_pi->Id > 4 && _process != nullptr) {
//			WinSys::Token token(_process->GetHandle(), WinSys::TokenAccessMask::Query);
//			if (token.IsValid())
//				_elevated = token.IsElevated();
//		}
//	}
//	return _elevated;
//}

std::wstring ProcessInfoEx::GetCurDirectory() const {
	auto hProcess = DriverHelper::OpenProcess(_pi->Id, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION);
	if (!hProcess)
		return L"";

	auto dir = WinSys::Process::GetCurDirectory(hProcess);
	if (hProcess)
		::CloseHandle(hProcess);
	return dir;
}

const std::wstring& ProcessInfoEx::GetCompanyName() const {
	if (!_companyChecked) {
		BYTE buffer[1 << 12];
		if (::GetFileVersionInfo(GetExecutablePath().c_str(), 0, sizeof(buffer), buffer)) {
			WORD* langAndCodePage;
			UINT len;
			if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
				CString text;
				text.Format(L"\\StringFileInfo\\%04x%04x\\CompanyName", langAndCodePage[0], langAndCodePage[1]);
				WCHAR* companyName;
				if (::VerQueryValue(buffer, text, (void**)&companyName, &len))
					_companyName = companyName;
			}
		}
	}
	return _companyName;
}

const std::wstring& ProcessInfoEx::GetVersion() const {
	if (!_versionChecked) {
		BYTE buffer[1 << 12];
		if (::GetFileVersionInfo(GetExecutablePath().c_str(), 0, sizeof(buffer), buffer)) {
			WORD* langAndCodePage;
			UINT len;
			if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
				CString text;
				text.Format(L"\\StringFileInfo\\%04x%04x\\FileVersion", langAndCodePage[0], langAndCodePage[1]);
				WCHAR* version;
				if (::VerQueryValue(buffer, text, (void**)&version, &len))
					_version = version;
			}
		}
		_versionChecked = true;
	}
	return _version;
}

int ProcessInfoEx::GetBitness() const {
	if(_bitness==0){
		static SYSTEM_INFO si = { 0 };
		if (si.dwNumberOfProcessors == 0)
			::GetNativeSystemInfo(&si);
		if (_process == nullptr) {
			_bitness = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM ? 32 : 64;
		}
		else {
			if (_process->IsWow64Process())
				_bitness = 32;
			else
				_bitness = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM ? 32 : 64;
		}
	}
	return _bitness;
}

const std::wstring& ProcessInfoEx::GetDescription() const {
	if (!_descChecked) {
		BYTE buffer[1 << 12];
		if (::GetFileVersionInfo(GetExecutablePath().c_str(), 0, sizeof(buffer), buffer)) {
			WORD* langAndCodePage;
			UINT len;
			if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
				CString text;
				text.Format(L"\\StringFileInfo\\%04x%04x\\FileDescription", langAndCodePage[0], langAndCodePage[1]);
				WCHAR* desc;
				if (::VerQueryValue(buffer, text, (void**)&desc, &len))
					_description = desc;
			}
		}
		_descChecked = true;
	}
	return _description;
}

const std::wstring& ProcessInfoEx::UserName() const {
	if (_username.empty()) {
		if (_pi->Id <= 4)
			_username = L"NT AUTHORITY\\SYSTEM";
		else {
			if (_process)
				_username = _process->GetTokenUserName();
			if (_username.empty())
				_username = L"<¾Ü¾ø·ÃÎÊ>";
		}
	}
	return _username;
}

int ProcessInfoEx::GetImageIndex(CImageList images) const {
	if (_image < 0) {
		_image = 0;
		HICON hIcon = nullptr;
		::ExtractIconEx(GetExecutablePath().c_str(), 0, nullptr, &hIcon, 1);
		if (!hIcon) {
			SHFILEINFO sfi;
			::SHGetFileInfo(GetExecutablePath().c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_USEFILEATTRIBUTES);
			hIcon = sfi.hIcon;
		}
		if (hIcon)
			_image = images.AddIcon(hIcon);
	}
	return _image;
}