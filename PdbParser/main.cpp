#include <Windows.h>
#include <DbgHelp.h>
#include <wininet.h>
#include <stdio.h>

#pragma comment(lib,"Wininet.lib")
#pragma comment(lib,"Dbghelp.lib")

typedef struct _PDBINFO
{
	ULONG Signature;
	GUID UID;
	ULONG Age;
	CHAR PDBFileName[128];
}PDBINFO, * PPDBINFO;

BOOLEAN
WINAPI
DownloadSymbol(
	IN PCHAR SymbolUrl,
	IN PCHAR SavePath
)
{
	BOOL Success = FALSE;
	ULONG numOfBytesRead = 0;

	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;

	FILE* fp = NULL;

	UCHAR Buffer[8192] = { 0 };

	hSession = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hSession)
	{
		return FALSE;
	}

	hConnect = InternetConnectA(hSession, "msdl.microsoft.com", 80, "", "", INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect)
	{
		InternetCloseHandle(hSession);
		return FALSE;
	}

	hRequest = HttpOpenRequestA(hConnect, "GET", SymbolUrl, NULL, NULL, NULL, 0, 0);
	if (!hRequest)
	{
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hSession);
		return FALSE;
	}

	Success = HttpSendRequestA(hRequest, NULL, 0, NULL, 0);
	if (Success)
	{
		fp = fopen(SavePath, "wb+");
		if (fp)
		{
			do
			{
				RtlZeroMemory(Buffer, sizeof(Buffer));
				Success = InternetReadFile(hRequest, Buffer, 8192, &numOfBytesRead);
				if (Success)
				{
					if (numOfBytesRead != 0)
					{
						fwrite(Buffer, numOfBytesRead, 1, fp);
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			} while (TRUE);
			fclose(fp);
		}
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hSession);
	return Success;
}

BOOLEAN
WINAPI
GetSymbol(
	IN ULONG64 ImageBase
)
{
	PIMAGE_DEBUG_DIRECTORY DebugDirectory = NULL;
	ULONG DirectorySize = 0;
	PDBINFO PDB = { 0 };

	CHAR PDBGUID[64] = { 0 };
	CHAR SymURL[128] = { 0 };
	CHAR SymPath[128] = { 0 };

	FILE* File;

	if (ImageBase)
	{
		DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)ImageDirectoryEntryToData(
			(PVOID)ImageBase,
			FALSE,
			IMAGE_DIRECTORY_ENTRY_DEBUG,
			&DirectorySize
		);

		RtlCopyMemory(&PDB, (PCHAR)ImageBase + DebugDirectory->PointerToRawData, sizeof(PDB));

		sprintf_s(
			PDBGUID, 64, "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%X",
			PDB.UID.Data1, PDB.UID.Data2, PDB.UID.Data3,
			PDB.UID.Data4[0], PDB.UID.Data4[1], PDB.UID.Data4[2],
			PDB.UID.Data4[3], PDB.UID.Data4[4], PDB.UID.Data4[5],
			PDB.UID.Data4[6], PDB.UID.Data4[7], PDB.Age
		);

		sprintf_s(
			SymURL, 128, "%s/%s/%s/%s",
			"/download/symbols",
			PDB.PDBFileName, PDBGUID, PDB.PDBFileName
		);

		sprintf_s(
			SymPath, 128, "%s%s",
			"D:\\", PDB.PDBFileName
		);

		/*File = fopen(SymPath, "wb+");
		if (File) {
			fclose(File);
			return TRUE;
		}*/
		if (DownloadSymbol(SymURL, SymPath)) {
			return TRUE;
		}
	}
	return FALSE;
}

int main(int argc,char* argv[]) {
	if (argc < 2) {
		return 0;
	}
	HANDLE hFile = ::CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;
	HANDLE hMemMap = ::CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!hMemMap)
		return 0;

	ULONG64 imageBase = (ULONG64)::MapViewOfFile(hMemMap, FILE_MAP_READ, 0, 0, 0);
	GetSymbol(imageBase);

	system("pause");
	return 0;
}