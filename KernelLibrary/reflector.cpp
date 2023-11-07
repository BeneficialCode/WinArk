#include "pch.h"
#include "reflector.h"

static ZydisDecoder s_Decoder;

static SIZE_T s_TotalSize = 0;
static SIZE_T s_EncodeSize = 0;

PVOID NTAPI Reflector(_In_ PUCHAR Address,
    _In_ ULONG Length,
    _In_opt_ REPAIR_CALLBACK Callback){
    ULONG count = 0;
    PANALYZER pAnalyzers = NULL;
    
    PUCHAR procedure = NULL;

    NTSTATUS status = STATUS_SUCCESS;

    SIZE_T size = max(Length * 4, PAGE_SIZE);

    do
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&s_Decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
            break;
        procedure = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, size, 'corp');
        if (NULL == procedure) {
            break;
        }
        RtlFillMemory(procedure, size, 0xCC);
        // 获取重构指令的数量
        count = GetAnalyzerCount(Address, Length);
        pAnalyzers = BuildAnalyzer(Address, count);
        if (NULL == pAnalyzers){
            break;
        }
        BuildAllRepairInfo(pAnalyzers, count);
        status = BuildFirstProcedure(procedure, pAnalyzers, count, Callback);
        if (!NT_SUCCESS(status)) {
            break;
        }
        status = BuildSecondProcedure(pAnalyzers, count);
        if (!NT_SUCCESS(status)) {
            break;
        }
    } while (FALSE);

    NT_ASSERT(s_TotalSize < size);

    if (NULL != pAnalyzers) {
        ExFreePool(pAnalyzers);
        pAnalyzers = NULL;
    }

    if (!NT_SUCCESS(status)) {
        if (NULL != procedure)
            ExFreePool(procedure);
        procedure = NULL;
    }
    return procedure;
}

PUCHAR ReflectCode(_In_ PUCHAR Address, _In_ ULONG Length,
    _In_ PUCHAR pTrampoline, _In_ ULONG Size) {
    ULONG count = 0;
    PANALYZER pAnalyzers = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    s_TotalSize = 0;

    do
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&s_Decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
            break;
        // 获取重构指令的数量
        count = GetAnalyzerCount(Address, Length);
        pAnalyzers = BuildAnalyzer(Address, count);
        if (NULL == pAnalyzers) {
            break;
        }
        BuildAllRepairInfo(pAnalyzers, count);
        status = BuildFirstProcedure(pTrampoline, pAnalyzers, count, NULL);
        if (!NT_SUCCESS(status)) {
            break;
        }
        status = BuildSecondProcedure(pAnalyzers, count);
        if (!NT_SUCCESS(status)) {
            break;
        }
    } while (FALSE);

    NT_ASSERT(s_TotalSize < Size);

    if (NULL != pAnalyzers) {
        ExFreePool(pAnalyzers);
        pAnalyzers = NULL;
    }

    if (!NT_SUCCESS(status)) {
        return pTrampoline;
    }
    return pTrampoline + s_TotalSize;
}

ULONG GetAnalyzerCount(_In_ PUCHAR Address, _In_ ULONG Length) {
    ZyanStatus status = ZYAN_STATUS_SUCCESS;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT] = { 0 };
    ZyanUSize offset = 0;
    ULONG count = 0;

    do
    {
        status = ZydisDecoderDecodeFull(&s_Decoder,
            Address + offset,
            Length - offset, &instruction,
            operands);

        if (!ZYAN_SUCCESS(status))
        {
            break;
        }

        count += 1;
        offset += instruction.length;
    } while (FALSE != ZYAN_SUCCESS(status));

    return count;
}

