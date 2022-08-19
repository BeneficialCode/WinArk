#include "pch.h"
#include <limits.h>
#include "disasm.h"
#include "detours.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Detours Disassembler (disasm.cpp of detours.lib)
//
//  Microsoft Research Detours Package, Version 4.0.1
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//


// thanks detours and pepsi :)

// These macros define common uses of nFixedSize, nFixedSize16, nModOffset, nRelOffset, nFlagBits, pfCopy.
#define ENTRY_DataIgnored           0, 0, 0, 0, 0,
#define ENTRY_CopyBytes1            { 1, 1, 0, 0, 0, &Disasm::CopyBytes }
#ifdef DETOURS_X64
#define ENTRY_CopyBytes1Address     { 9, 5, 0, 0, FlagBits::Address, &Disasm::CopyBytes }
#else
#define ENTRY_CopyBytes1Address     { 5, 3, 0, 0, FlagBits::Address, &Disasm::CopyBytes }
#endif
#define ENTRY_CopyBytes1Dynamic     { 1, 1, 0, 0, FlagBits::Dynamic, &Disasm::CopyBytes }
#define ENTRY_CopyBytes2            { 2, 2, 0, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes2Jump        { ENTRY_DataIgnored &Disasm::CopyBytesJump }
#define ENTRY_CopyBytes2CantJump    { 2, 2, 0, 1, FlagBits::NoEnlarge, &Disasm::CopyBytes }
#define ENTRY_CopyBytes2Dynamic     { 2, 2, 0, 0, FlagBits::Dynamic, &Disasm::CopyBytes }
#define ENTRY_CopyBytes3            { 3, 3, 0, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes3Dynamic     { 3, 3, 0, 0, FlagBits::Dynamic, &Disasm::CopyBytes }
#define ENTRY_CopyBytes3Or5         { 5, 3, 0, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes3Or5Dynamic  { 5, 3, 0, 0, FlagBits::Dynamic, &Disasm::CopyBytes }// x86 only
#ifdef DETOURS_X64
#define ENTRY_CopyBytes3Or5Rax      { 5, 3, 0, 0, FlagBits::Rax, &Disasm::CopyBytes }
#define ENTRY_CopyBytes3Or5Target   { 5, 5, 0, 1, 0, &Disasm::CopyBytes }
#else
#define ENTRY_CopyBytes3Or5Rax      { 5, 3, 0, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes3Or5Target   { 5, 3, 0, 1, 0, &Disasm::CopyBytes }
#endif
#define ENTRY_CopyBytes4            { 4, 4, 0, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes5            { 5, 5, 0, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes5Or7Dynamic  { 7, 5, 0, 0, FlagBits::Dynamic, &Disasm::CopyBytes }
#define ENTRY_CopyBytes7            { 7, 7, 0, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes2Mod         { 2, 2, 1, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes2ModDynamic  { 2, 2, 1, 0, FlagBits::Dynamic, &Disasm::CopyBytes }
#define ENTRY_CopyBytes2Mod1        { 3, 3, 1, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes2ModOperand  { 6, 4, 1, 0, 0, &Disasm::CopyBytes }
#define ENTRY_CopyBytes3Mod         { 3, 3, 2, 0, 0, &Disasm::CopyBytes } // SSE3 0F 38 opcode modrm
#define ENTRY_CopyBytes3Mod1        { 4, 4, 2, 0, 0, &Disasm::CopyBytes } // SSE3 0F 3A opcode modrm .. imm8
#define ENTRY_CopyBytesPrefix       { ENTRY_DataIgnored &Disasm::CopyBytesPrefix }
#define ENTRY_CopyBytesSegment      { ENTRY_DataIgnored &Disasm::CopyBytesSegment }
#define ENTRY_CopyBytesRax          { ENTRY_DataIgnored &Disasm::CopyBytesRax }
#define ENTRY_CopyF2                { ENTRY_DataIgnored &Disasm::CopyF2 }
#define ENTRY_CopyF3                { ENTRY_DataIgnored &Disasm::CopyF3 } // 32bit x86 only
#define ENTRY_Copy0F                { ENTRY_DataIgnored &Disasm::Copy0F }
#define ENTRY_Copy0F78              { ENTRY_DataIgnored &Disasm::Copy0F78 }
#define ENTRY_Copy0F00              { ENTRY_DataIgnored &Disasm::Copy0F00 } // 32bit x86 only
#define ENTRY_Copy0FB8              { ENTRY_DataIgnored &Disasm::Copy0FB8 } // 32bit x86 only
#define ENTRY_Copy66                { ENTRY_DataIgnored &Disasm::Copy66 }
#define ENTRY_Copy67                { ENTRY_DataIgnored &Disasm::Copy67 }
#define ENTRY_CopyF6                { ENTRY_DataIgnored &Disasm::CopyF6 }
#define ENTRY_CopyF7                { ENTRY_DataIgnored &Disasm::CopyF7 }
#define ENTRY_CopyFF                { ENTRY_DataIgnored &Disasm::CopyFF }
#define ENTRY_CopyVex2              { ENTRY_DataIgnored &Disasm::CopyVex2 }
#define ENTRY_CopyVex3              { ENTRY_DataIgnored &Disasm::CopyVex3 }
#define ENTRY_CopyEvex              { ENTRY_DataIgnored &Disasm::CopyEvex } // 62, 3 byte payload, then normal with implied prefixes like vex
#define ENTRY_CopyXop               { ENTRY_DataIgnored &Disasm::CopyXop }   // 0x8F ... POP /0 or AMD XOP
#define ENTRY_CopyBytesXop          { 5, 5, 4, 0, 0, &Disasm::CopyBytes } // 0x8F xop1 xop2 opcode modrm
#define ENTRY_CopyBytesXop1         { 6, 6, 4, 0, 0, &Disasm::CopyBytes } // 0x8F xop1 xop2 opcode modrm ... imm8
#define ENTRY_CopyBytesXop4         { 9, 9, 4, 0, 0, &Disasm::CopyBytes } // 0x8F xop1 xop2 opcode modrm ... imm32
#define ENTRY_Invalid               { ENTRY_DataIgnored &Disasm::Invalid }

