#include<ntifs.h>
/*
https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/ntxxx-routines?redirectedfrom=MSDN
	Zw && Nt
*/

NTKERNELAPI NTSTATUS ZwQueryDirectoryFile(
	_In_     HANDLE                 FileHandle,
	_In_opt_ HANDLE                 Event,
	_In_opt_ PIO_APC_ROUTINE        ApcRoutine,
	_In_opt_ PVOID                  ApcContext,
	_Out_    PIO_STATUS_BLOCK       IoStatusBlock,
	_Out_    PVOID                  FileInformation,
	_In_     ULONG                  Length,
	_In_     FILE_INFORMATION_CLASS FileInformationClass,
	_In_     BOOLEAN                ReturnSingleEntry,
	_In_opt_ PUNICODE_STRING        FileName,
	_In_     BOOLEAN                RestartScan
);

#define MAX_PATH2 4096
#define kmalloc(_s)	ExAllocatePoolWithTag(NonPagedPool,_s,'quer')
#define kfree(_p) ExFreePool(_p);

HANDLE MyFindFirstFile(LPSTR lpDirectory, PFILE_BOTH_DIR_INFORMATION pDir, ULONG uLength) {
	char strFolder[MAX_PATH2] = { 0 };
	STRING astrFolder;
	UNICODE_STRING ustrFolder;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status;
	IO_STATUS_BLOCK ioStatus;
	HANDLE hFile = NULL;
	memset(strFolder, 0, MAX_PATH2);
	strcpy(strFolder, "\\??\\");
	strcat(strFolder, lpDirectory);
	RtlInitString(&astrFolder, strFolder);
	if (RtlAnsiStringToUnicodeString(&ustrFolder, &astrFolder, TRUE) == 0) {
		InitializeObjectAttributes(&oa, &ustrFolder, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
		status = IoCreateFile(
			&hFile,
			FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_ANY_ACCESS,
			&oa,
			&ioStatus,
			nullptr,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN,
			FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
			NULL,
			0,
			CreateFileTypeNone,
			NULL,
			IO_NO_PARAMETER_CHECKING
		);
		RtlFreeUnicodeString(&ustrFolder);
		if (status == STATUS_SUCCESS) {
			status = ZwQueryDirectoryFile(
				hFile,
				NULL,
				NULL,
				NULL,
				&ioStatus,
				pDir,
				uLength,
				FileBothDirectoryInformation,
				TRUE,
				NULL,
				FALSE
			);
			if (!NT_SUCCESS(status)) {
				if (hFile != NULL) {
					ZwClose(hFile);
					hFile = NULL;
				}	
			}
		}
	}
	return hFile;
}

bool MyFindNextFile(HANDLE hFile, PFILE_BOTH_DIR_INFORMATION pDir, ULONG uLength) {
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS status;
	status = ZwQueryDirectoryFile(
		hFile,
		NULL,
		NULL,
		NULL,
		&ioStatus,
		pDir,
		uLength,
		FileBothDirectoryInformation,
		FALSE,
		NULL,
		FALSE
	);
	if (NT_SUCCESS(status))
		return TRUE;
	return false;
}

ULONG SearchDirectory(LPSTR lpPath) {
	ULONG muFileCount = 0;
	HANDLE hFile = NULL;
	PFILE_BOTH_DIR_INFORMATION pDir;
	char *strBuffer = nullptr;
	char strFileName[255 * 2];
	ULONG uLength = MAX_PATH2 * 2 + sizeof(FILE_BOTH_DIR_INFORMATION);
	strBuffer = (PCHAR)kmalloc(uLength);
	pDir = (PFILE_BOTH_DIR_INFORMATION)strBuffer;
	hFile = MyFindFirstFile(lpPath, pDir, uLength);
	if (hFile != NULL) {
		ExFreePool(strBuffer);
		uLength = (MAX_PATH2 * 2 + sizeof(FILE_BOTH_DIR_INFORMATION)) * 0x2000;
		strBuffer = (PCHAR)kmalloc(uLength);
		pDir = (PFILE_BOTH_DIR_INFORMATION)strBuffer;
		if (MyFindNextFile(hFile, pDir, uLength)) {
			while (true)
			{
				memset(strFileName, 0, 255 * 2);
				memcpy(strFileName, pDir->FileName, pDir->FileNameLength);
				if (strcmp(strFileName, "..") != 0 && strcmp(strFileName, ".") != 0) {
					if (pDir->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						KdPrint(("[д©б╪]%S\n", strFileName));
					}
					else {
						KdPrint(("[нд╪Ч]%S\n", strFileName));
					}
					muFileCount++;
				}
				if (pDir->NextEntryOffset == 0)
					break;
				pDir = (PFILE_BOTH_DIR_INFORMATION)((char*)pDir + pDir->NextEntryOffset);
			}
			kfree(strBuffer);
		}
		ZwClose(hFile);
	}
	return muFileCount;
}