PANALYZER BuildAnalyzer(_In_ PUCHAR Address, _In_ ULONG Count) {
    ZyanStatus status = ZYAN_STATUS_SUCCESS;
    ZyanUSize offset = 0;
    PANALYZER pAnalyzers = NULL;

    ULONG idx = 0;

    if (Count <= 0) {
        return NULL;
    }

    pAnalyzers = (PANALYZER)ExAllocatePoolWithTag(PagedPool, sizeof(ANALYZER) * Count, 'oted');
    if (NULL != pAnalyzers) {
        RtlZeroMemory(pAnalyzers, sizeof(ANALYZER) * Count);
        for (idx = 0; idx < Count; idx++) {
            status = ZydisDecoderDecodeFull(&s_Decoder,
                Address + offset,
                ZYDIS_MAX_INSTRUCTION_LENGTH,
                &pAnalyzers[idx].Decoded.Instruction,
                pAnalyzers[idx].Decoded.Operands);
            if (!ZYAN_SUCCESS(status))
            {
                break;
            }

            status = ZydisEncoderDecodedInstructionToEncoderRequest(
                &pAnalyzers[idx].Decoded.Instruction,
                pAnalyzers[idx].Decoded.Operands,
                pAnalyzers[idx].Decoded.Instruction.operand_count_visible,
                &pAnalyzers[idx].Encoder.Request);

            if (!ZYAN_SUCCESS(status)) {
                break;
            }

            pAnalyzers[idx].Decoded.Address= Address + offset;
            pAnalyzers[idx].Decoded.Length= pAnalyzers[idx].Decoded.Instruction.length;
            offset += pAnalyzers[idx].Decoded.Length;
        }

        if (idx != Count) {
            ExFreePool(pAnalyzers);
            pAnalyzers = NULL;
        }
    }

    return pAnalyzers;
}

VOID BuildAllRepairInfo(_In_ PANALYZER pAnalyzers, _In_ ULONG Count) {
    ULONG idx = 0;
    PANALYZER pAnalyzer = NULL;

    for (idx = 0; idx < Count; idx++) {
        pAnalyzer = &pAnalyzers[idx];

        NTSTATUS status = BuildBranchType(pAnalyzer);
        if (NT_SUCCESS(status)) {
            if (ZYDIS_ATTRIB_HAS_MODRM & pAnalyzer->Decoded.Instruction.attributes) {
                if (0 == pAnalyzer->Decoded.Instruction.raw.modrm.mod
                    && 5 == pAnalyzer->Decoded.Instruction.raw.modrm.rm) {
                    pAnalyzer->Repair.Address =
                        pAnalyzer->Decoded.Address +
                        pAnalyzer->Decoded.Length +
                        pAnalyzer->Decoded.Instruction.raw.disp.value;
                    pAnalyzer->Repair.Flags |= BRANCH_IMP;
                }
                else {
                    pAnalyzer->Repair.Flags = REPAIR_NONE;
                }
            }
            else {
                status = BuildBranchAnalyzerId(pAnalyzers, Count, pAnalyzer);
                if (NT_SUCCESS(status)) {
                    pAnalyzer->Repair.Flags |= BRANCH_LOCAL;
                }
                else {
                    pAnalyzer->Repair.Address =
                        pAnalyzer->Decoded.Address +
                        pAnalyzer->Decoded.Length +
                        pAnalyzer->Decoded.Instruction.raw.imm->value.s;
                    pAnalyzer->Repair.Flags |= BRANCH_REMOTE;
                }
            }
        }
        else {
            if (ZYDIS_ATTRIB_HAS_MODRM & pAnalyzer->Decoded.Instruction.attributes) {
                if (0 == pAnalyzer->Decoded.Instruction.raw.modrm.mod &&
                    5 == pAnalyzer->Decoded.Instruction.raw.modrm.rm) {
                    pAnalyzer->Repair.Address =
                        pAnalyzer->Decoded.Address +
                        pAnalyzer->Decoded.Length +
                        pAnalyzer->Decoded.Instruction.raw.disp.value;

                    pAnalyzer->Repair.Flags = REPAIR_REL;
                }
                else {
                    pAnalyzer->Repair.Flags = REPAIR_NONE;
                }
            }
            else {
                pAnalyzer->Repair.Flags = REPAIR_NONE;
            }
        }
    }
}