const Disasm::CopyEntry Disasm::s_CopyTable[] = {
    /* 00 */ ENTRY_CopyBytes2Mod,                      // ADD /r
    /* 01 */ ENTRY_CopyBytes2Mod,                      // ADD /r
    /* 02 */ ENTRY_CopyBytes2Mod,                      // ADD /r
    /* 03 */ ENTRY_CopyBytes2Mod,                      // ADD /r
    /* 04 */ ENTRY_CopyBytes2,                         // ADD ib
    /* 05 */ ENTRY_CopyBytes3Or5,                      // ADD iw
#ifdef DETOURS_X64
    /* 06 */ ENTRY_Invalid,                            // Invalid
    /* 07 */ ENTRY_Invalid,                            // Invalid
#else
    /* 06 */ ENTRY_CopyBytes1,                         // PUSH
    /* 07 */ ENTRY_CopyBytes1,                         // POP
#endif
    /* 08 */ ENTRY_CopyBytes2Mod,                      // OR /r
    /* 09 */ ENTRY_CopyBytes2Mod,                      // OR /r
    /* 0A */ ENTRY_CopyBytes2Mod,                      // OR /r
    /* 0B */ ENTRY_CopyBytes2Mod,                      // OR /r
    /* 0C */ ENTRY_CopyBytes2,                         // OR ib
    /* 0D */ ENTRY_CopyBytes3Or5,                      // OR iw
#ifdef DETOURS_X64
    /* 0E */ ENTRY_Invalid,                            // Invalid
#else
    /* 0E */ ENTRY_CopyBytes1,                         // PUSH
#endif
    /* 0F */ ENTRY_Copy0F,                             // Extension Ops
    /* 10 */ ENTRY_CopyBytes2Mod,                      // ADC /r
    /* 11 */ ENTRY_CopyBytes2Mod,                      // ADC /r
    /* 12 */ ENTRY_CopyBytes2Mod,                      // ADC /r
    /* 13 */ ENTRY_CopyBytes2Mod,                      // ADC /r
    /* 14 */ ENTRY_CopyBytes2,                         // ADC ib
    /* 15 */ ENTRY_CopyBytes3Or5,                      // ADC id
#ifdef DETOURS_X64
    /* 16 */ ENTRY_Invalid,                            // Invalid
    /* 17 */ ENTRY_Invalid,                            // Invalid
#else
    /* 16 */ ENTRY_CopyBytes1,                         // PUSH
    /* 17 */ ENTRY_CopyBytes1,                         // POP
#endif
    /* 18 */ ENTRY_CopyBytes2Mod,                      // SBB /r
    /* 19 */ ENTRY_CopyBytes2Mod,                      // SBB /r
    /* 1A */ ENTRY_CopyBytes2Mod,                      // SBB /r
    /* 1B */ ENTRY_CopyBytes2Mod,                      // SBB /r
    /* 1C */ ENTRY_CopyBytes2,                         // SBB ib
    /* 1D */ ENTRY_CopyBytes3Or5,                      // SBB id
#ifdef DETOURS_X64
    /* 1E */ ENTRY_Invalid,                            // Invalid
    /* 1F */ ENTRY_Invalid,                            // Invalid
#else
    /* 1E */ ENTRY_CopyBytes1,                         // PUSH
    /* 1F */ ENTRY_CopyBytes1,                         // POP
#endif
    /* 20 */ ENTRY_CopyBytes2Mod,                      // AND /r
    /* 21 */ ENTRY_CopyBytes2Mod,                      // AND /r
    /* 22 */ ENTRY_CopyBytes2Mod,                      // AND /r
    /* 23 */ ENTRY_CopyBytes2Mod,                      // AND /r
    /* 24 */ ENTRY_CopyBytes2,                         // AND ib
    /* 25 */ ENTRY_CopyBytes3Or5,                      // AND id
    /* 26 */ ENTRY_CopyBytesSegment,                   // ES prefix
#ifdef DETOURS_X64
    /* 27 */ ENTRY_Invalid,                            // Invalid
#else
    /* 27 */ ENTRY_CopyBytes1,                         // DAA
#endif
    /* 28 */ ENTRY_CopyBytes2Mod,                      // SUB /r
    /* 29 */ ENTRY_CopyBytes2Mod,                      // SUB /r
    /* 2A */ ENTRY_CopyBytes2Mod,                      // SUB /r
    /* 2B */ ENTRY_CopyBytes2Mod,                      // SUB /r
    /* 2C */ ENTRY_CopyBytes2,                         // SUB ib
    /* 2D */ ENTRY_CopyBytes3Or5,                      // SUB id
    /* 2E */ ENTRY_CopyBytesSegment,                   // CS prefix
#ifdef DETOURS_X64
    /* 2F */ ENTRY_Invalid,                            // Invalid
#else
    /* 2F */ ENTRY_CopyBytes1,                         // DAS
#endif
    /* 30 */ ENTRY_CopyBytes2Mod,                      // XOR /r
    /* 31 */ ENTRY_CopyBytes2Mod,                      // XOR /r
    /* 32 */ ENTRY_CopyBytes2Mod,                      // XOR /r
    /* 33 */ ENTRY_CopyBytes2Mod,                      // XOR /r
    /* 34 */ ENTRY_CopyBytes2,                         // XOR ib
    /* 35 */ ENTRY_CopyBytes3Or5,                      // XOR id
    /* 36 */ ENTRY_CopyBytesSegment,                   // SS prefix
#ifdef DETOURS_X64
    /* 37 */ ENTRY_Invalid,                            // Invalid
#else
    /* 37 */ ENTRY_CopyBytes1,                         // AAA
#endif
    /* 38 */ ENTRY_CopyBytes2Mod,                      // CMP /r
    /* 39 */ ENTRY_CopyBytes2Mod,                      // CMP /r
    /* 3A */ ENTRY_CopyBytes2Mod,                      // CMP /r
    /* 3B */ ENTRY_CopyBytes2Mod,                      // CMP /r
    /* 3C */ ENTRY_CopyBytes2,                         // CMP ib
    /* 3D */ ENTRY_CopyBytes3Or5,                      // CMP id
    /* 3E */ ENTRY_CopyBytesSegment,                   // DS prefix
#ifdef DETOURS_X64
    /* 3F */ ENTRY_Invalid,                            // Invalid
#else
    /* 3F */ ENTRY_CopyBytes1,                         // AAS
#endif
#ifdef DETOURS_X64 // For Rax Prefix
    /* 40 */ ENTRY_CopyBytesRax,                       // Rax
    /* 41 */ ENTRY_CopyBytesRax,                       // Rax
    /* 42 */ ENTRY_CopyBytesRax,                       // Rax
    /* 43 */ ENTRY_CopyBytesRax,                       // Rax
    /* 44 */ ENTRY_CopyBytesRax,                       // Rax
    /* 45 */ ENTRY_CopyBytesRax,                       // Rax
    /* 46 */ ENTRY_CopyBytesRax,                       // Rax
    /* 47 */ ENTRY_CopyBytesRax,                       // Rax
    /* 48 */ ENTRY_CopyBytesRax,                       // Rax
    /* 49 */ ENTRY_CopyBytesRax,                       // Rax
    /* 4A */ ENTRY_CopyBytesRax,                       // Rax
    /* 4B */ ENTRY_CopyBytesRax,                       // Rax
    /* 4C */ ENTRY_CopyBytesRax,                       // Rax
    /* 4D */ ENTRY_CopyBytesRax,                       // Rax
    /* 4E */ ENTRY_CopyBytesRax,                       // Rax
    /* 4F */ ENTRY_CopyBytesRax,                       // Rax
#else
    /* 40 */ ENTRY_CopyBytes1,                         // INC
    /* 41 */ ENTRY_CopyBytes1,                         // INC
    /* 42 */ ENTRY_CopyBytes1,                         // INC
    /* 43 */ ENTRY_CopyBytes1,                         // INC
    /* 44 */ ENTRY_CopyBytes1,                         // INC
    /* 45 */ ENTRY_CopyBytes1,                         // INC
    /* 46 */ ENTRY_CopyBytes1,                         // INC
    /* 47 */ ENTRY_CopyBytes1,                         // INC
    /* 48 */ ENTRY_CopyBytes1,                         // DEC
    /* 49 */ ENTRY_CopyBytes1,                         // DEC
    /* 4A */ ENTRY_CopyBytes1,                         // DEC
    /* 4B */ ENTRY_CopyBytes1,                         // DEC
    /* 4C */ ENTRY_CopyBytes1,                         // DEC
    /* 4D */ ENTRY_CopyBytes1,                         // DEC
    /* 4E */ ENTRY_CopyBytes1,                         // DEC
    /* 4F */ ENTRY_CopyBytes1,                         // DEC
#endif
    /* 50 */ ENTRY_CopyBytes1,                         // PUSH
    /* 51 */ ENTRY_CopyBytes1,                         // PUSH
    /* 52 */ ENTRY_CopyBytes1,                         // PUSH
    /* 53 */ ENTRY_CopyBytes1,                         // PUSH
    /* 54 */ ENTRY_CopyBytes1,                         // PUSH
    /* 55 */ ENTRY_CopyBytes1,                         // PUSH
    /* 56 */ ENTRY_CopyBytes1,                         // PUSH
    /* 57 */ ENTRY_CopyBytes1,                         // PUSH
    /* 58 */ ENTRY_CopyBytes1,                         // POP
    /* 59 */ ENTRY_CopyBytes1,                         // POP
    /* 5A */ ENTRY_CopyBytes1,                         // POP
    /* 5B */ ENTRY_CopyBytes1,                         // POP
    /* 5C */ ENTRY_CopyBytes1,                         // POP
    /* 5D */ ENTRY_CopyBytes1,                         // POP
    /* 5E */ ENTRY_CopyBytes1,                         // POP
    /* 5F */ ENTRY_CopyBytes1,                         // POP
#ifdef DETOURS_X64
    /* 60 */ ENTRY_Invalid,                            // Invalid
    /* 61 */ ENTRY_Invalid,                            // Invalid
    /* 62 */ ENTRY_CopyEvex,                           // EVEX / AVX512
#else
    /* 60 */ ENTRY_CopyBytes1,                         // PUSHAD
    /* 61 */ ENTRY_CopyBytes1,                         // POPAD
    /* 62 */ ENTRY_CopyEvex,                           // BOUND /r and EVEX / AVX512
#endif
    /* 63 */ ENTRY_CopyBytes2Mod,                      // 32bit ARPL /r, 64bit MOVSXD
    /* 64 */ ENTRY_CopyBytesSegment,                   // FS prefix
    /* 65 */ ENTRY_CopyBytesSegment,                   // GS prefix
    /* 66 */ ENTRY_Copy66,                             // Operand Prefix
    /* 67 */ ENTRY_Copy67,                             // Address Prefix
    /* 68 */ ENTRY_CopyBytes3Or5,                      // PUSH
    /* 69 */ ENTRY_CopyBytes2ModOperand,               // IMUL /r iz
    /* 6A */ ENTRY_CopyBytes2,                         // PUSH
    /* 6B */ ENTRY_CopyBytes2Mod1,                     // IMUL /r ib
    /* 6C */ ENTRY_CopyBytes1,                         // INS
    /* 6D */ ENTRY_CopyBytes1,                         // INS
    /* 6E */ ENTRY_CopyBytes1,                         // OUTS/OUTSB
    /* 6F */ ENTRY_CopyBytes1,                         // OUTS/OUTSW
    /* 70 */ ENTRY_CopyBytes2Jump,                     // JO           // 0f80
    /* 71 */ ENTRY_CopyBytes2Jump,                     // JNO          // 0f81
    /* 72 */ ENTRY_CopyBytes2Jump,                     // JB/JC/JNAE   // 0f82
    /* 73 */ ENTRY_CopyBytes2Jump,                     // JAE/JNB/JNC  // 0f83
    /* 74 */ ENTRY_CopyBytes2Jump,                     // JE/JZ        // 0f84
    /* 75 */ ENTRY_CopyBytes2Jump,                     // JNE/JNZ      // 0f85
    /* 76 */ ENTRY_CopyBytes2Jump,                     // JBE/JNA      // 0f86
    /* 77 */ ENTRY_CopyBytes2Jump,                     // JA/JNBE      // 0f87
    /* 78 */ ENTRY_CopyBytes2Jump,                     // JS           // 0f88
    /* 79 */ ENTRY_CopyBytes2Jump,                     // JNS          // 0f89
    /* 7A */ ENTRY_CopyBytes2Jump,                     // JP/JPE       // 0f8a
    /* 7B */ ENTRY_CopyBytes2Jump,                     // JNP/JPO      // 0f8b
    /* 7C */ ENTRY_CopyBytes2Jump,                     // JL/JNGE      // 0f8c
    /* 7D */ ENTRY_CopyBytes2Jump,                     // JGE/JNL      // 0f8d
    /* 7E */ ENTRY_CopyBytes2Jump,                     // JLE/JNG      // 0f8e
    /* 7F */ ENTRY_CopyBytes2Jump,                     // JG/JNLE      // 0f8f
    /* 80 */ ENTRY_CopyBytes2Mod1,                     // ADD/0 OR/1 ADC/2 SBB/3 AND/4 SUB/5 XOR/6 CMP/7 byte reg, immediate byte
    /* 81 */ ENTRY_CopyBytes2ModOperand,               // ADD/0 OR/1 ADC/2 SBB/3 AND/4 SUB/5 XOR/6 CMP/7 byte reg, immediate word or dword
#ifdef DETOURS_X64
    /* 82 */ ENTRY_Invalid,                            // Invalid
#else
    /* 82 */ ENTRY_CopyBytes2Mod1,                     // MOV al,x
#endif
    /* 83 */ ENTRY_CopyBytes2Mod1,                     // ADD/0 OR/1 ADC/2 SBB/3 AND/4 SUB/5 XOR/6 CMP/7 reg, immediate byte
    /* 84 */ ENTRY_CopyBytes2Mod,                      // TEST /r
    /* 85 */ ENTRY_CopyBytes2Mod,                      // TEST /r
    /* 86 */ ENTRY_CopyBytes2Mod,                      // XCHG /r @todo
    /* 87 */ ENTRY_CopyBytes2Mod,                      // XCHG /r @todo
    /* 88 */ ENTRY_CopyBytes2Mod,                      // MOV /r
    /* 89 */ ENTRY_CopyBytes2Mod,                      // MOV /r
    /* 8A */ ENTRY_CopyBytes2Mod,                      // MOV /r
    /* 8B */ ENTRY_CopyBytes2Mod,                      // MOV /r
    /* 8C */ ENTRY_CopyBytes2Mod,                      // MOV /r
    /* 8D */ ENTRY_CopyBytes2Mod,                      // LEA /r
    /* 8E */ ENTRY_CopyBytes2Mod,                      // MOV /r
    /* 8F */ ENTRY_CopyXop,                            // POP /0 or AMD XOP
    /* 90 */ ENTRY_CopyBytes1,                         // NOP
    /* 91 */ ENTRY_CopyBytes1,                         // XCHG
    /* 92 */ ENTRY_CopyBytes1,                         // XCHG
    /* 93 */ ENTRY_CopyBytes1,                         // XCHG
    /* 94 */ ENTRY_CopyBytes1,                         // XCHG
    /* 95 */ ENTRY_CopyBytes1,                         // XCHG
    /* 96 */ ENTRY_CopyBytes1,                         // XCHG
    /* 97 */ ENTRY_CopyBytes1,                         // XCHG
    /* 98 */ ENTRY_CopyBytes1,                         // CWDE
    /* 99 */ ENTRY_CopyBytes1,                         // CDQ
#ifdef DETOURS_X64
    /* 9A */ ENTRY_Invalid,                            // Invalid
#else
    /* 9A */ ENTRY_CopyBytes5Or7Dynamic,               // CALL cp
#endif
    /* 9B */ ENTRY_CopyBytes1,                         // WAIT/FWAIT
    /* 9C */ ENTRY_CopyBytes1,                         // PUSHFD
    /* 9D */ ENTRY_CopyBytes1,                         // POPFD
    /* 9E */ ENTRY_CopyBytes1,                         // SAHF
    /* 9F */ ENTRY_CopyBytes1,                         // LAHF
    /* A0 */ ENTRY_CopyBytes1Address,                  // MOV
    /* A1 */ ENTRY_CopyBytes1Address,                  // MOV
    /* A2 */ ENTRY_CopyBytes1Address,                  // MOV
    /* A3 */ ENTRY_CopyBytes1Address,                  // MOV
    /* A4 */ ENTRY_CopyBytes1,                         // MOVS
    /* A5 */ ENTRY_CopyBytes1,                         // MOVS/MOVSD
    /* A6 */ ENTRY_CopyBytes1,                         // CMPS/CMPSB
    /* A7 */ ENTRY_CopyBytes1,                         // CMPS/CMPSW
    /* A8 */ ENTRY_CopyBytes2,                         // TEST
    /* A9 */ ENTRY_CopyBytes3Or5,                      // TEST
    /* AA */ ENTRY_CopyBytes1,                         // STOS/STOSB
    /* AB */ ENTRY_CopyBytes1,                         // STOS/STOSW
    /* AC */ ENTRY_CopyBytes1,                         // LODS/LODSB
    /* AD */ ENTRY_CopyBytes1,                         // LODS/LODSW
    /* AE */ ENTRY_CopyBytes1,                         // SCAS/SCASB
    /* AF */ ENTRY_CopyBytes1,                         // SCAS/SCASD
    /* B0 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B1 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B2 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B3 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B4 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B5 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B6 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B7 */ ENTRY_CopyBytes2,                         // MOV B0+rb
    /* B8 */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* B9 */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* BA */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* BB */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* BC */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* BD */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* BE */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* BF */ ENTRY_CopyBytes3Or5Rax,                   // MOV B8+rb
    /* C0 */ ENTRY_CopyBytes2Mod1,                     // RCL/2 ib, etc.
    /* C1 */ ENTRY_CopyBytes2Mod1,                     // RCL/2 ib, etc.
    /* C2 */ ENTRY_CopyBytes3,                         // RET
    /* C3 */ ENTRY_CopyBytes1,                         // RET
    /* C4 */ ENTRY_CopyVex3,                           // LES, VEX 3-byte opcodes.
    /* C5 */ ENTRY_CopyVex2,                           // LDS, VEX 2-byte opcodes.
    /* C6 */ ENTRY_CopyBytes2Mod1,                     // MOV
    /* C7 */ ENTRY_CopyBytes2ModOperand,               // MOV/0 XBEGIN/7
    /* C8 */ ENTRY_CopyBytes4,                         // ENTER
    /* C9 */ ENTRY_CopyBytes1,                         // LEAVE
    /* CA */ ENTRY_CopyBytes3Dynamic,                  // RET
    /* CB */ ENTRY_CopyBytes1Dynamic,                  // RET
    /* CC */ ENTRY_CopyBytes1Dynamic,                  // INT 3
    /* CD */ ENTRY_CopyBytes2Dynamic,                  // INT ib
#ifdef DETOURS_X64
    /* CE */ ENTRY_Invalid,                            // Invalid
#else
    /* CE */ ENTRY_CopyBytes1Dynamic,                  // INTO
#endif
    /* CF */ ENTRY_CopyBytes1Dynamic,                  // IRET
    /* D0 */ ENTRY_CopyBytes2Mod,                      // RCL/2, etc.
    /* D1 */ ENTRY_CopyBytes2Mod,                      // RCL/2, etc.
    /* D2 */ ENTRY_CopyBytes2Mod,                      // RCL/2, etc.
    /* D3 */ ENTRY_CopyBytes2Mod,                      // RCL/2, etc.
#ifdef DETOURS_X64
    /* D4 */ ENTRY_Invalid,                            // Invalid
    /* D5 */ ENTRY_Invalid,                            // Invalid
#else
    /* D4 */ ENTRY_CopyBytes2,                         // AAM
    /* D5 */ ENTRY_CopyBytes2,                         // AAD
#endif
    /* D6 */ ENTRY_Invalid,                            // Invalid
    /* D7 */ ENTRY_CopyBytes1,                         // XLAT/XLATB
    /* D8 */ ENTRY_CopyBytes2Mod,                      // FADD, etc.
    /* D9 */ ENTRY_CopyBytes2Mod,                      // F2XM1, etc.
    /* DA */ ENTRY_CopyBytes2Mod,                      // FLADD, etc.
    /* DB */ ENTRY_CopyBytes2Mod,                      // FCLEX, etc.
    /* DC */ ENTRY_CopyBytes2Mod,                      // FADD/0, etc.
    /* DD */ ENTRY_CopyBytes2Mod,                      // FFREE, etc.
    /* DE */ ENTRY_CopyBytes2Mod,                      // FADDP, etc.
    /* DF */ ENTRY_CopyBytes2Mod,                      // FBLD/4, etc.
    /* E0 */ ENTRY_CopyBytes2CantJump,                 // LOOPNE cb
    /* E1 */ ENTRY_CopyBytes2CantJump,                 // LOOPE cb
    /* E2 */ ENTRY_CopyBytes2CantJump,                 // LOOP cb
    /* E3 */ ENTRY_CopyBytes2CantJump,                 // JCXZ/JECXZ
    /* E4 */ ENTRY_CopyBytes2,                         // IN ib
    /* E5 */ ENTRY_CopyBytes2,                         // IN id
    /* E6 */ ENTRY_CopyBytes2,                         // OUT ib
    /* E7 */ ENTRY_CopyBytes2,                         // OUT ib
    /* E8 */ ENTRY_CopyBytes3Or5Target,                // CALL cd
    /* E9 */ ENTRY_CopyBytes3Or5Target,                // JMP cd
#ifdef DETOURS_X64
    /* EA */ ENTRY_Invalid,                            // Invalid
#else
    /* EA */ ENTRY_CopyBytes5Or7Dynamic,               // JMP cp
#endif
    /* EB */ ENTRY_CopyBytes2Jump,                     // JMP cb
    /* EC */ ENTRY_CopyBytes1,                         // IN ib
    /* ED */ ENTRY_CopyBytes1,                         // IN id
    /* EE */ ENTRY_CopyBytes1,                         // OUT
    /* EF */ ENTRY_CopyBytes1,                         // OUT
    /* F0 */ ENTRY_CopyBytesPrefix,                    // LOCK prefix
    /* F1 */ ENTRY_CopyBytes1Dynamic,                  // INT1 / ICEBP somewhat documented by AMD, not by Intel
    /* F2 */ ENTRY_CopyF2,                             // REPNE prefix
//#ifdef DETOURS_X86
/* F3 */ ENTRY_CopyF3,                             // REPE prefix
//#else
// This does presently suffice for AMD64 but it requires tracing
// through a bunch of code to verify and seems not worth maintaining.
//  /* F3 */ ENTRY_CopyBytesPrefix,                    // REPE prefix
//#endif
/* F4 */ ENTRY_CopyBytes1,                         // HLT
/* F5 */ ENTRY_CopyBytes1,                         // CMC
/* F6 */ ENTRY_CopyF6,                             // TEST/0, DIV/6
/* F7 */ ENTRY_CopyF7,                             // TEST/0, DIV/6
/* F8 */ ENTRY_CopyBytes1,                         // CLC
/* F9 */ ENTRY_CopyBytes1,                         // STC
/* FA */ ENTRY_CopyBytes1,                         // CLI
/* FB */ ENTRY_CopyBytes1,                         // STI
/* FC */ ENTRY_CopyBytes1,                         // CLD
/* FD */ ENTRY_CopyBytes1,                         // STD
/* FE */ ENTRY_CopyBytes2Mod,                      // DEC/1,INC/0
/* FF */ ENTRY_CopyFF,                             // CALL/2
};

