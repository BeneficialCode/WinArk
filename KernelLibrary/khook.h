#pragma once
#include "LinkedList.h"
#include "typesdefs.h"

#pragma pack(push,1)

struct OpcodesInfo {
#ifdef _WIN64
	unsigned short int mov = 0xB848;
#else
	unsigned char mov = 0xB8;
#endif // _WIN64
	ULONG_PTR addr;				// mov rax,addr
	unsigned char push = 0x50;	// push rax
	unsigned char ret = 0xc3;	// ret
};

struct InlineOpcodesInfo {
    unsigned char push = 0x68;
    ULONG lowAddr;                      // push xxxxxxxx
#ifdef _WIN64
    unsigned char op1 = 0xc7;
    unsigned char op2 = 0x44;
    unsigned char op3 = 0x24;
    unsigned char op4 = 0x04;
    ULONG highAddr;                     // mov dword ptr[rsp+4],xxxxxxxx
#endif
    unsigned char ret = 0xc3;           // ret
    unsigned short int nop = 0x9090;    // nop
};
#pragma pack(pop)


struct HookInfo {
	ULONG_PTR Address;
	OpcodesInfo Opcodes;
	unsigned char Original[sizeof(OpcodesInfo)];
	// SSDT and shadow SSDT extension
	int Index;
	LONG Old;
	LONG New;
	ULONG_PTR OriginalAddress;
};

struct InlineHookInfo {
    ULONG_PTR Address;
    InlineOpcodesInfo Opcodes;
    unsigned char Original[sizeof(InlineOpcodesInfo)];
};

struct SystemServiceTable {
	PULONG ServiceTableBase;
	PVOID ServiceCounterTableBase; 
#ifdef _WIN64
	ULONGLONG NumberOfServices;
#else
	ULONG NumberOfServices;
#endif // _WIN64
	PVOID ParamTableBase;
};



typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    HANDLE Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

typedef enum _KTHREAD_STATE {
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
    GateWaitObsolete,
    WaitingForProcessInSwap,
    MaximumThreadState
} KTHREAD_STATE, * PKTHREAD_STATE;

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    KTHREAD_STATE ThreadState;
    KWAIT_REASON WaitReason;
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER WorkingSetPrivateSize; // since VISTA
    ULONG HardFaultCount; // since WIN7
    ULONG NumberOfThreadsHighWatermark; // since WIN7
    ULONGLONG CycleTime; // since WIN7
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR UniqueProcessKey; // since VISTA (requires SystemExtendedProcessInformation)
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SYSTEM_THREAD_INFORMATION Threads[1]; // SystemProcessInformation
    // SYSTEM_EXTENDED_THREAD_INFORMATION Threads[1]; // SystemExtendedProcessinformation
    // SYSTEM_EXTENDED_THREAD_INFORMATION + SYSTEM_PROCESS_INFORMATION_EXTENSION // SystemFullProcessInformation
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;


struct ShadowServiceNameEntry {
    UNICODE_STRING Name;
    ULONG Index;
    LIST_ENTRY Entry;

    void Free() {
        if (Name.Buffer) {
            //DbgPrint("[Library]FreeType:NameBuffer Ptr:%llx\n", (ULONG_PTR)Name.Buffer);
            ExFreePool(Name.Buffer);
            Name.Buffer = nullptr;
        }
    }
};

extern "C"
NTSTATUS NTAPI ZwQuerySystemInformation(
	_In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Inout_   PVOID                    SystemInformation,
	_In_      ULONG                    SystemInformationLength,
	_Out_opt_ PULONG                   ReturnLength
);

enum class HookType {
    SSDT,
    ShadowSSDT
};

//#define VIRTUAL_ADDRESS_BITS 48
//#define VIRTUAL_ADDRESS_MASK ((((ULONG_PTR)1) << VIRTUAL_ADDRESS_BITS) - 1)
//
//#define MiGetPteAddress(va) \
//    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PTI_SHIFT) << PTE_SHIFT) + PTE_BASE))
//
//#define MiGetPxeAddress(va)   ((PMMPTE)PXE_BASE + MiGetPxeOffset(va))
//
//#define MiGetPpeAddress(va)   \
//    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PPI_SHIFT) << PTE_SHIFT) + PPE_BASE))
//
//#define MiGetPdeAddress(va)  \
//    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PDI_SHIFT) << PTE_SHIFT) + PDE_BASE))

class khook{
public:
	
	//PVOID GetFunctionAddress(const char* apiName);
	static PVOID GetApiAddress(PCWSTR name);

    static bool GetApiAddress(ULONG index, PVOID* address);
    static bool GetShadowApiAddress(ULONG index, PVOID* address);

	static bool GetSystemServiceTable();
    static bool GetShadowSystemServiceTable();
    ULONG GetShadowServiceLimit();

	bool HookSSDT(const char* apiName,void* newfunc);
    bool UnhookSSDT();

    bool HookShadowSSDT(PUNICODE_STRING apiName, void* newfunc);
    bool UnhookShadowSSDT();

	bool GetSystemServiceNumber(const char* exportName, PULONG index);
	static bool GetKernelAndWin32kBase();
    bool GetShadowSystemServiceNumber(PUNICODE_STRING symbolName, PULONG index);
    
    static bool SearchSessionProcess();

    static bool GetSectionStart(ULONG_PTR va);
    static bool GetShadowSectionStart(ULONG_PTR va);
    static PVOID FindCaveAddress(PVOID start, ULONG size, ULONG caveSize);

	bool Hook(PVOID api, void* newfunc);
	bool Unhook();
	NTSTATUS RtlSuperCopyMemory(_In_ VOID UNALIGNED* Destination, _In_ VOID UNALIGNED* Source, _In_ ULONG Length);

    PVOID GetOriginalFunction(HookType type,const char* apiName,PUNICODE_STRING symbolName);

    // 地址必须16字节对齐
    bool HookKernelApi(PVOID api,void* newfunc,bool first);
    bool UnhookKernelApi(bool end);
    NTSTATUS SecureExchange(PVOID opcodes);

    static void DetectInlineHook(KInlineData* pInfo,KernelInlineHookData* pData);
    static ULONG GetInlineHookCount(ULONG_PTR base);

private:
	HookInfo* _info = nullptr;
    InlineHookInfo* _inlineInfo = nullptr;

public:
	static SystemServiceTable* _ntTable;
    static SystemServiceTable* _win32kTable;
    static PVOID _kernelImageBase;
    static ULONG _kernelImageSize;
    static PVOID _win32kImageBase;
    static ULONG _win32kImageSize;
    static PVOID _sectionStart;
    static ULONG _sectionSize;
    static PVOID _shadowSectionStart;
    static ULONG _shadowSectionSize;
    static LinkedList<ShadowServiceNameEntry> _shadowServiceNameList;
    static HANDLE _pid;
    bool _success = false;
};