NTSTATUS BuildBranchType(_In_ PANALYZER Analyzer) {
    switch (Analyzer->Decoded.Instruction.mnemonic)
    {
    case ZYDIS_MNEMONIC_JB:
    case ZYDIS_MNEMONIC_JBE:
    case ZYDIS_MNEMONIC_JCXZ:
    case ZYDIS_MNEMONIC_JECXZ:
    case ZYDIS_MNEMONIC_JKNZD:
    case ZYDIS_MNEMONIC_JKZD:
    case ZYDIS_MNEMONIC_JL:
    case ZYDIS_MNEMONIC_JLE:
    case ZYDIS_MNEMONIC_JNB:
    case ZYDIS_MNEMONIC_JNBE:
    case ZYDIS_MNEMONIC_JNL:
    case ZYDIS_MNEMONIC_JNLE:
    case ZYDIS_MNEMONIC_JNO:
    case ZYDIS_MNEMONIC_JNP:
    case ZYDIS_MNEMONIC_JNS:
    case ZYDIS_MNEMONIC_JNZ:
    case ZYDIS_MNEMONIC_JO:
    case ZYDIS_MNEMONIC_JP:
    case ZYDIS_MNEMONIC_JRCXZ:
    case ZYDIS_MNEMONIC_JS:
    case ZYDIS_MNEMONIC_JZ:
        Analyzer->Repair.Flags = REPAIR_JCC;
        break;
    case ZYDIS_MNEMONIC_JMP:
        Analyzer->Repair.Flags = REPAIR_JMP;
        break;
    case ZYDIS_MNEMONIC_CALL:
        Analyzer->Repair.Flags = REPAIR_CALL;
        break;
    default:
        Analyzer->Repair.Flags = REPAIR_NONE;
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS BuildBranchAnalyzerId(_In_ PANALYZER pAnalyzers,
    _In_ ULONG Count, _In_ PANALYZER pAnalyzer) {
    ULONG idx = 0;

    for (idx = 0; idx < Count; idx++) {
        if (pAnalyzers[idx].Decoded.Address ==
            pAnalyzer->Decoded.Address +
            pAnalyzer->Decoded.Length +
            pAnalyzer->Decoded.Instruction.raw.imm->value.s) {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS BuildFirstProcedure(_In_ PUCHAR Procedure, _In_ PANALYZER pAnalyzers,
    _In_ ULONG Count, _In_opt_ REPAIR_CALLBACK Callback) {
    ULONG idx = 0;
    PANALYZER pAnalyzer = NULL;
    PUCHAR pPointer = NULL;

    pPointer = Procedure;

    for (idx = 0; idx < Count; idx++) {
        pAnalyzer = &pAnalyzers[idx];
        if (NULL != Callback) {
            if (0 != (BRANCH_REMOTE & pAnalyzer->Repair.Flags) ||
                0 != (BRANCH_IMP & pAnalyzer->Repair.Flags) ||
                0 != (REPAIR_REL & pAnalyzer->Repair.Flags)) {
                Callback(
                    pAnalyzer->Repair.Flags,
                    pAnalyzer->Repair.Address);
            }
        }
        switch (pAnalyzer->Repair.Flags)
        {
        case REPAIR_NONE:
            // 拷贝原始指令
            pPointer = BuildNormalInstruction(pPointer, pAnalyzer);
            break;

        case REPAIR_REL:
            pPointer = BuildRelativeInstruction(pPointer, pAnalyzer);
            break;

        case REPAIR_JCC | BRANCH_LOCAL:
            pAnalyzer->Encoder.Length = 6;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;

        case REPAIR_JCC | BRANCH_REMOTE:
            pAnalyzer->Encoder.Length = 20;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;

        case REPAIR_JMP | BRANCH_LOCAL:
            pAnalyzer->Encoder.Length = 5;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;

        case REPAIR_JMP | BRANCH_REMOTE:
            pAnalyzer->Encoder.Length = 16;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;

        case REPAIR_JMP | BRANCH_IMP:
            pAnalyzer->Encoder.Length = 19;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;

        case REPAIR_CALL | BRANCH_LOCAL:
            pAnalyzer->Encoder.Length = 5;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;

        case REPAIR_CALL | BRANCH_REMOTE:
            pAnalyzer->Encoder.Length = 23;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;

        case REPAIR_CALL | BRANCH_IMP:
            pAnalyzer->Encoder.Length = 26;
            pAnalyzer->Encoder.Address = pPointer;
            pPointer += pAnalyzer->Encoder.Length;
            break;
        default:
            break;
        }

        if (NULL == pPointer) {
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (NULL != pAnalyzer) {
        // 构建跳转回去的代码
        PUCHAR pInsn = BuildTruncationInstruction(pAnalyzer);
        if (NULL == pInsn)
            return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

PUCHAR BuildTruncationInstruction(_In_ PANALYZER pAnalyzer) {
    UCHAR code[] ={
        // push rax
        0x50,
        // mov rax,0x8877665544332211
        0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        // xchg qword ptr ss:[rsp],rax
        0x48, 0x87, 0x04, 0x24,
        // ret
        0xC3,
    };

    *(PUCHAR*)&code[3] = pAnalyzer->Decoded.Address + pAnalyzer->Decoded.Length;
    
    RtlCopyMemory(pAnalyzer->Encoder.Address + pAnalyzer->Encoder.Length,
        code,
        sizeof(code));

    s_TotalSize += sizeof(code) + pAnalyzer->Encoder.Length;

    return pAnalyzer->Encoder.Address;
}

PUCHAR BuildNormalInstruction(_In_ PUCHAR pPointer, _In_ PANALYZER pAnalyzer) {
    RtlCopyMemory(pPointer, pAnalyzer->Decoded.Address,
        pAnalyzer->Decoded.Length);

    s_TotalSize += pAnalyzer->Decoded.Length;

    pAnalyzer->Encoder.Address = pPointer;
    pAnalyzer->Encoder.Length = pAnalyzer->Decoded.Length;
    return pPointer + pAnalyzer->Encoder.Length;
}

PUCHAR BuildRelativeInstruction(_In_ PUCHAR pPointer, _In_ PANALYZER pAnalyzer) {
    ZydisRegister idleRegister = ZYDIS_REGISTER_NONE;
    SIZE_T size = sizeof(ZydisEncoderRequest) * 4;
    ZydisEncoderRequest* pRequests = (ZydisEncoderRequest*)ExAllocatePoolWithTag(NonPagedPool, 
       size, 'tqer');
    if (NULL == pRequests) {
        return NULL;
    }
    ZyanU32 idx = 0;

    RtlZeroMemory(pRequests, size);

    idleRegister = SelectIdleRegister(pAnalyzer);

    if (ZYDIS_REGISTER_NONE != idleRegister) {
        for (idx = 0; idx < pAnalyzer->Encoder.Request.operand_count; idx++) {
            if (ZYDIS_OPERAND_TYPE_MEMORY == pAnalyzer->Encoder.Request.operands[idx].type) {
                pAnalyzer->Encoder.Request.operands[idx].mem.base = idleRegister;
                pAnalyzer->Encoder.Request.operands[idx].mem.displacement = 0;
            }
        }

        pRequests[0].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
        pRequests[0].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
        pRequests[0].mnemonic = ZYDIS_MNEMONIC_PUSH;
        pRequests[0].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
        pRequests[0].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
        pRequests[0].operand_count = 1;
        pRequests[0].operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
        pRequests[0].operands[0].reg.value = idleRegister;

        pRequests[1].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
        pRequests[1].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
        pRequests[1].mnemonic = ZYDIS_MNEMONIC_MOV;
        pRequests[1].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
        pRequests[1].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
        pRequests[1].operand_count = 2;
        pRequests[1].operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
        pRequests[1].operands[0].reg.value = idleRegister;
        pRequests[1].operands[1].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
        pRequests[1].operands[1].imm.u = (ZyanU64)pAnalyzer->Repair.Address;

        RtlCopyMemory(&pRequests[2], &pAnalyzer->Encoder.Request,
            sizeof(ZydisEncoderRequest));

        pRequests[3].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
        pRequests[3].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
        pRequests[3].mnemonic = ZYDIS_MNEMONIC_POP;
        pRequests[3].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
        pRequests[3].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
        pRequests[3].operand_count = 1;
        pRequests[3].operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
        pRequests[3].operands[0].reg.value = idleRegister;

        pAnalyzer->Encoder.Address = pPointer;

        PUCHAR pInst = BuildInstructionFromRequest(
            pAnalyzer->Encoder.Address,
            size/ sizeof(ZydisEncoderRequest),
            pRequests);

        ExFreePool(pRequests);

        return pInst;
    }

    return NULL;
}

ULONG CreateRegisterBitmap(_In_ PANALYZER pAnalyzer) {
    ULONG idx = 0;
    ULONG bitmap = 0;

    for (idx = 0; idx < pAnalyzer->Decoded.Instruction.operand_count; idx++) {
        if (ZYDIS_OPERAND_TYPE_REGISTER == pAnalyzer->Decoded.Operands[idx].type) {
            switch (pAnalyzer->Decoded.Operands[idx].reg.value)
            {
            case ZYDIS_REGISTER_AL:
            case ZYDIS_REGISTER_AH:
            case ZYDIS_REGISTER_AX:
            case ZYDIS_REGISTER_EAX:
            case ZYDIS_REGISTER_RAX:
                bitmap |= REGISTER_BITMAP_EAX;
                break;

            case ZYDIS_REGISTER_CL:
            case ZYDIS_REGISTER_CH:
            case ZYDIS_REGISTER_CX:
            case ZYDIS_REGISTER_ECX:
            case ZYDIS_REGISTER_RCX:
                bitmap |= REGISTER_BITMAP_ECX;
                break;

            case ZYDIS_REGISTER_DL:
            case ZYDIS_REGISTER_DH:
            case ZYDIS_REGISTER_DX:
            case ZYDIS_REGISTER_EDX:
            case ZYDIS_REGISTER_RDX:
                bitmap |= REGISTER_BITMAP_EDX;
                break;

            case ZYDIS_REGISTER_BL:
            case ZYDIS_REGISTER_BH:
            case ZYDIS_REGISTER_BX:
            case ZYDIS_REGISTER_EBX:
            case ZYDIS_REGISTER_RBX:
                bitmap |= REGISTER_BITMAP_EBX;
                break;

            case ZYDIS_REGISTER_SIL:
            case ZYDIS_REGISTER_SI:
            case ZYDIS_REGISTER_ESI:
            case ZYDIS_REGISTER_RSI:
                bitmap |= REGISTER_BITMAP_ESI;
                break;
            case ZYDIS_REGISTER_DIL:
            case ZYDIS_REGISTER_DI:
            case ZYDIS_REGISTER_EDI:
            case ZYDIS_REGISTER_RDI:
                bitmap |= REGISTER_BITMAP_EDI;
                break;
            case ZYDIS_REGISTER_BPL:
            case ZYDIS_REGISTER_BP:
            case ZYDIS_REGISTER_EBP:
            case ZYDIS_REGISTER_RBP:
                bitmap |= REGISTER_BITMAP_EBP;
                break;

            default:
                break;
            }
        }
    }

    return bitmap;
}

ZydisRegister SelectIdleRegister(_In_ PANALYZER pAnalyzer) {
    ULONG idx = 0;
    ULONG bitmap = 0;

    bitmap = CreateRegisterBitmap(pAnalyzer);

    for (idx = 0; idx < 7; idx++) {
        if (0 == (REGISTER_BITMAP_EAX & bitmap)) {
            return ZYDIS_REGISTER_RAX;
        }
        if (0 == (REGISTER_BITMAP_ECX & bitmap)) {
            return ZYDIS_REGISTER_RCX;
        }
        if (0 == (REGISTER_BITMAP_EDX & bitmap)) {
            return ZYDIS_REGISTER_RDX;
        }
        if (0 == (REGISTER_BITMAP_EBX & bitmap)) {
            return ZYDIS_REGISTER_RBX;
        }
        if (0 == (REGISTER_BITMAP_ESI & bitmap)) {
            return ZYDIS_REGISTER_RSI;
        }
        if (0 == (REGISTER_BITMAP_EDI & bitmap)) {
            return ZYDIS_REGISTER_RDI;
        }
        if (0 == (REGISTER_BITMAP_EBP & bitmap)) {
            return ZYDIS_REGISTER_RBP;
        }
    }

    return ZYDIS_REGISTER_NONE;
}

PUCHAR BuildInstructionFromRequest(_In_ PUCHAR pPointer,
    _In_ ULONG Count, _In_ ZydisEncoderRequest* pRequests) {
    ZyanStatus status = ZYAN_STATUS_SUCCESS;
    ZyanUSize length = 0;
    ZyanUSize offset = 0;
    ULONG idx = 0;

    for (idx = 0; idx < Count; idx++) {
        length = 15;
        status = ZydisEncoderEncodeInstruction(&pRequests[idx],
            pPointer + offset, &length);
        if (!ZYAN_SUCCESS(status)) {
            return NULL;
        }
        else {
            offset += length;
        }
    }

    s_TotalSize += offset;

    return pPointer + offset;
}

NTSTATUS BuildSecondProcedure(_In_ PANALYZER pAnalyzers, _In_ ULONG Count) {
    ULONG idx = 0;
    PANALYZER pAnalyzer = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR pInsn = NULL;

    for (idx = 0; idx < Count; idx++) {
        pAnalyzer = &pAnalyzers[idx];
        switch (pAnalyzer->Repair.Flags)
        {
        case REPAIR_JCC | BRANCH_LOCAL:
        case REPAIR_JMP | BRANCH_LOCAL:
        case REPAIR_CALL | BRANCH_LOCAL:
            pInsn = BuildLocalInstruction(pAnalyzers, pAnalyzer);
            if (NULL == pInsn) {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case REPAIR_JCC | BRANCH_REMOTE:
            pInsn = BuildRemoteJccInstruction(pAnalyzer);
            if (NULL == pInsn) {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case REPAIR_JMP | BRANCH_REMOTE:
            pInsn = BuildRemoteJmpInstruction(pAnalyzer);
            if (NULL == pInsn) {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case REPAIR_JMP | BRANCH_IMP:
            pInsn = BuildImpJmpInstruction(pAnalyzer);
            if (NULL == pInsn) {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case REPAIR_CALL | BRANCH_REMOTE:
            pInsn = BuildRemoteCallInstruction(pAnalyzer);
            if (pInsn == NULL) {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case REPAIR_CALL | BRANCH_IMP:
            pInsn = BuildImpCallInstruction(pAnalyzer);
            if (pInsn == NULL) {
                status = STATUS_UNSUCCESSFUL;
            }
            break;
        }
        
        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    return status;
}

PUCHAR BuildLocalInstruction(_In_ PANALYZER pAnalyzers, _In_ PANALYZER pAnalyzer) {
    ZyanStatus status = ZYAN_STATUS_SUCCESS;
    ZyanUSize length = 0;

    pAnalyzer->Encoder.Request.branch_type = ZYDIS_BRANCH_TYPE_NEAR;
    pAnalyzer->Encoder.Request.branch_width = ZYDIS_BRANCH_WIDTH_32;
    pAnalyzer->Encoder.Request.operands[0].imm.s
        = (ZyanI64)pAnalyzers[pAnalyzer->Repair.Index].Encoder.Address
        - (ZyanI64)pAnalyzer->Encoder.Address - pAnalyzer->Encoder.Length;

    length = 15;
    status = ZydisEncoderEncodeInstruction(
        &pAnalyzer->Encoder.Request,
        pAnalyzer->Encoder.Address, &length
    );

    s_TotalSize += length;

    if (!ZYAN_SUCCESS(status)) {
        return NULL;
    }

    return pAnalyzer->Encoder.Address;
}

PUCHAR BuildRemoteJccInstruction(_In_ PANALYZER pAnalyzer) {
    SIZE_T size = sizeof(ZydisEncoderRequest) * 6;
    ZydisEncoderRequest* pRequests = (ZydisEncoderRequest*)ExAllocatePoolWithTag(NonPagedPool,
        size, 'tqer');
    if (NULL == pRequests) {
        return NULL;
    }

    RtlZeroMemory(pRequests, size);

    pRequests[0].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    pRequests[0].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
    pRequests[0].mnemonic = pAnalyzer->Encoder.Request.mnemonic;
    pRequests[0].branch_type = ZYDIS_BRANCH_TYPE_SHORT;
    pRequests[0].branch_width = ZYDIS_BRANCH_WIDTH_8;
    pRequests[0].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
    pRequests[0].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
    pRequests[0].operand_count = 1;
    pRequests[0].operands[0].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    pRequests[0].operands[0].imm.s = 2;

    pRequests[1].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    pRequests[1].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
    pRequests[1].mnemonic = ZYDIS_MNEMONIC_JMP;
    pRequests[1].branch_type = ZYDIS_BRANCH_TYPE_SHORT;
    pRequests[1].branch_width = ZYDIS_BRANCH_WIDTH_8;
    pRequests[1].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
    pRequests[1].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
    pRequests[1].operand_count = 1;
    pRequests[1].operands[0].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    pRequests[1].operands[0].imm.s = 16;

    pRequests[2].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    pRequests[2].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
    pRequests[2].mnemonic = ZYDIS_MNEMONIC_PUSH;
    pRequests[2].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
    pRequests[2].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
    pRequests[2].operand_count = 1;
    pRequests[2].operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
    pRequests[2].operands[0].reg.value = ZYDIS_REGISTER_RAX;

    pRequests[3].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    pRequests[3].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
    pRequests[3].mnemonic = ZYDIS_MNEMONIC_MOV;
    pRequests[3].branch_type = ZYDIS_BRANCH_TYPE_NONE;
    pRequests[3].branch_width = ZYDIS_BRANCH_WIDTH_NONE;
    pRequests[3].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
    pRequests[3].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
    pRequests[3].operand_count = 2;
    pRequests[3].operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
    pRequests[3].operands[0].reg.value = ZYDIS_REGISTER_RAX;
    pRequests[3].operands[1].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    pRequests[3].operands[1].imm.u = (ZyanU64)pAnalyzer->Repair.Address;

    pRequests[4].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    pRequests[4].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
    pRequests[4].mnemonic = ZYDIS_MNEMONIC_XCHG;
    pRequests[4].branch_type = ZYDIS_BRANCH_TYPE_NONE;
    pRequests[4].branch_width = ZYDIS_BRANCH_WIDTH_NONE;
    pRequests[4].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
    pRequests[4].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_32;
    pRequests[4].operand_count = 2;
    pRequests[4].operands[0].type = ZYDIS_OPERAND_TYPE_MEMORY;
    pRequests[4].operands[0].mem.base = ZYDIS_REGISTER_RSP;
    pRequests[4].operands[0].mem.index = ZYDIS_REGISTER_NONE;
    pRequests[4].operands[0].mem.scale = 0;
    pRequests[4].operands[0].mem.displacement = 0;
    pRequests[4].operands[0].mem.size = 8;
    pRequests[4].operands[1].type = ZYDIS_OPERAND_TYPE_REGISTER;
    pRequests[4].operands[1].reg.value = ZYDIS_REGISTER_RAX;

    pRequests[5].machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    pRequests[5].allowed_encodings = ZYDIS_ENCODABLE_ENCODING_LEGACY;
    pRequests[5].mnemonic = ZYDIS_MNEMONIC_RET;
    pRequests[5].branch_type = ZYDIS_BRANCH_TYPE_NEAR;
    pRequests[5].branch_width = ZYDIS_BRANCH_WIDTH_64;
    pRequests[5].address_size_hint = ZYDIS_ADDRESS_SIZE_HINT_64;
    pRequests[5].operand_size_hint = ZYDIS_OPERAND_SIZE_HINT_64;
    pRequests[5].operand_count = 0;

    PUCHAR pInst = BuildInstructionFromRequest(
        pAnalyzer->Encoder.Address,
        size / sizeof(ZydisEncoderRequest),
        pRequests);

    ExFreePool(pRequests);

    return pInst;
}

PUCHAR BuildRemoteJmpInstruction(_In_ PANALYZER pAnalyzer) {
    UCHAR code[] = {
        // push rax
        0x50,
        // mov rax,0x8877665544332211
        0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        // xchg qword ptr ss:[rsp],rax
        0x48, 0x87, 0x04, 0x24,
        // ret
        0xC3,
    };

    *(PUCHAR*)&code[3] = pAnalyzer->Repair.Address;

    RtlCopyMemory(pAnalyzer->Encoder.Address, code, sizeof(code));

    s_TotalSize += sizeof(code);

    return pAnalyzer->Encoder.Address;
}

PUCHAR BuildImpJmpInstruction(_In_ PANALYZER pAnalyzer) {
    UCHAR code[] = {
        // push rax
        0x50,
        // mov rax,0x8877665544332211
        0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        // mov rax,qword ptr ds:[rax]
        0x48, 0x8B, 0x00,
        // xchg qword ptr ss:[rsp],rax
        0x48, 0x87, 0x04, 0x24,
        // ret
        0xC3
    };

    *(PUCHAR*)&code[3] = pAnalyzer->Repair.Address;

    RtlCopyMemory(pAnalyzer->Encoder.Address, code, sizeof(code));

    s_TotalSize += sizeof(code);

    return pAnalyzer->Encoder.Address;
}

PUCHAR BuildRemoteCallInstruction(_In_ PANALYZER pAnalyzer) {
    UCHAR code[] = {
        // call rip + 2 + 5
        0xE8, 0x02, 0x00, 0x00, 0x00,
        // jmp rip + 0x10 + 2
        0xEB, 0x10,
        // push rax
        0x50,
        // mov rax,0x8877665544332211
        0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        // xchg qword ptr ss:[rsp],rax
        0x48, 0x87, 0x04, 0x24,
        // ret
        0xC3
    };

    *(PUCHAR*)&code[10] = pAnalyzer->Repair.Address;

    RtlCopyMemory(pAnalyzer->Encoder.Address, code, sizeof(code));

    s_TotalSize += sizeof(code);

    return pAnalyzer->Encoder.Address;
}


PUCHAR BuildImpCallInstruction(_In_ PANALYZER pAnalyzer) {
    UCHAR code[] = {
        // call rip + 2 + 5
        0xE8, 0x02, 0x00, 0x00, 0x00,
        // jmp rip + 0x13 + 2
        0xEB, 0x13,
        // push rax
        0x50,
        // mov rax, 0x8877665544332211
        0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        // mov rax, qword ptr ds:[rax]
        0x48, 0x8B, 0x00,
        // xchg qword ptr ss:[rsp],rax
        0x48, 0x87, 0x04, 0x24,
        // ret
        0xC3
    };

    *(PUCHAR*)&code[10] = pAnalyzer->Repair.Address;

    RtlCopyMemory(pAnalyzer->Encoder.Address, code, sizeof(code));

    s_TotalSize += sizeof(code);

    return pAnalyzer->Encoder.Address;
}