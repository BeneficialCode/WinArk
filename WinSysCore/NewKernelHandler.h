#pragma once


struct NewKernelHandler {
	static void InitMaxPhyMask();

	// number of bits a physical address can be made up of
	static inline BYTE _MaxPhyAddr;
	// mask to AND with a physical address to strip invalid bits
	static inline ULONG64 _MaxPhyAddrMask{ 0xfffffffff };
	// same as MAXPHYADDRMASK but also aligns to strip invalid bits
	static inline ULONG64 _MaxPhyAddrMaskPb{ 0xffffff000 };
	static inline ULONG64 _MaxPhyAddrMaskPbBig{ 0xfffe00000 };

	// number of bits in a virtual address. All bits after it have to match
	static inline BYTE _MaxLinearAddr;
	static inline ULONG_PTR _MaxLinearAddrTest;
	static inline ULONG_PTR _MaxLinearAddrMask;

	bool VerifyAddress(ULONG_PTR addr);
};