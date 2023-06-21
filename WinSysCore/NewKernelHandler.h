#pragma once


struct NewKernelHandler {
	static void InitMaxPhyMask();

	// number of bits a physical address can be made up of
	static BYTE _MaxPhyAddr;
	// mask to AND with a physical address to strip invalid bits
	static ULONG_PTR _MaxPhyAddrMask{ 0xfffffffff };
	// same as MAXPHYADDRMASK but also aligns to strip invalid bits
	static ULONG_PTR _MaxPhyAddrMaskPb{ 0xffffff000 };
	static ULONG_PTR _MaxPhyAddrMaskPbBig{ 0xfffe00000 };

	// number of bits in a virtual address. All bits after it have to match
	static BYTE _MaxLinearAddr;
	static ULONG_PTR _MaxLinearAddrTest;
	static ULONG_PTR _MaxLinearAddrMask;

	bool VerifyAddress(ULONG_PTR addr);
};