#pragma once

#define PHNT_MODE PHNT_MODE_USER
#define PHNT_VERSION PHNT_THRESHOLD	// Windows 10

#include <phnt_windows.h>
#include <phnt.h>

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7

#define INITGUID	// include all GUIDs

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <tcpmib.h>
#include <tdh.h>



#include <wil\resource.h>
#include <SetupAPI.h>

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

#pragma comment(lib,"ntdll")

