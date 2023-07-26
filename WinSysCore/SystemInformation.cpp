#include "pch.h"
#include "SystemInformation.h"
#include <Psapi.h>
#include <assert.h>

using namespace WinSys;

thread_local int LastStatus;

static_assert(sizeof(PerformanceInformation) == sizeof(SYSTEM_PERFORMANCE_INFORMATION));

static_assert(sizeof(CodeIntegrityInformation) == sizeof(SYSTEM_CODEINTEGRITY_INFORMATION));

PerformanceInformation WinSys::SystemInformation::GetPerformanceInformation() {
	PerformanceInformation info{};
	ULONG len;
	auto status = ::NtQuerySystemInformation(SystemPerformanceInformation, &info, sizeof(info), &len);
	assert(NT_SUCCESS(status));
	return info;
}

const WindowsVersion& WinSys::SystemInformation::GetWindowsVersion() {
	static WindowsVersion version;
	if (version.Major == 0) {
		auto shareData = USER_SHARED_DATA;
		version.Build = shareData->NtBuildNumber;
		version.Major = shareData->NtMajorVersion;
		version.Minor = shareData->NtMinorVersion;
	}
#pragma warning(push)
#pragma warning(disable:4996)
	OSVERSIONINFOEX osv = { sizeof(osv) };
	GetVersionEx((OSVERSIONINFO*)&osv);
	if (version.Build != osv.dwBuildNumber) {
		version.Build = osv.dwBuildNumber;
	}
#pragma warning(pop)
	return version;
}

const BasicSystemInfo& WinSys::SystemInformation::GetBasicSystemInfo() {
	static BasicSystemInfo info;
	if (info.PageSize == 0) {
		SYSTEM_INFO si;
		::GetNativeSystemInfo(&si);
		info.NumberOfProcessors = si.dwNumberOfProcessors;
		info.PageSize = si.dwPageSize;
		info.ProcessorArchitecture = static_cast<ProcessorArchitecture>(si.wProcessorArchitecture);
		info.ProcessorLevel = si.wProcessorLevel;
		info.ProcessorRevision = si.wProcessorRevision;
		info.MinimumAppAddress = si.lpMinimumApplicationAddress;
		info.MaximumAppAddress = si.lpMaximumApplicationAddress;

		PERFORMACE_INFORMATION pi = { sizeof(pi) };
		::GetPerformanceInfo(&pi, sizeof(pi));
		info.TotalPhysicalInPages = pi.PhysicalTotal;
		info.CommitLimitInPages = pi.CommitLimit;
	}
	return info;
}

uint64_t SystemInformation::GetBootTime() {
	static int64_t time = 0;
	if (time == 0) {
		SYSTEM_TIMEOFDAY_INFORMATION info;
		if(NT_SUCCESS(::NtQuerySystemInformation(SystemTimeOfDayInformation,&info,sizeof(info),nullptr)))
			time = info.BootTime.QuadPart;
	}
	return time;
}

int WinSys::SetLastStatus(int status) {
	LastStatus = status;
	return status;
}

int WinSys::GetLastStatus() {
	return LastStatus;
}

std::string SystemInformation::GetCpuBrand() {
	char brand[49] = { 0 };
	auto status = ::NtQuerySystemInformation(SystemProcessorBrandString, brand, sizeof(brand), nullptr);
	assert(NT_SUCCESS(status));
	std::string cpuBrand(brand);
	return cpuBrand;
}

BOOL SystemInformation::GetCodeIntegrityInformation(PBOOLEAN pbKMCI, PBOOLEAN pbStrictMode,
	PBOOLEAN pbIUM){
	CodeIntegrityInformation info;
	info.Length = sizeof(info);
	ULONG len;
	auto status = NtQuerySystemInformation(SystemCodeIntegrityInformation,
		&info, sizeof(info), &len);
	assert(NT_SUCCESS(status));
	if ((info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_ENABLED) &&
		(info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED)) {
		if (pbKMCI)
			*pbKMCI = TRUE;

		if (pbStrictMode) {
			*pbStrictMode = info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED;
		}

		if (pbIUM) {
			*pbIUM = (info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED) > 0;
		}
	}

	
	return TRUE;
}