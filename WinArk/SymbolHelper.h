#pragma once
#include "SymbolHandler.h"

class SymbolHelper {
public:
	static SymbolHelper& Get();
	~SymbolHelper();

	std::unique_ptr<SymbolInfo> GetSymbolFromAddress(DWORD64 address, PDWORD64 offset = nullptr);
	ULONG64 GetKernelSymbolAddressFromName(PCSTR name);
	ULONG64 GetWin32kSymbolAddressFromName(PCSTR name);
	DWORD GetKernelStructMemberOffset(std::string name, std::string memberName);
private:
	SymbolHelper();


	static inline DWORD _win32kSize, _kernelSize;
	static inline DWORD64 _win32kBase, _kernelBase;
	static inline std::string _win32kPdb, _kernelPdb;
	static inline std::string _win32kModule, _kernelModule;
};