///////////////////////////////////////////////////////// Disassembler Tables.
//
const UCHAR Disasm::s_ModRm[256] = {
    0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Rip) | 4,0,0, 0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Sib) | 4,0,0, // 0x
    0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Rip) | 4,0,0, 0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Sib) | 4,0,0, // 1x
    0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Rip) | 4,0,0, 0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Sib) | 4,0,0, // 2x
    0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Rip) | 4,0,0, 0,0,0,0, static_cast<UCHAR>(ModRm::Sib) | 1,static_cast<UCHAR>(ModRm::Sib) | 4,0,0, // 3x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 4x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 5x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 6x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 7x
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // 8x
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // 9x
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // Ax
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // Bx
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,                 // Cx
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,                 // Dx
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,                 // Ex
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0                  // Fx
};

const Disasm::CopyEntry	Disasm::s_CopyTable0F[] = {
#ifdef DETOURS_X86
    /* 00 */ ENTRY_Copy0F00,                           // sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6/dynamic invalid/7
#else
    /* 00 */ ENTRY_CopyBytes2Mod,                      // sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6/dynamic invalid/7
#endif
    /* 01 */ ENTRY_CopyBytes2Mod,                      // INVLPG/7, etc.
    /* 02 */ ENTRY_CopyBytes2Mod,                      // LAR/r
    /* 03 */ ENTRY_CopyBytes2Mod,                      // LSL/r
    /* 04 */ ENTRY_Invalid,                            // _04
    /* 05 */ ENTRY_CopyBytes1,                         // SYSCALL
    /* 06 */ ENTRY_CopyBytes1,                         // CLTS
    /* 07 */ ENTRY_CopyBytes1,                         // SYSRET
    /* 08 */ ENTRY_CopyBytes1,                         // INVD
    /* 09 */ ENTRY_CopyBytes1,                         // WBINVD
    /* 0A */ ENTRY_Invalid,                            // _0A
    /* 0B */ ENTRY_CopyBytes1,                         // UD2
    /* 0C */ ENTRY_Invalid,                            // _0C
    /* 0D */ ENTRY_CopyBytes2Mod,                      // PREFETCH
    /* 0E */ ENTRY_CopyBytes1,                         // FEMMS (3DNow -- not in Intel documentation)
    /* 0F */ ENTRY_CopyBytes2Mod1,                     // 3DNow Opcodes
    /* 10 */ ENTRY_CopyBytes2Mod,                      // MOVSS MOVUPD MOVSD
    /* 11 */ ENTRY_CopyBytes2Mod,                      // MOVSS MOVUPD MOVSD
    /* 12 */ ENTRY_CopyBytes2Mod,                      // MOVLPD
    /* 13 */ ENTRY_CopyBytes2Mod,                      // MOVLPD
    /* 14 */ ENTRY_CopyBytes2Mod,                      // UNPCKLPD
    /* 15 */ ENTRY_CopyBytes2Mod,                      // UNPCKHPD
    /* 16 */ ENTRY_CopyBytes2Mod,                      // MOVHPD
    /* 17 */ ENTRY_CopyBytes2Mod,                      // MOVHPD
    /* 18 */ ENTRY_CopyBytes2Mod,                      // PREFETCHINTA...
    /* 19 */ ENTRY_CopyBytes2Mod,                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    /* 1A */ ENTRY_CopyBytes2Mod,                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    /* 1B */ ENTRY_CopyBytes2Mod,                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    /* 1C */ ENTRY_CopyBytes2Mod,                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    /* 1D */ ENTRY_CopyBytes2Mod,                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    /* 1E */ ENTRY_CopyBytes2Mod,                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    /* 1F */ ENTRY_CopyBytes2Mod,                      // NOP/r multi byte nop
    /* 20 */ ENTRY_CopyBytes2Mod,                      // MOV/r
    /* 21 */ ENTRY_CopyBytes2Mod,                      // MOV/r
    /* 22 */ ENTRY_CopyBytes2Mod,                      // MOV/r
    /* 23 */ ENTRY_CopyBytes2Mod,                      // MOV/r
#ifdef DETOURS_X64
    /* 24 */ ENTRY_Invalid,                            // _24
#else
    /* 24 */ ENTRY_CopyBytes2Mod,                      // MOV/r,TR TR is test register on 80386 and 80486, removed in Pentium
#endif
    /* 25 */ ENTRY_Invalid,                            // _25
#ifdef DETOURS_X64
    /* 26 */ ENTRY_Invalid,                            // _26
#else
    /* 26 */ ENTRY_CopyBytes2Mod,                      // MOV TR/r TR is test register on 80386 and 80486, removed in Pentium
#endif
    /* 27 */ ENTRY_Invalid,                            // _27
    /* 28 */ ENTRY_CopyBytes2Mod,                      // MOVAPS MOVAPD
    /* 29 */ ENTRY_CopyBytes2Mod,                      // MOVAPS MOVAPD
    /* 2A */ ENTRY_CopyBytes2Mod,                      // CVPI2PS &
    /* 2B */ ENTRY_CopyBytes2Mod,                      // MOVNTPS MOVNTPD
    /* 2C */ ENTRY_CopyBytes2Mod,                      // CVTTPS2PI &
    /* 2D */ ENTRY_CopyBytes2Mod,                      // CVTPS2PI &
    /* 2E */ ENTRY_CopyBytes2Mod,                      // UCOMISS UCOMISD
    /* 2F */ ENTRY_CopyBytes2Mod,                      // COMISS COMISD
    /* 30 */ ENTRY_CopyBytes1,                         // WRMSR
    /* 31 */ ENTRY_CopyBytes1,                         // RDTSC
    /* 32 */ ENTRY_CopyBytes1,                         // RDMSR
    /* 33 */ ENTRY_CopyBytes1,                         // RDPMC
    /* 34 */ ENTRY_CopyBytes1,                         // SYSENTER
    /* 35 */ ENTRY_CopyBytes1,                         // SYSEXIT
    /* 36 */ ENTRY_Invalid,                            // _36
    /* 37 */ ENTRY_CopyBytes1,                         // GETSEC
    /* 38 */ ENTRY_CopyBytes3Mod,                      // SSE3 Opcodes
    /* 39 */ ENTRY_Invalid,                            // _39
    /* 3A */ ENTRY_CopyBytes3Mod1,                      // SSE3 Opcodes
    /* 3B */ ENTRY_Invalid,                            // _3B
    /* 3C */ ENTRY_Invalid,                            // _3C
    /* 3D */ ENTRY_Invalid,                            // _3D
    /* 3E */ ENTRY_Invalid,                            // _3E
    /* 3F */ ENTRY_Invalid,                            // _3F
    /* 40 */ ENTRY_CopyBytes2Mod,                      // CMOVO (0F 40)
    /* 41 */ ENTRY_CopyBytes2Mod,                      // CMOVNO (0F 41)
    /* 42 */ ENTRY_CopyBytes2Mod,                      // CMOVB & CMOVNE (0F 42)
    /* 43 */ ENTRY_CopyBytes2Mod,                      // CMOVAE & CMOVNB (0F 43)
    /* 44 */ ENTRY_CopyBytes2Mod,                      // CMOVE & CMOVZ (0F 44)
    /* 45 */ ENTRY_CopyBytes2Mod,                      // CMOVNE & CMOVNZ (0F 45)
    /* 46 */ ENTRY_CopyBytes2Mod,                      // CMOVBE & CMOVNA (0F 46)
    /* 47 */ ENTRY_CopyBytes2Mod,                      // CMOVA & CMOVNBE (0F 47)
    /* 48 */ ENTRY_CopyBytes2Mod,                      // CMOVS (0F 48)
    /* 49 */ ENTRY_CopyBytes2Mod,                      // CMOVNS (0F 49)
    /* 4A */ ENTRY_CopyBytes2Mod,                      // CMOVP & CMOVPE (0F 4A)
    /* 4B */ ENTRY_CopyBytes2Mod,                      // CMOVNP & CMOVPO (0F 4B)
    /* 4C */ ENTRY_CopyBytes2Mod,                      // CMOVL & CMOVNGE (0F 4C)
    /* 4D */ ENTRY_CopyBytes2Mod,                      // CMOVGE & CMOVNL (0F 4D)
    /* 4E */ ENTRY_CopyBytes2Mod,                      // CMOVLE & CMOVNG (0F 4E)
    /* 4F */ ENTRY_CopyBytes2Mod,                      // CMOVG & CMOVNLE (0F 4F)
    /* 50 */ ENTRY_CopyBytes2Mod,                      // MOVMSKPD MOVMSKPD
    /* 51 */ ENTRY_CopyBytes2Mod,                      // SQRTPS &
    /* 52 */ ENTRY_CopyBytes2Mod,                      // RSQRTTS RSQRTPS
    /* 53 */ ENTRY_CopyBytes2Mod,                      // RCPPS RCPSS
    /* 54 */ ENTRY_CopyBytes2Mod,                      // ANDPS ANDPD
    /* 55 */ ENTRY_CopyBytes2Mod,                      // ANDNPS ANDNPD
    /* 56 */ ENTRY_CopyBytes2Mod,                      // ORPS ORPD
    /* 57 */ ENTRY_CopyBytes2Mod,                      // XORPS XORPD
    /* 58 */ ENTRY_CopyBytes2Mod,                      // ADDPS &
    /* 59 */ ENTRY_CopyBytes2Mod,                      // MULPS &
    /* 5A */ ENTRY_CopyBytes2Mod,                      // CVTPS2PD &
    /* 5B */ ENTRY_CopyBytes2Mod,                      // CVTDQ2PS &
    /* 5C */ ENTRY_CopyBytes2Mod,                      // SUBPS &
    /* 5D */ ENTRY_CopyBytes2Mod,                      // MINPS &
    /* 5E */ ENTRY_CopyBytes2Mod,                      // DIVPS &
    /* 5F */ ENTRY_CopyBytes2Mod,                      // MASPS &
    /* 60 */ ENTRY_CopyBytes2Mod,                      // PUNPCKLBW/r
    /* 61 */ ENTRY_CopyBytes2Mod,                      // PUNPCKLWD/r
    /* 62 */ ENTRY_CopyBytes2Mod,                      // PUNPCKLWD/r
    /* 63 */ ENTRY_CopyBytes2Mod,                      // PACKSSWB/r
    /* 64 */ ENTRY_CopyBytes2Mod,                      // PCMPGTB/r
    /* 65 */ ENTRY_CopyBytes2Mod,                      // PCMPGTW/r
    /* 66 */ ENTRY_CopyBytes2Mod,                      // PCMPGTD/r
    /* 67 */ ENTRY_CopyBytes2Mod,                      // PACKUSWB/r
    /* 68 */ ENTRY_CopyBytes2Mod,                      // PUNPCKHBW/r
    /* 69 */ ENTRY_CopyBytes2Mod,                      // PUNPCKHWD/r
    /* 6A */ ENTRY_CopyBytes2Mod,                      // PUNPCKHDQ/r
    /* 6B */ ENTRY_CopyBytes2Mod,                      // PACKSSDW/r
    /* 6C */ ENTRY_CopyBytes2Mod,                      // PUNPCKLQDQ
    /* 6D */ ENTRY_CopyBytes2Mod,                      // PUNPCKHQDQ
    /* 6E */ ENTRY_CopyBytes2Mod,                      // MOVD/r
    /* 6F */ ENTRY_CopyBytes2Mod,                      // MOV/r
    /* 70 */ ENTRY_CopyBytes2Mod1,                     // PSHUFW/r ib
    /* 71 */ ENTRY_CopyBytes2Mod1,                     // PSLLW/6 ib,PSRAW/4 ib,PSRLW/2 ib
    /* 72 */ ENTRY_CopyBytes2Mod1,                     // PSLLD/6 ib,PSRAD/4 ib,PSRLD/2 ib
    /* 73 */ ENTRY_CopyBytes2Mod1,                     // PSLLQ/6 ib,PSRLQ/2 ib
    /* 74 */ ENTRY_CopyBytes2Mod,                      // PCMPEQB/r
    /* 75 */ ENTRY_CopyBytes2Mod,                      // PCMPEQW/r
    /* 76 */ ENTRY_CopyBytes2Mod,                      // PCMPEQD/r
    /* 77 */ ENTRY_CopyBytes1,                         // EMMS
    // extrq/insertq require mode=3 and are followed by two immediate bytes
    /* 78 */ ENTRY_Copy0F78,                           // VMREAD/r, 66/EXTRQ/r/ib/ib, F2/INSERTQ/r/ib/ib
    // extrq/insertq require mod=3, therefore ENTRY_CopyBytes2, but it ends up the same
    /* 79 */ ENTRY_CopyBytes2Mod,                      // VMWRITE/r, 66/EXTRQ/r, F2/INSERTQ/r
    /* 7A */ ENTRY_Invalid,                            // _7A
    /* 7B */ ENTRY_Invalid,                            // _7B
    /* 7C */ ENTRY_CopyBytes2Mod,                      // HADDPS
    /* 7D */ ENTRY_CopyBytes2Mod,                      // HSUBPS
    /* 7E */ ENTRY_CopyBytes2Mod,                      // MOVD/r
    /* 7F */ ENTRY_CopyBytes2Mod,                      // MOV/r
    /* 80 */ ENTRY_CopyBytes3Or5Target,                // JO
    /* 81 */ ENTRY_CopyBytes3Or5Target,                // JNO
    /* 82 */ ENTRY_CopyBytes3Or5Target,                // JB,JC,JNAE
    /* 83 */ ENTRY_CopyBytes3Or5Target,                // JAE,JNB,JNC
    /* 84 */ ENTRY_CopyBytes3Or5Target,                // JE,JZ,JZ
    /* 85 */ ENTRY_CopyBytes3Or5Target,                // JNE,JNZ
    /* 86 */ ENTRY_CopyBytes3Or5Target,                // JBE,JNA
    /* 87 */ ENTRY_CopyBytes3Or5Target,                // JA,JNBE
    /* 88 */ ENTRY_CopyBytes3Or5Target,                // JS
    /* 89 */ ENTRY_CopyBytes3Or5Target,                // JNS
    /* 8A */ ENTRY_CopyBytes3Or5Target,                // JP,JPE
    /* 8B */ ENTRY_CopyBytes3Or5Target,                // JNP,JPO
    /* 8C */ ENTRY_CopyBytes3Or5Target,                // JL,NGE
    /* 8D */ ENTRY_CopyBytes3Or5Target,                // JGE,JNL
    /* 8E */ ENTRY_CopyBytes3Or5Target,                // JLE,JNG
    /* 8F */ ENTRY_CopyBytes3Or5Target,                // JG,JNLE
    /* 90 */ ENTRY_CopyBytes2Mod,                      // CMOVO (0F 40)
    /* 91 */ ENTRY_CopyBytes2Mod,                      // CMOVNO (0F 41)
    /* 92 */ ENTRY_CopyBytes2Mod,                      // CMOVB & CMOVC & CMOVNAE (0F 42)
    /* 93 */ ENTRY_CopyBytes2Mod,                      // CMOVAE & CMOVNB & CMOVNC (0F 43)
    /* 94 */ ENTRY_CopyBytes2Mod,                      // CMOVE & CMOVZ (0F 44)
    /* 95 */ ENTRY_CopyBytes2Mod,                      // CMOVNE & CMOVNZ (0F 45)
    /* 96 */ ENTRY_CopyBytes2Mod,                      // CMOVBE & CMOVNA (0F 46)
    /* 97 */ ENTRY_CopyBytes2Mod,                      // CMOVA & CMOVNBE (0F 47)
    /* 98 */ ENTRY_CopyBytes2Mod,                      // CMOVS (0F 48)
    /* 99 */ ENTRY_CopyBytes2Mod,                      // CMOVNS (0F 49)
    /* 9A */ ENTRY_CopyBytes2Mod,                      // CMOVP & CMOVPE (0F 4A)
    /* 9B */ ENTRY_CopyBytes2Mod,                      // CMOVNP & CMOVPO (0F 4B)
    /* 9C */ ENTRY_CopyBytes2Mod,                      // CMOVL & CMOVNGE (0F 4C)
    /* 9D */ ENTRY_CopyBytes2Mod,                      // CMOVGE & CMOVNL (0F 4D)
    /* 9E */ ENTRY_CopyBytes2Mod,                      // CMOVLE & CMOVNG (0F 4E)
    /* 9F */ ENTRY_CopyBytes2Mod,                      // CMOVG & CMOVNLE (0F 4F)
    /* A0 */ ENTRY_CopyBytes1,                         // PUSH
    /* A1 */ ENTRY_CopyBytes1,                         // POP
    /* A2 */ ENTRY_CopyBytes1,                         // CPUID
    /* A3 */ ENTRY_CopyBytes2Mod,                      // BT  (0F A3)
    /* A4 */ ENTRY_CopyBytes2Mod1,                     // SHLD
    /* A5 */ ENTRY_CopyBytes2Mod,                      // SHLD
    /* A6 */ ENTRY_CopyBytes2Mod,                      // XBTS
    /* A7 */ ENTRY_CopyBytes2Mod,                      // IBTS
    /* A8 */ ENTRY_CopyBytes1,                         // PUSH
    /* A9 */ ENTRY_CopyBytes1,                         // POP
    /* AA */ ENTRY_CopyBytes1,                         // RSM
    /* AB */ ENTRY_CopyBytes2Mod,                      // BTS (0F AB)
    /* AC */ ENTRY_CopyBytes2Mod1,                     // SHRD
    /* AD */ ENTRY_CopyBytes2Mod,                      // SHRD

    // 0F AE mod76=mem mod543=0 fxsave
    // 0F AE mod76=mem mod543=1 fxrstor
    // 0F AE mod76=mem mod543=2 ldmxcsr
    // 0F AE mod76=mem mod543=3 stmxcsr
    // 0F AE mod76=mem mod543=4 xsave
    // 0F AE mod76=mem mod543=5 xrstor
    // 0F AE mod76=mem mod543=6 saveopt
    // 0F AE mod76=mem mod543=7 clflush
    // 0F AE mod76=11b mod543=5 lfence
    // 0F AE mod76=11b mod543=6 mfence
    // 0F AE mod76=11b mod543=7 sfence
    // F3 0F AE mod76=11b mod543=0 rdfsbase
    // F3 0F AE mod76=11b mod543=1 rdgsbase
    // F3 0F AE mod76=11b mod543=2 wrfsbase
    // F3 0F AE mod76=11b mod543=3 wrgsbase
    /* AE */ ENTRY_CopyBytes2Mod,                      // fxsave fxrstor ldmxcsr stmxcsr xsave xrstor saveopt clflush lfence mfence sfence rdfsbase rdgsbase wrfsbase wrgsbase
    /* AF */ ENTRY_CopyBytes2Mod,                      // IMUL (0F AF)
    /* B0 */ ENTRY_CopyBytes2Mod,                      // CMPXCHG (0F B0)
    /* B1 */ ENTRY_CopyBytes2Mod,                      // CMPXCHG (0F B1)
    /* B2 */ ENTRY_CopyBytes2Mod,                      // LSS/r
    /* B3 */ ENTRY_CopyBytes2Mod,                      // BTR (0F B3)
    /* B4 */ ENTRY_CopyBytes2Mod,                      // LFS/r
    /* B5 */ ENTRY_CopyBytes2Mod,                      // LGS/r
    /* B6 */ ENTRY_CopyBytes2Mod,                      // MOVZX/r
    /* B7 */ ENTRY_CopyBytes2Mod,                      // MOVZX/r
#ifdef DETOURS_X86
    /* B8 */ ENTRY_Copy0FB8,                           // jmpe f3/popcnt
#else
    /* B8 */ ENTRY_CopyBytes2Mod,                      // f3/popcnt
#endif
    /* B9 */ ENTRY_Invalid,                            // _B9
    /* BA */ ENTRY_CopyBytes2Mod1,                     // BT & BTC & BTR & BTS (0F BA)
    /* BB */ ENTRY_CopyBytes2Mod,                      // BTC (0F BB)
    /* BC */ ENTRY_CopyBytes2Mod,                      // BSF (0F BC)
    /* BD */ ENTRY_CopyBytes2Mod,                      // BSR (0F BD)
    /* BE */ ENTRY_CopyBytes2Mod,                      // MOVSX/r
    /* BF */ ENTRY_CopyBytes2Mod,                      // MOVSX/r
    /* C0 */ ENTRY_CopyBytes2Mod,                      // XADD/r
    /* C1 */ ENTRY_CopyBytes2Mod,                      // XADD/r
    /* C2 */ ENTRY_CopyBytes2Mod1,                     // CMPPS &
    /* C3 */ ENTRY_CopyBytes2Mod,                      // MOVNTI
    /* C4 */ ENTRY_CopyBytes2Mod1,                     // PINSRW /r ib
    /* C5 */ ENTRY_CopyBytes2Mod1,                     // PEXTRW /r ib
    /* C6 */ ENTRY_CopyBytes2Mod1,                     // SHUFPS & SHUFPD
    /* C7 */ ENTRY_CopyBytes2Mod,                      // CMPXCHG8B (0F C7)
    /* C8 */ ENTRY_CopyBytes1,                         // BSWAP 0F C8 + rd
    /* C9 */ ENTRY_CopyBytes1,                         // BSWAP 0F C8 + rd
    /* CA */ ENTRY_CopyBytes1,                         // BSWAP 0F C8 + rd
    /* CB */ ENTRY_CopyBytes1,                         // CVTPD2PI BSWAP 0F C8 + rd
    /* CC */ ENTRY_CopyBytes1,                         // BSWAP 0F C8 + rd
    /* CD */ ENTRY_CopyBytes1,                         // BSWAP 0F C8 + rd
    /* CE */ ENTRY_CopyBytes1,                         // BSWAP 0F C8 + rd
    /* CF */ ENTRY_CopyBytes1,                         // BSWAP 0F C8 + rd
    /* D0 */ ENTRY_CopyBytes2Mod,                      // ADDSUBPS (untestd)
    /* D1 */ ENTRY_CopyBytes2Mod,                      // PSRLW/r
    /* D2 */ ENTRY_CopyBytes2Mod,                      // PSRLD/r
    /* D3 */ ENTRY_CopyBytes2Mod,                      // PSRLQ/r
    /* D4 */ ENTRY_CopyBytes2Mod,                      // PADDQ
    /* D5 */ ENTRY_CopyBytes2Mod,                      // PMULLW/r
    /* D6 */ ENTRY_CopyBytes2Mod,                      // MOVDQ2Q / MOVQ2DQ
    /* D7 */ ENTRY_CopyBytes2Mod,                      // PMOVMSKB/r
    /* D8 */ ENTRY_CopyBytes2Mod,                      // PSUBUSB/r
    /* D9 */ ENTRY_CopyBytes2Mod,                      // PSUBUSW/r
    /* DA */ ENTRY_CopyBytes2Mod,                      // PMINUB/r
    /* DB */ ENTRY_CopyBytes2Mod,                      // PAND/r
    /* DC */ ENTRY_CopyBytes2Mod,                      // PADDUSB/r
    /* DD */ ENTRY_CopyBytes2Mod,                      // PADDUSW/r
    /* DE */ ENTRY_CopyBytes2Mod,                      // PMAXUB/r
    /* DF */ ENTRY_CopyBytes2Mod,                      // PANDN/r
    /* E0 */ ENTRY_CopyBytes2Mod ,                     // PAVGB
    /* E1 */ ENTRY_CopyBytes2Mod,                      // PSRAW/r
    /* E2 */ ENTRY_CopyBytes2Mod,                      // PSRAD/r
    /* E3 */ ENTRY_CopyBytes2Mod,                      // PAVGW
    /* E4 */ ENTRY_CopyBytes2Mod,                      // PMULHUW/r
    /* E5 */ ENTRY_CopyBytes2Mod,                      // PMULHW/r
    /* E6 */ ENTRY_CopyBytes2Mod,                      // CTDQ2PD &
    /* E7 */ ENTRY_CopyBytes2Mod,                      // MOVNTQ
    /* E8 */ ENTRY_CopyBytes2Mod,                      // PSUBB/r
    /* E9 */ ENTRY_CopyBytes2Mod,                      // PSUBW/r
    /* EA */ ENTRY_CopyBytes2Mod,                      // PMINSW/r
    /* EB */ ENTRY_CopyBytes2Mod,                      // POR/r
    /* EC */ ENTRY_CopyBytes2Mod,                      // PADDSB/r
    /* ED */ ENTRY_CopyBytes2Mod,                      // PADDSW/r
    /* EE */ ENTRY_CopyBytes2Mod,                      // PMAXSW /r
    /* EF */ ENTRY_CopyBytes2Mod,                      // PXOR/r
    /* F0 */ ENTRY_CopyBytes2Mod,                      // LDDQU
    /* F1 */ ENTRY_CopyBytes2Mod,                      // PSLLW/r
    /* F2 */ ENTRY_CopyBytes2Mod,                      // PSLLD/r
    /* F3 */ ENTRY_CopyBytes2Mod,                      // PSLLQ/r
    /* F4 */ ENTRY_CopyBytes2Mod,                      // PMULUDQ/r
    /* F5 */ ENTRY_CopyBytes2Mod,                      // PMADDWD/r
    /* F6 */ ENTRY_CopyBytes2Mod,                      // PSADBW/r
    /* F7 */ ENTRY_CopyBytes2Mod,                      // MASKMOVQ
    /* F8 */ ENTRY_CopyBytes2Mod,                      // PSUBB/r
    /* F9 */ ENTRY_CopyBytes2Mod,                      // PSUBW/r
    /* FA */ ENTRY_CopyBytes2Mod,                      // PSUBD/r
    /* FB */ ENTRY_CopyBytes2Mod,                      // FSUBQ/r
    /* FC */ ENTRY_CopyBytes2Mod,                      // PADDB/r
    /* FD */ ENTRY_CopyBytes2Mod,                      // PADDW/r
    /* FE */ ENTRY_CopyBytes2Mod,                      // PADDD/r
    /* FF */ ENTRY_Invalid,                            // _FF
};

