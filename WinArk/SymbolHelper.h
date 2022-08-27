#pragma once
#include "SymbolHandler.h"


class SymbolHelper final{
public:
	static std::unique_ptr<SymbolInfo> GetSymbolFromAddress(DWORD64 address, PDWORD64 offset = nullptr);
	static ULONG64 GetKernelSymbolAddressFromName(PCSTR name);
	static ULONG64 GetWin32kSymbolAddressFromName(PCSTR name);
	static DWORD GetKernelStructMemberOffset(std::string name, std::string memberName);
	static DWORD GetKernelSturctMemberSize(std::string name, std::string memberName);
	static DWORD GetKernelStructSize(std::string name);
	static void Init();
private:
	
	static inline DWORD _win32kSize, _kernelSize;
	static inline DWORD64 _win32kBase, _kernelBase;
	static inline std::string _win32kPdb, _kernelPdb;
	static inline std::string _win32kModule, _kernelModule;
	static inline SymbolHandler _win32k;
	static inline SymbolHandler _kernel;
};