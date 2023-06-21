#include "pch.h"
#include "NewKernelHandler.h"

void NewKernelHandler::InitMaxPhyMask(){
	INT cpuInfo[4];// eax,ebx,ecx,edx
	CpuIdEx(cpuInfo, 0x80000008, 0);
	if (cpuInfo[0] != 0) {
		_MaxPhyAddr = cpuInfo[0] & 0xFF;
		_MaxPhyAddrMask = 0xffffffffffffffff;
		_MaxPhyAddrMask = _MaxPhyAddrMask >> _MaxPhyAddr;
		_MaxPhyAddrMask = ~(_MaxPhyAddrMask << _MaxPhyAddr);
		_MaxPhyAddrMaskPb = _MaxPhyAddrMask & 0xfffffffffffff000;

		_MaxLinearAddr = (cpuInfo[0] >> 8) & 0xFF;
		_MaxLinearAddrMask = 0xffffffffffffffff;
		_MaxLinearAddrMask = _MaxLinearAddrMask >> _MaxLinearAddr;
		_MaxLinearAddrMask = _MaxLinearAddrMask << _MaxLinearAddr;

		_MaxLinearAddrTest = 1ull << (_MaxLinearAddr - 1);

#ifdef _WIN64
		_MaxPhyAddrMaskPbBig = _MaxPhyAddrMask & 0xffffffffffe00000;
#else
		_MaxPhyAddrMaskPbBig = _MaxPhyAddrMask & 0xffffffffffc00000;
#endif // _WIN64
	}
	else {
		_MaxPhyAddrMask = 0xffffffffffffffff;
		_MaxLinearAddrMask = 0xffffffffffffffff;
	}
}

bool NewKernelHandler::VerifyAddress(ULONG_PTR addr){
	bool result = false;
	if ((addr & _MaxLinearAddrTest) > 0) {
		result = (addr & _MaxLinearAddrMask) == _MaxLinearAddrMask;
	}
	else {
		result = (addr & _MaxLinearAddrMask) == 0;
	}
	return result;
}