_Use_decl_annotations_
PVOID DetourCopyInstruction(PVOID pDst,
    PVOID* ppDstPool,
    PVOID pSrc,
    PVOID* ppTarget,
    LONG* pExtra) {
    UNREFERENCED_PARAMETER(ppDstPool);

    Disasm disasm((PUCHAR*)ppTarget, pExtra);
    return disasm.CopyInstruction((PUCHAR)pDst, (PUCHAR)pSrc);
}

Disasm::Disasm(PUCHAR* ppTarget, LONG* pExtra) {
    m_ppTarget = ppTarget ? ppTarget : &m_pScratchTarget;
    m_pExtra = pExtra ? pExtra : &m_ScratchExtra;
    *m_ppTarget = nullptr;
    *m_pExtra = 0;
}

PUCHAR Disasm::CopyInstruction(PUCHAR pDst, PUCHAR pSrc) {
    // Configure scratch areas if real areas are not available.
    if (nullptr == pDst) {
        pDst = m_ScratchDst;
    }
    if (nullptr == pSrc) {
        // We cannot copy a non-existent instruction
        return nullptr;
    }

    // Figure out how big the instruction is, do the appropriate copy,
    // and figure out what the target of the instruction is if any
    //
    RefCopyEntry pEntry = &s_CopyTable[pSrc[0]];
    return (this->*pEntry->pfCopy)(pEntry, pDst, pSrc);
}

