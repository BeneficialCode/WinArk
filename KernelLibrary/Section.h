#pragma once

enum class SectionType {
    None,
    Native, // Native section - meaning: 64bit on a 64 bit OS, or 32-bit on a 32-bit OS
    Wow,    // Wow64 section - meaning 32-bit on a 64-bit OS
};

struct DllStats {
    SectionType Type;
    PVOID Section{ nullptr };   // Section Object
    bool IsVaid() {
        return Section != nullptr;
    }
};

struct Section {
    SectionType _type;
    RTL_RUN_ONCE _sectionSingletonState;

    NTSTATUS Initialize(SectionType type);
    NTSTATUS GetSection(DllStats** ppSectionInfo = nullptr);
    NTSTATUS FreeSection();
private:
    NTSTATUS CreateKnownDllSection(DllStats& pDllState);
};