// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include <Windows.h>
#include <winnt.h>
#include <winternl.h>
#include <stddef.h>

#include "..\KernelLibrary\Common.h"

#define DBG_FILE_PATH  L"C:\\Log_InjectAll.txt"          //Log file to write from the injected DLL

extern "C" {
	typedef short CSHORT;

	typedef struct _TIME_FIELDS {
		CSHORT Year;        // range [1601...]
		CSHORT Month;       // range [1..12]
		CSHORT Day;         // range [1..31]
		CSHORT Hour;        // range [0..23]
		CSHORT Minute;      // range [0..59]
		CSHORT Second;      // range [0..59]
		CSHORT Milliseconds;// range [0..999]
		CSHORT Weekday;     // range [0..6] == [Sunday..Saturday]
	} TIME_FIELDS;
	typedef TIME_FIELDS* PTIME_FIELDS;

	__declspec(dllimport) NTSTATUS NTAPI RtlDosPathNameToNtPathName_U_WithStatus(
		_In_ PCWSTR DosName,
		_Out_ PUNICODE_STRING NtName,
		_Out_ PWSTR* PartName,
		_Out_ PVOID RelativeName
	);

	__declspec(dllimport) NTSTATUS NTAPI NtWriteFile(
		_In_           HANDLE           FileHandle,
		_In_opt_ HANDLE           Event,
		_In_opt_ PIO_APC_ROUTINE  ApcRoutine,
		_In_opt_ PVOID            ApcContext,
		_Out_          PIO_STATUS_BLOCK IoStatusBlock,
		_In_           PVOID            Buffer,
		_In_           ULONG            Length,
		_In_opt_ PLARGE_INTEGER   ByteOffset,
		_In_opt_ PULONG           Key
	);

	__declspec(dllimport) int __CRTDECL vsprintf_s(
		_Out_writes_(_BufferCount) _Always_(_Post_z_) char* const _Buffer,
		_In_                                          size_t      const _BufferCount,
		_In_z_ _Printf_format_string_                 char const* const _Format,
		va_list           _ArgList
	);

	__declspec(dllimport) void NTAPI RtlSystemTimeToLocalTime(
		PLARGE_INTEGER SystemTime,
		PLARGE_INTEGER LocalTime
	);

	__declspec(dllimport) VOID NTAPI RtlTimeToTimeFields(
		_In_ PLARGE_INTEGER Time,
		_Out_ PTIME_FIELDS TimeFields
	);

	__declspec(dllimport) NTSTATUS NTAPI ZwQueueApcThread(HANDLE hThread,
		void* ApcRoutine, PVOID ApcContext, PVOID Argument1, PVOID Argument2);
}

#endif //PCH_H