PUCHAR Disasm::CopyBytes(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    ULONG bytesFixed;

    if (m_Vex || m_Evex) {
        ASSERT(pEntry->FlagBits == FlagBits::None);
        ASSERT(pEntry->FixedSize == pEntry->FixedSize16);
    }

    ULONG const modOffset = pEntry->ModOffset;
    FlagBits flagBits = pEntry->FlagBits;
    ULONG const fixedSize = pEntry->FixedSize;
    ULONG const fixedSize16 = pEntry->FixedSize16;

    bool address = (flagBits & FlagBits::Address) == FlagBits::Address;
    if (address) {
        bytesFixed = m_AddressOverride ? fixedSize16 : fixedSize;
    }
#ifdef DETOURS_X64
    // REX.W trumps 66
    else if (m_RaxOverride) {
        bool rax = (flagBits & FlagBits::Rax) == FlagBits::Rax;
        bytesFixed = fixedSize + (rax ? 4 : 0);
    }
#endif // DETOURS_X64
    else {
        bytesFixed = m_OperandOverride ? fixedSize16 : fixedSize;
    }

    ULONG bytes = bytesFixed;
    ULONG relOffset = pEntry->RelOffset;
    ULONG target = bytes - relOffset;
    if (modOffset > 0) {
        ASSERT(relOffset == 0);
        UCHAR const modRm = pSrc[modOffset];
        UCHAR const flags = s_ModRm[modRm];

        bytes += flags & static_cast<UCHAR>(ModRm::NotSib);
        if (flags & static_cast<UCHAR>(ModRm::Sib)) {
            UCHAR const sib = pSrc[modOffset + 1];

            if ((sib & 0x07) == 0x05) {
                if ((modRm & 0xc0) == 0x00) {
                    bytes += 4;
                }
                else if ((modRm & 0xc0) == 0x40) {
                    bytes += 1;
                }
                else if ((modRm & 0xc0) == 0x80) {
                    bytes += 4;
                }
            }
            target = bytes - relOffset;
        }
#ifdef DETOURS_X64
        else if (flags & static_cast<UCHAR>(ModRm::Rip)) {
            relOffset = modOffset + 1;
            target = 4;
        }
#endif // DETOURS_X64
    }
    RtlCopyMemory(pDst, pSrc, bytes);

    if (relOffset) {
        *m_ppTarget = AdjustTarget(pDst, pSrc, bytes, relOffset, target);
#ifdef DETOURS_X64
        if (pEntry->RelOffset == 0) {
            // This is a data target, not a code target, so we shouldn't return it.
            *m_ppTarget = nullptr;
        }
#endif // DETOURS_X64
    }
    bool noEnlarge = (flagBits & FlagBits::NoEnlarge) == FlagBits::NoEnlarge;
    if (noEnlarge) {
        *m_pExtra = -*m_pExtra;
    }
    bool dynamic = (flagBits & FlagBits::Dynamic) == FlagBits::Dynamic;
    if (dynamic) {
        *m_ppTarget = (PUCHAR)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    }
    return pSrc + bytes;
}

