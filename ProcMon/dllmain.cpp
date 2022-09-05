// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

NTSTATUS LogToFileFmt(const char* pstrFmt, ...);

PTEB GetTeb();
PPEB GetPeb(PTEB pTeb);


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        PTEB pTEB = GetTeb();
        PPEB pPEB = GetPeb(pTEB);

        // Get current PID
        CLIENT_ID* pCID = (CLIENT_ID*)((BYTE*)pTEB + sizeof(NT_TIB) + sizeof(void*));
        ULONG uiPID = (ULONG)(ULONG_PTR)pCID->UniqueProcess;

        // Get current time
        LARGE_INTEGER liSt = {};
        NtQuerySystemTime(&liSt);
        RtlSystemTimeToLocalTime(&liSt, &liSt);
        TIME_FIELDS tfSt = {};
        RtlTimeToTimeFields(&liSt, &tfSt);

        //We can't use SEH!
        if (pPEB->ProcessParameters)
        {
            //Simply output where we ran from
            LogToFileFmt("%04u-%02u-%02u %02u:%02u:%02u.%03u > PID=%u: \"%wZ\"\n"
                ,
                tfSt.Year, tfSt.Month, tfSt.Day, tfSt.Hour, tfSt.Minute, tfSt.Second, tfSt.Milliseconds,
                uiPID, &pPEB->ProcessParameters->ImagePathName
            );
        }
        else
        {
            //No path
            LogToFileFmt("%04u-%02u-%02u %02u:%02u:%02u.%03u > PID=%u: no path\n"
                ,
                tfSt.Year, tfSt.Month, tfSt.Day, tfSt.Hour, tfSt.Minute, tfSt.Second, tfSt.Milliseconds,
                uiPID
            );
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

NTSTATUS LogToFile(const char* pOutput,LPCTSTR pFile) {
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (pOutput && pOutput[0]) {
        // Get length of output
        UINT cbSize = 0;
        while (pOutput[cbSize])
            cbSize++;

        // Convert to NT file path
        UNICODE_STRING uStrFile;
        status = RtlDosPathNameToNtPathName_U_WithStatus(pFile,&uStrFile,nullptr,nullptr);
        if (NT_SUCCESS(status)) {
            // INFO: The use of the FILE_APPEND_DATA flag will make writing into our file atomic
            HANDLE hFile;
            OBJECT_ATTRIBUTES oa = { sizeof(oa),0,&uStrFile,OBJ_CASE_INSENSITIVE };
            IO_STATUS_BLOCK iosb;
            status = NtCreateFile(&hFile, FILE_APPEND_DATA | SYNCHRONIZE, &oa, &iosb, 0,
                FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, nullptr, NULL);

            if (NT_SUCCESS(status)) {

                status = NtWriteFile(hFile, nullptr, nullptr, nullptr,
                    &iosb, (PVOID)pOutput, cbSize, nullptr, nullptr);

                // Close file
                NtClose(hFile);
            }
            // Free string
            RtlFreeUnicodeString(&uStrFile);
        }
    }

    return status;
}

NTSTATUS LogToFileFmt(const char* pstrFmt, ...) {
    va_list argList;
    va_start(argList, pstrFmt);

    char buff[512];
    buff[0] = 0;
    vsprintf_s(buff,sizeof(buff),pstrFmt,argList);
    buff[sizeof(buff) - 1] = 0; // safety null
    NTSTATUS status = LogToFile(buff, DBG_FILE_PATH);

    va_end(argList);


    return status;
}

PTEB GetTeb() {
#ifdef _WIN64
    return (PTEB)__readgsqword((ULONG)offsetof(NT_TIB, Self));
#elif _WIN32
    return (PTEB)__readfsdword((ULONG)offsetof(NT_TIB, Self));
#else
#error unupported_CPU
#endif // _WIN64

}

PPEB GetPeb(PTEB pTeb) {
    return pTeb->ProcessEnvironmentBlock;
}