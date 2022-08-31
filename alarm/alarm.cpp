// alarm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <string>
#include "..\WinArkSvc\ServiceCommon.h"

int Error(const char* msg) {
    printf("%s (%u)\n", msg, ::GetLastError());
    return 1;
}

int main(int argc,const char* argv[]){
    if (argc < 2) {
        printf("Usage: alarm <set hh::mm::ss | cancel>\n");
        return 0;
    }

    HANDLE hFile = ::CreateFile(L"\\\\.\\mailslot\\winark", GENERIC_WRITE|GENERIC_READ, 0, nullptr, OPEN_EXISTING,
        0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return Error("Failed to open mailslot");

    DWORD bytes;

    if (_stricmp(argv[1], "set") == 0 && argc > 2) {
        AlarmMessage msg;
        msg.Type = MessageType::SetAlarm;
        std::string time(argv[2]);
        if (time.size() != 8) {
            ::CloseHandle(hFile);
            printf("Time format is illegal.\n");
            return 1;
        }
        //
        // get local time (for obtaining the date)
        //
        SYSTEMTIME st;
        ::GetLocalTime(&st);

        //
        // change the time based on user input
        //
        st.wHour = atoi(time.substr(0, 2).c_str());
        st.wMinute = atoi(time.substr(3, 2).c_str());
        st.wSecond = atoi(time.substr(6, 2).c_str());
        ::SystemTimeToFileTime(&st, &msg.Time);

        //
        // convert to UTC time
        //
        ::LocalFileTimeToFileTime(&msg.Time, &msg.Time);

        //
        // write to the mailsolt
        //
        if (!::WriteFile(hFile, &msg, sizeof(msg), &bytes, nullptr)) {
            ::CloseHandle(hFile);
            return Error("failed in WriteFile");
        }
    }
    else if (_stricmp(argv[1], "cancel") == 0) {
        AlarmMessage msg;
        msg.Type = MessageType::CancelAlarm;
        if (!::WriteFile(hFile, &msg, sizeof(msg), &bytes, nullptr)) {
            ::CloseHandle(hFile);
            return Error("failed in WriteFile");
        }
    }
    else {
        printf("Error in command line arguments.\n");
    }
    ::CloseHandle(hFile);

    return 0;
}