PUCHAR Disasm::CopyBytesPrefix(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    pDst[0] = pSrc[0];
    pEntry = &s_CopyTable[pSrc[1]];
    return (this->*pEntry->pfCopy)(pEntry, pDst + 1, pSrc + 1);
}

PUCHAR Disasm::CopyBytesSegment(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    m_SegmentOverride = pSrc[0];
    return CopyBytesPrefix(nullptr, pDst, pSrc);
}

PUCHAR Disasm::CopyBytesRax(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    // AMD64 only
    if (pSrc[0] * 0x8) {
        m_RaxOverride = true;
    }
    return CopyBytesPrefix(nullptr, pDst, pSrc);
}

PUCHAR Disasm::CopyBytesJump(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    UNREFERENCED_PARAMETER(pEntry);

    PVOID pSrcAddr = &pSrc[1];
    PVOID pDstAddr = nullptr;
    LONG_PTR oldOffset = (LONG_PTR) * (signed char*&)pSrcAddr;
    LONG_PTR newOffset = 0;

    *m_ppTarget = pSrc + 2 + oldOffset;

    if (pSrc[0] == 0xeb) {
        pDst[0] = 0xe9;
        pDstAddr = &pDst[1];
        newOffset = oldOffset - ((pDst - pSrc) + 3);
        *(UNALIGNED LONG*&)pDstAddr = (LONG)newOffset;
        *m_pExtra = 3;
        return pSrc + 2;
    }

    ASSERT(pSrc[0] >= 0x70 && pSrc[0] <= 0x7f);

    pDst[0] = 0x0f;
    pDst[1] = 0x80 | (pSrc[0] & 0xf);
    pDstAddr = &pDst[2];
    newOffset = oldOffset - ((pDst - pSrc) + 4);
    *(UNALIGNED LONG*&)pDstAddr = (LONG)newOffset;

    *m_pExtra = 4;
    return pSrc + 2;
}

