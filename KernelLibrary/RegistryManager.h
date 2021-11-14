#pragma once

enum class RegistryAccessMask {
	None = 0,
	Write = KEY_WRITE,
	Read = KEY_READ,
};
DEFINE_ENUM_FLAG_OPERATORS(RegistryAccessMask);

struct RegistryManager {
	NTSTATUS Open(PUNICODE_STRING path,RegistryAccessMask accessMask=RegistryAccessMask::Write,ULONG opts = REG_OPTION_NON_VOLATILE);
	NTSTATUS Close();
	
	NTSTATUS SetDWORDValue(PUNICODE_STRING name, const ULONG& value);
	NTSTATUS SetQWORDValue(PUNICODE_STRING name, const ULONGLONG& value);
	//NTSTATUS SetMultiStringValue(PUNICODE_STRING name,);
	//NTSTATUS SetStringValue(PUNICODE_STRING name,);
	//NTSTATUS SetBinaryValue(PUNICODE_STRING name,);
	
	NTSTATUS QueryDWORDValue(PUNICODE_STRING name, ULONG& value);

private:
	HANDLE _key;
};