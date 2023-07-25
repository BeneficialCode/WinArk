#pragma once

enum class FileAccessMask :ULONG {
	WriteData = FILE_WRITE_DATA,// 写文件内容
	ReadData = FILE_READ_DATA,	// 读文件内容
	Delete = DELETE,			// 删除或者给文件改名字
	WriteAttributes = FILE_WRITE_ATTRIBUTES,	// 设置文件写属性
	ReadAttributes = FILE_READ_ATTRIBUTES,	// 设置文件读属性
	Read = GENERIC_READ,
	Write = GENERIC_WRITE,
	All = GENERIC_ALL,
	Synchronize = SYNCHRONIZE,
};
DEFINE_ENUM_FLAG_OPERATORS(FileAccessMask);

extern "C"
NTSTATUS NtQueryAttributesFile(
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_Out_ PFILE_BASIC_INFORMATION FileInformation
);

extern "C"
NTSTATUS ObCreateObject(
	_In_opt_ KPROCESSOR_MODE ObjectAttributesAccessMode,
	_In_ POBJECT_TYPE Type,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ KPROCESSOR_MODE AccessMode,
	_Inout_opt_ PVOID ParseContext,
	_In_ ULONG ObjectSize,
	_In_opt_ ULONG PagedPoolCharge,
	_In_opt_ ULONG NonPagedPoolCharge,
	_Out_ PVOID * Object
);

typedef struct _AUX_ACCESS_DATA {
	PPRIVILEGE_SET PrivilegesUsed;
	GENERIC_MAPPING GenericMapping;
	ACCESS_MASK AccessesToAudit;
	ACCESS_MASK MaximumAuditMask;
	ULONG Unknown[256];
}AUX_ACCESS_DATA, * PAUX_ACCESS_DATA;


extern "C"
NTSTATUS SeCreateAccessState(
	_Out_ PACCESS_STATE AccessState,
	_Out_ PAUX_ACCESS_DATA AuxData,
	_In_ ACCESS_MASK DesiredAccess,
	_In_opt_ PGENERIC_MAPPING GenericMapping
);

IO_COMPLETION_ROUTINE IoCompletionRoutine;


// 参考链接
// https://bbs.pediy.com/thread-264070.htm
struct FileManager {
public:
	NTSTATUS Open(PUNICODE_STRING fileName, FileAccessMask accessMask = FileAccessMask::All);
	NTSTATUS Close();
	NTSTATUS WriteFile(PVOID buffer, ULONG size, PIO_STATUS_BLOCK ioStatus, PLARGE_INTEGER offset);
	NTSTATUS ReadFile(PVOID buffer, ULONG size, PIO_STATUS_BLOCK ioStatus, PLARGE_INTEGER offset);
	NTSTATUS GetFileSize(PLARGE_INTEGER fileSize);
	//NTSTATUS CreateDir(PUNICODE_STRING dir);
	static NTSTATUS DeleteFile(PUNICODE_STRING fileName);
	static NTSTATUS ConvertDosNameToNtName(_In_ PCWSTR dosName, _Out_ PUNICODE_STRING ntName);

	NTSTATUS GetRootName(_In_ PCWSTR dosName, _Out_ PUNICODE_STRING rootName);

	NTSTATUS OpenFileForRead(PCWSTR path);
	//NTSTATUS RenameFile();
	//NTSTATUS EnumurateFile();

	NTSTATUS SetInformationFile(PIO_STATUS_BLOCK ioStatus, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);

	// 解锁并释放MDL链
	static VOID FreeMdl(_In_ PMDL mdl);

	NTSTATUS IrpCreateFile(
		_Out_ PFILE_OBJECT* pFileObject,
		_Out_ PDEVICE_OBJECT* pDeviceObject,
		_In_ ACCESS_MASK DesiredAccess,
		_In_ PCWSTR DosName,
		_Out_ PIO_STATUS_BLOCK IoStatusBlock,
		_In_opt_ PLARGE_INTEGER AllocationSize,
		_In_ ULONG FileAttributes,
		_In_ ULONG ShareAccess,
		_In_ ULONG CreateDisposition,
		_In_ ULONG CreateOptions,
		_In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
		_In_ ULONG EaLength
	);

	NTSTATUS IrpQueryDirectoryFile(
		_In_ PDEVICE_OBJECT DeviceObject,
		_In_ PFILE_OBJECT FileObject,
		_Out_ PIO_STATUS_BLOCK IoStatusBlock,
		_Out_writes_bytes_(Length) PVOID FileInformation,
		_In_ ULONG Length,
		_In_ FILE_INFORMATION_CLASS FileInformationClass,
		_In_ BOOLEAN ReturnSingleEntry,
		_In_opt_ PUNICODE_STRING FileName,
		_In_ BOOLEAN RestartScan
	);

	NTSTATUS IrpSetInformationFile(
		_In_ PDEVICE_OBJECT DeviceObject,
		_In_ PFILE_OBJECT FileObject,
		_In_opt_ PFILE_OBJECT TargetFileObject,
		_Out_ PIO_STATUS_BLOCK IoStatusBlock,
		_In_reads_bytes_(Length) PVOID FileInformation,
		_In_ ULONG Length,
		_In_ FILE_INFORMATION_CLASS FileInformationClass
	);

	NTSTATUS IrpCleanupFile(
		_In_ PDEVICE_OBJECT DeviceObject,
		_In_ PFILE_OBJECT FileObject
	);

	NTSTATUS IrpCloseFile(
		_In_ PDEVICE_OBJECT DeviceObject,
		_In_ PFILE_OBJECT FileObject
	);

	NTSTATUS IrpDeleteFile(
		IN PDEVICE_OBJECT DeviceObject, //如果指定为pFileObject->Vpb->DeviceObject,可绕过文件系统过滤驱动
		IN PFILE_OBJECT FileObject
	);

	NTSTATUS IrpQueryInformationFile(
		_In_ PDEVICE_OBJECT DeviceObject,
		_In_ PFILE_OBJECT FileObject,
		_Out_ PIO_STATUS_BLOCK IoStatusBlock,
		_Out_writes_bytes_(Length) PVOID FileInformation,
		_In_ ULONG Length,
		_In_ FILE_INFORMATION_CLASS FileInformationClass
	);

	NTSTATUS ForceDeleteFile(PCWSTR fileName);

	~FileManager();

private:
	HANDLE _handle{ nullptr };
};