PUCHAR Disasm::AdjustTarget(PUCHAR pDst, PUCHAR pSrc, ULONG op, ULONG targetOffset,
    ULONG targetSize) {
    PUCHAR pTarget = nullptr;

#if 1 // fault injection to test code
#ifdef DETOURS_X64
    typedef LONGLONG T;
#else
    typedef LONG T;
#endif // DETOURS_X64

    T oldOffset;
    T newOffset;
    PVOID pTargetAddr = &pDst[targetOffset];

    switch (targetSize) {
        case 1:
            oldOffset = *(signed char*&)pTargetAddr;
            break;
        case 2:
            oldOffset = *(UNALIGNED SHORT*&)pTargetAddr;
            break;
        case 4:
            oldOffset = *(UNALIGNED LONG*&)pTargetAddr;
            break;
#if defined(DETOURS_X64)
        case 8:
            oldOffset = *(UNALIGNED LONGLONG*&)pTargetAddr;
            break;
#endif
        default:
            ASSERT(!"cbTargetSize is invalid.");
            oldOffset = 0;
            break;
    }

    pTarget = pSrc + op + oldOffset;
    newOffset = oldOffset - (T)(pDst - pSrc);

    switch (targetSize) {
        case 1:
            *(CHAR*&)pTargetAddr = (CHAR)newOffset;
            if (newOffset < SCHAR_MIN || newOffset > SCHAR_MAX) {
                *m_pExtra = sizeof(ULONG) - 1;
            }
            break;
        case 2:
            *(UNALIGNED SHORT*&)pTargetAddr = (SHORT)newOffset;
            if (newOffset < SHRT_MIN || newOffset > SHRT_MAX) {
                *m_pExtra = sizeof(ULONG) - 2;
            }
            break;
        case 4:
            *(UNALIGNED LONG*&)pTargetAddr = (LONG)newOffset;
            if (newOffset < LONG_MIN || newOffset > LONG_MAX) {
                *m_pExtra = sizeof(ULONG) - 4;
            }
            break;
#if defined(DETOURS_X64)
        case 8:
            *(UNALIGNED LONGLONG*&)pTargetAddr = newOffset;
            break;
#endif
    }

#ifdef DETOURS_X64
    // When we are only computing size, source and dest can be
    // far apart, distance not encodable in 32bits. Ok.
    // At least still check the lower 32bits.

    if (pDst >= m_ScratchDst && pDst < (sizeof(m_ScratchDst) + m_ScratchDst)) {
        ASSERT((((size_t)pDst + op + newOffset) & 0xFFFFFFFF) == (((size_t)pTarget) & 0xFFFFFFFF));
    }
    else
#endif
    {
        ASSERT(pDst + op + newOffset == pTarget);
    }
#endif
    return pTarget;
}

PUCHAR Disasm::Invalid(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    UNREFERENCED_PARAMETER(pEntry);
    UNREFERENCED_PARAMETER(pDst);

    ASSERT(!"Invalid Instruction");
    return pSrc + 1;
}

////////////////////////////////////////////////////// Individual Bytes Codes.
//
PUCHAR Disasm::Copy0F(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    pDst[0] = pSrc[0];
    pEntry = &s_CopyTable0F[pSrc[1]];
    return (this->*pEntry->pfCopy)(pEntry, pDst + 1, pSrc + 1);
}

// x86 only sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6/dynamic invalid/7
PUCHAR Disasm::Copy0F00(RefCopyEntry, PUCHAR pDst, PUCHAR pSrc) {
    // jmpe is 32bit x86 only
    // Notice that the size are the same either way, but jmpe is marked as "dynamic"
    static const CopyEntry other = /* B8 */ ENTRY_CopyBytes2Mod; // sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6 invalid/7
    static const CopyEntry jmpe = /* B8 */ ENTRY_CopyBytes2ModDynamic; // jmpe/6 x86-on-IA64 syscalls

    RefCopyEntry const pEntry = (((6 << 3) == ((7 << 3) & pSrc[1])) ? &jmpe : &other);
    return (this->*pEntry->pfCopy)(pEntry, pDst, pSrc);
}
// vmread, 66/extrq/ib/ib, F2/insertq/ib/ib
PUCHAR Disasm::Copy0F78(RefCopyEntry, PUCHAR pDst, PUCHAR pSrc) {
    // vmread, 66/extrq, F2/insertq
    static const CopyEntry vmread = /* 78 */ ENTRY_CopyBytes2Mod;
    static const CopyEntry extrq_insertq = /* 78 */ ENTRY_CopyBytes4;

    ASSERT(!(m_F2 && m_OperandOverride));
    // For insertq and presumably despite documentation extrq, mode must be 11, not checked.
    // insertq/extrq/78 are followed by two immediate bytes, and given mode == 11, mod/rm byte is always one byte,
    // and the 0x78 makes 4 bytes (not counting the 66/F2/F which are accounted for elsewhere)
    RefCopyEntry const pEntry = ((m_F2 || m_OperandOverride) ? &extrq_insertq : &vmread);

    return (this->*pEntry->pfCopy)(pEntry, pDst, pSrc);
}
// jmpe or F3/popcnt
PUCHAR Disasm::Copy0FB8(RefCopyEntry, PUCHAR pDst, PUCHAR pSrc) {
    // jmpe is 32bit x86 only
    static const CopyEntry popcnt = /* B8 */ ENTRY_CopyBytes2Mod;
    static const CopyEntry jmpe = /* B8 */ ENTRY_CopyBytes3Or5Dynamic; // jmpe x86-on-IA64 syscalls
    RefCopyEntry const pEntry = m_F3 ? &popcnt : &jmpe;
    return (this->*pEntry->pfCopy)(pEntry, pDst, pSrc);
}

PUCHAR Disasm::Copy66(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    // Operand-size override prefix
    m_OperandOverride = TRUE;
    return CopyBytesPrefix(pEntry, pDst, pSrc);
}

PUCHAR Disasm::Copy67(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    m_AddressOverride = true;
    return CopyBytesPrefix(pEntry, pDst, pSrc);
}

PUCHAR Disasm::CopyF2(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    m_F2 = true;
    return CopyBytesPrefix(pEntry, pDst, pSrc);
}

PUCHAR Disasm::CopyF3(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    // x86 only
    m_F3 = TRUE;
    return CopyBytesPrefix(pEntry, pDst, pSrc);
}

PUCHAR Disasm::CopyF6(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    UNREFERENCED_PARAMETER(pEntry);

    // TEST BYTE /0
    if (0x00 == (0x38 & pSrc[1])) {    // reg(bits 543) of ModR/M == 0
        static const CopyEntry ce = /* f6 */ ENTRY_CopyBytes2Mod1;
        return (this->*ce.pfCopy)(&ce, pDst, pSrc);
    }
    // DIV /6
    // IDIV /7
    // IMUL /5
    // MUL /4
    // NEG /3
    // NOT /2

    static const CopyEntry ce = /* f6 */ ENTRY_CopyBytes2Mod;
    return (this->*ce.pfCopy)(&ce, pDst, pSrc);
}

