#pragma once
// https://github.com/kanren3/reflector
#include <Zydis/Zydis.h>

#define REPAIR_NONE            0x00000000
#define REPAIR_REL             0x00000001
#define REPAIR_JCC             0x00000002
#define REPAIR_JMP             0x00000004
#define REPAIR_CALL            0x00000008

#define BRANCH_LOCAL           0x00010000
#define BRANCH_REMOTE          0x00020000
#define BRANCH_IMP             0x00040000

#define REGISTER_BITMAP_EAX     0x00000001
#define REGISTER_BITMAP_ECX     0x00000002
#define REGISTER_BITMAP_EDX     0x00000004
#define REGISTER_BITMAP_EBX     0x00000008
#define REGISTER_BITMAP_ESI     0x00000010
#define REGISTER_BITMAP_EDI     0x00000020
#define REGISTER_BITMAP_EBP     0x00000040

typedef struct _ANALYZER {
    // 解码信息
    struct {
        PUCHAR Address;
        ZyanUSize Length;
        ZydisDecodedInstruction Instruction;
        ZydisDecodedOperand Operands[ZYDIS_MAX_OPERAND_COUNT];
    }Decoded;
    // 编码信息
    struct {
        PUCHAR Address;
        ZyanUSize Length;
        ZydisEncoderRequest Request;
    }Encoder;
    struct {
        ULONG Flags;
        ULONG Index;
        PUCHAR Address;
    }Repair;
}ANALYZER, * PANALYZER;

typedef PVOID(NTAPI* REPAIR_CALLBACK)(_In_ ULONG Flags,
    _In_opt_ PVOID Address);

PVOID Reflector(_In_ PUCHAR Address,
    _In_ ULONG Length,
    _In_opt_ REPAIR_CALLBACK Callback);

PUCHAR ReflectCode(_In_ PUCHAR Address, _In_ ULONG Length, 
    _In_ PUCHAR pTrampoline, _In_ ULONG Size);

ULONG GetAnalyzerCount(_In_ PUCHAR Address, _In_ ULONG Length);

PANALYZER BuildAnalyzer(_In_ PUCHAR Address, _In_ ULONG Count);

VOID BuildAllRepairInfo(_In_ PANALYZER pAnalyzers, _In_ ULONG Count);

NTSTATUS BuildBranchType(_In_ PANALYZER Analyzer);

NTSTATUS BuildBranchAnalyzerId(_In_ PANALYZER pAnalyzers,
    _In_ ULONG Count, _In_ PANALYZER pAnalyzer);

NTSTATUS BuildFirstProcedure(_In_ PUCHAR Procedure, _In_ PANALYZER pAnalyzers,
    _In_ ULONG Count, _In_opt_ REPAIR_CALLBACK Callback);

PUCHAR BuildTruncationInstruction(_In_ PANALYZER pAnalyzer);

PUCHAR BuildNormalInstruction(_In_ PUCHAR pPointer, _In_ PANALYZER pAnalyzer);

PUCHAR BuildRelativeInstruction(_In_ PUCHAR pPointer, _In_ PANALYZER pAnalyzer);

ZydisRegister SelectIdleRegister(_In_ PANALYZER pAnalyzer);

ULONG CreateRegisterBitmap(_In_ PANALYZER pAnalyzer);

PUCHAR BuildInstructionFromRequest(_In_ PUCHAR pPointer,
    _In_ ULONG Count, _In_ ZydisEncoderRequest* Requests);

NTSTATUS BuildSecondProcedure(_In_ PANALYZER pAnalyzers, _In_ ULONG Count);

PUCHAR BuildLocalInstruction(_In_ PANALYZER pAnalyzers, _In_ PANALYZER pAnalyzer);

PUCHAR BuildRemoteJccInstruction(_In_ PANALYZER pAnalyzer);

PUCHAR BuildRemoteJmpInstruction(_In_ PANALYZER pAnalyzer);

PUCHAR BuildImpJmpInstruction(_In_ PANALYZER pAnalyzer);

PUCHAR BuildRemoteCallInstruction(_In_ PANALYZER pAnalyzer);

PUCHAR BuildImpCallInstruction(_In_ PANALYZER pAnalyzer);