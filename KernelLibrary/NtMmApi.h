#pragma once

typedef struct _SECTION_IMAGE_INFORMATION {
	PVOID TransferAddress;
	ULONG ZeroBits;
	SIZE_T MaximumStackSize;
	SIZE_T CommittedStackSize;
	ULONG SubSystemType;
	union {
		struct {
			USHORT SubSystemMinorVersion;
			USHORT SubSystemMajorVersion;
		};
		ULONG SubSystemVersion;
	};
	union {
		struct {
			USHORT MajorOperatingSystemVersion;
			USHORT MinorOperatingSystemVersion;
		};
		ULONG OperatingSystemVersion;
	};
	USHORT ImageCharacteristics;
	USHORT DllCharacteristics;
	USHORT Machine;
	BOOLEAN ImageContainsCode;
	union {
		UCHAR ImageFlags;
		struct {
			UCHAR ComPlusNativeReady : 1;
			UCHAR ComPlusILOnly : 1;
			UCHAR ImageDynamicallyRelocated : 1;
			UCHAR ImageMappedFlat : 1;
			UCHAR BaseBelow4gb : 1;
			UCHAR ComPlusPrefer32bit : 1;
			UCHAR Reserved : 2;
		};
	};
	ULONG LoaderFlags;
	ULONG ImageFileSize;
	ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, * PSECTION_IMAGE_INFORMATION;

typedef struct _PS_SYSTEM_DLL_INFO {

	//
	// Flags.
	// Initialized statically.
	// 

	USHORT        Flags;

	//
	// Machine type of this WoW64 NTDLL.
	// Initialized statically.
	// Examples:
	//   - IMAGE_FILE_MACHINE_I386
	//   - IMAGE_FILE_MACHINE_ARMNT
	//

	USHORT        MachineType;

	//
	// Unused, always 0.
	//

	ULONG         Reserved1;

	//
	// Path to the WoW64 NTDLL.
	// Initialized statically.
	// Examples:
	//   - "\\SystemRoot\\SysWOW64\\ntdll.dll"
	//   - "\\SystemRoot\\SysArm32\\ntdll.dll"
	//

	UNICODE_STRING Ntdll32Path;

	//
	// Image base of the DLL.
	// Initialized at runtime by PspMapSystemDll.
	// Equivalent of:
	//      RtlImageNtHeader(BaseAddress)->
	//          OptionalHeader.ImageBase;
	//

	PVOID         ImageBase;

	//
	// Contains DLL name (such as "ntdll.dll" or
	// "ntdll32.dll") before runtime initialization.
	// Initialized at runtime by MmMapViewOfSectionEx,
	// called from PspMapSystemDll.
	//

	union {
		PVOID       BaseAddress;
		PWCHAR      DllName;
	};

	//
	// Unused, always 0.
	//

	PVOID         Reserved2;

	//
	// Section relocation information.
	//

	PVOID         SectionRelocationInformation;

	//
	// Unused, always 0.
	//

	PVOID         Reserved3;

} PS_SYSTEM_DLL_INFO, * PPS_SYSTEM_DLL_INFO;