PUCHAR Disasm::CopyF7(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    UNREFERENCED_PARAMETER(pEntry);

    // TEST WORD /0
    if (0x00 == (0x38 & pSrc[1])) {    // reg(bits 543) of ModR/M == 0
        static const CopyEntry ce = /* f7 */ ENTRY_CopyBytes2ModOperand;
        return (this->*ce.pfCopy)(&ce, pDst, pSrc);
    }

    // DIV /6
    // IDIV /7
    // IMUL /5
    // MUL /4
    // NEG /3
    // NOT /2
    static const CopyEntry ce = /* f7 */ ENTRY_CopyBytes2Mod;
    return (this->*ce.pfCopy)(&ce, pDst, pSrc);
}

PUCHAR Disasm::CopyFF(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    // INC /0
    // DEC /1
    // CALL /2
    // CALL /3
    // JMP /4
    // JMP /5
    // PUSH /6
    // invalid/7
    UNREFERENCED_PARAMETER(pEntry);

    static const CopyEntry ce = /* ff */ ENTRY_CopyBytes2Mod;
    PUCHAR pOut = (this->*ce.pfCopy)(&ce, pDst, pSrc);

    UCHAR const b1 = pSrc[1];

    if (0x15 == b1 || 0x25 == b1) {         // CALL [], JMP []
#ifdef DETOURS_X64
        // All segments but FS and GS are equivalent.
        if (m_SegmentOverride != 0x64 && m_SegmentOverride != 0x65)
#else
        if (m_SegmentOverride == 0 || m_SegmentOverride == 0x2E)
#endif
        {
#ifdef DETOURS_X64
            INT32 offset = *(UNALIGNED INT32*) & pSrc[2];
            PUCHAR* ppTarget = (PUCHAR*)(pSrc + 6 + offset);
#else
            PUCHAR* ppTarget = (PUCHAR*)(SIZE_T) * (UNALIGNED ULONG*) & pSrc[2];
#endif
            if (s_fLimitReferencesToModule &&
                (ppTarget < (PVOID)s_pModuleBeg || ppTarget >= (PVOID)s_pModuleEnd)) {

                *m_ppTarget = (PUCHAR)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
            }
            else {
                // This can access violate on random bytes. Use DetourSetCodeModule.
                *m_ppTarget = *ppTarget;
            }
        }
        else {
            *m_ppTarget = (PUCHAR)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
        }
    }
    else if (0x10 == (0x30 & b1) || // CALL /2 or /3  --> reg(bits 543) of ModR/M == 010 or 011
        0x20 == (0x30 & b1)) { // JMP /4 or /5 --> reg(bits 543) of ModR/M == 100 or 101
        *m_ppTarget = (PUCHAR)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    }
    return pOut;
}

PUCHAR Disasm::CopyVex2(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    // 2 byte VEX prefix 0xC5
#ifdef DETOURS_X86
    const static CopyEntry ceLDS = /* C5 */ ENTRY_CopyBytes2Mod;
    if ((pSrc[1] & 0xC0) != 0xC0) {
        RefCopyEntry pEntry = &ceLDS;
        return (this->*pEntry->pfCopy)(pEntry, pDst, pSrc);
    }
#endif
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    return CopyVexCommon(1, pDst + 2, pSrc + 2);
}

PUCHAR Disasm::CopyVex3(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    // 3 byte VEX prefix 0xc4
#ifdef DETOURS_X86
    const static CopyEntry ceLES = /* C4 */ ENTRY_CopyBytes2Mod;
    if ((pSrc[1] & 0xC0) != 0xC0) {
        RefCopyEntry pEntry = &ceLES;
        return (this->*pEntry->pfCopy)(pEntry, pDst, pSrc);
    }
#endif
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
#ifdef DETOURS_X64
    m_RaxOverride |= !!(pSrc[2] & 0x80); // w in last byte, see CopyBytesRax
#else
    //
    // TODO
    //
    // Usually the VEX.W bit changes the size of a general purpose register and is ignored for 32bit.
    // Sometimes it is an opcode extension.
    // Look in the Intel manual, in the instruction-by-instruction reference, for ".W1",
    // without nearby wording saying it is ignored for 32bit.
    // For example: "VFMADD132PD/VFMADD213PD/VFMADD231PD Fused Multiply-Add of Packed Double-Precision Floating-Point Values".
    //
    // Then, go through each such case and determine if W0 vs. W1 affect the size of the instruction. Probably not.
    // Look for the same encoding but with "W1" changed to "W0".
    // Here is one such pairing:
    // VFMADD132PD/VFMADD213PD/VFMADD231PD Fused Multiply-Add of Packed Double-Precision Floating-Point Values
    //
    // VEX.DDS.128.66.0F38.W1 98 /r A V/V FMA Multiply packed double-precision floating-point values
    // from xmm0 and xmm2/mem, add to xmm1 and
    // put result in xmm0.
    // VFMADD132PD xmm0, xmm1, xmm2/m128
    //
    // VFMADD132PS/VFMADD213PS/VFMADD231PS Fused Multiply-Add of Packed Single-Precision Floating-Point Values
    // VEX.DDS.128.66.0F38.W0 98 /r A V/V FMA Multiply packed single-precision floating-point values
    // from xmm0 and xmm2/mem, add to xmm1 and put
    // result in xmm0.
    // VFMADD132PS xmm0, xmm1, xmm2/m128
    //
#endif
    return CopyVexCommon(pSrc[1] & 0x1F, pDst + 3, pSrc + 3);
}

PUCHAR Disasm::CopyVexEvexCommon(UCHAR m, PUCHAR pDst, PUCHAR pSrc,UCHAR p,UCHAR fp16) {
    // m is first instead of last in the hopes of pbDst/pbSrc being
    // passed along efficiently in the registers they were already in.
    static const CopyEntry ceF38 = /* 38 */ ENTRY_CopyBytes2Mod;
    static const CopyEntry ceF3A = /* 3A */ ENTRY_CopyBytes2Mod1;
    static const CopyEntry ceInvalid = /* C4 */ ENTRY_Invalid;

    switch (p & 3) {
        case 0: break;
        case 1: m_OperandOverride = TRUE; break;
        case 2: m_F3 = TRUE; break;
        case 3: m_F2 = TRUE; break;
    }

    RefCopyEntry pEntry;

    // see https://software.intel.com/content/www/us/en/develop/download/intel-avx512-fp16-architecture-specification.html
    switch (m | fp16) {
        default: return Invalid(&ceInvalid, pDst, pSrc);
        case 1:  pEntry = &s_CopyTable0F[pSrc[0]];
            return (this->*pEntry->pfCopy)(pEntry, pDst, pSrc);
        case 5:  // fallthrough
        case 6:  // fallthrough
        case 2:  return CopyBytes(&ceF38, pDst, pSrc);
        case 3:  return CopyBytes(&ceF3A, pDst, pSrc);
    }
}

PUCHAR Disasm::CopyVexCommon(UCHAR m, PUCHAR pDst, PUCHAR pSrc) {
    // m is first instead of last in the hopes of pDst/pSrc being
    // passed along efficiently in the registers they were already in.
    m_Vex = TRUE;
    UCHAR const p = (UCHAR)(pSrc[-1] & 3); // p in last byte
    return CopyVexEvexCommon(m, pDst, pSrc, p);
}

PUCHAR Disasm::CopyEvex(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
    // 62, 3 byte payload, x86 with implied prefixes like Vex
    // for 32bit, mode 0xC0 else fallback to bound /r
    // NOTE: Intel and Wikipedia number these differently.
    // Intel says 0-2, Wikipedia says 1-3.
    UCHAR const p0 = pSrc[1];

#ifdef DETOURS_X86
    const static CopyEntry ceBound = /* 62 */ ENTRY_CopyBytes2Mod;
    if ((p0 & 0xC0) != 0xC0) {
        return CopyBytes(&ceBound, pDst, pSrc);
    }
#endif

    static const CopyEntry ceInvalid = /* 62 */ ENTRY_Invalid;

    // This could also be handled by default in CopyVexEvexCommon
    // if 4u changed to 4|8.
    if (p0 & 8u)
        return Invalid(&ceInvalid, pDst, pSrc);

    UCHAR const p1 = pSrc[2];

    if ((p1 & 0x04) != 0x04)
        return Invalid(&ceInvalid, pDst, pSrc);

    // Copy 4 byte prefix.
    *(UNALIGNED ULONG*)pDst = *(UNALIGNED ULONG*)pSrc;

    m_Evex = TRUE;

#ifdef DETOURS_X64
    m_RaxOverride |= !!(p1 & 0x80); // w
#endif

    return CopyVexEvexCommon(p0 & 3u, pDst + 4, pSrc + 4, p1 & 3u, p0 & 4u);
}

PUCHAR Disasm::CopyXop(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc) {
/* 3 byte AMD XOP prefix 0x8F
byte0: 0x8F
byte1: RXBmmmmm
byte2: WvvvvLpp
byte3: opcode
mmmmm >= 8, else pop
mmmmm only otherwise defined for 8, 9, A.
pp is like VEX but only instructions with 0 are defined
*/
    const static CopyEntry cePop = /* 8F */ ENTRY_CopyBytes2Mod;
    const static CopyEntry ceXop = /* 8F */ ENTRY_CopyBytesXop;
    const static CopyEntry ceXop1 = /* 8F */ ENTRY_CopyBytesXop1;
    const static CopyEntry ceXop4 = /* 8F */ ENTRY_CopyBytesXop4;

    UCHAR const m = (UCHAR)(pSrc[1] & 0x1F);
    ASSERT(m <= 10);
    switch (m)
    {
        default:
            return CopyBytes(&cePop, pDst, pSrc);

        case 8: // modrm with 8bit immediate
            return CopyBytes(&ceXop1, pDst, pSrc);

        case 9: // modrm with no immediate
            return CopyBytes(&ceXop, pDst, pSrc);

        case 10: // modrm with 32bit immediate
            return CopyBytes(&ceXop4, pDst, pSrc);
    }
}